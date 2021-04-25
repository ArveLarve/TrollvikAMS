#include "./main.h"

void setup() {
  Debug.showColors(true); // Colors

  // ---- Serial port --------------------------------- //
  // Serial.setDebugOutput(true);
  Serial.begin(2400, SERIAL_8E1);
  debugI("Serial starts...");


  // ---- Temperature sensor initialization ----------- //
  sensors.begin();

  if (Debug.isActive(RemoteDebug::INFO)) {
    // locate devices on the bus
    Debug.print("Found ");
    Debug.print(sensors.getDeviceCount(), DEC);
    Debug.println(" devices.");
  }


  // ---- WiFi ---------------------------------------- //
  rdebugI("Wifi connecting...");

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(wifiSSID, wifiPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    rdebugI(".");
  }

  if (Debug.isActive(RemoteDebug::INFO)) {
    Debug.print("");
    Debug.print("Connected to WiFi, IP address: ");
    Debug.println(WiFi.localIP());
  }
  
  #if useSSL
    // ---- Setting time from NTP
    setClock();

    // ---- Verifying MQTT Host Certificate
    if (Debug.isActive(RemoteDebug::INFO)) {
      Debug.print("Free RAM pre TLS: ");
      Debug.println(ESP.getFreeHeap());
    }

    espClient.setTrustAnchors(&caCertX509);
    verifylts();

    if (Debug.isActive(RemoteDebug::INFO)) {
      Debug.print("Free RAM post TLS: ");
      Debug.println(ESP.getFreeHeap());
    }
  #endif
  mqttClient.setServer(mqttHost, mqttPort);

  hanReader.setup(&Serial);
}

void loop() {
  int power = readHan();
  if(power != -1) {
    lastPowerReading = power;
  }
  // ---- MQTT
  if (!mqttClient.connected()) {
    reconnectMqtt();
  }
  if (millis() - lastTransmission > 10000) {
    String json = "{";
    json += "\"Temperature\": ";
    json += String(getTemperatures());
    json += ", \"Power\": ";
    json += String(lastPowerReading);
    json += "}";

    mqttClient.publish(mqttTopic,  json.c_str());
    lastTransmission = millis();

    if (Debug.isActive(RemoteDebug::INFO)) {
      Debug.print("Sent json: ");
      Debug.println(json);
    }
  }
  mqttClient.loop();
}

// ------------- Helper Functions ------------------------------- //

void setClock() {
  configTime(2 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  rdebugI("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    rdebugI(".");
    now = time(nullptr);
  }
  debugI("");
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  if (Debug.isActive(RemoteDebug::INFO)) {
    Debug.print("Current time: ");
    Debug.println(asctime(&timeinfo));
  }
}

#if useSSL
bool verifylts() {
  bool success = false;
  
  debugI("Verifying TLS connection to MQTT host");

  success = espClient.connect(mqttHost, mqttPort);

  if (success) {
    debugI("Connection complete, valid cert.");
  }
  else {
    debugI("Connection failed!");
    char buf[64];
    espClient.getLastSSLError(buf, 64);

    if (Debug.isActive(RemoteDebug::INFO)) {
      Debug.println(buf);
    }
  }

  return (success);
}
#endif

void reconnectMqtt() {
  while (!mqttClient.connected()) {
    debugI("Connecting to MQTT broker...");
    #if useSSL
    if (mqttClient.connect(mqttClientId, mqttUser, mqttPass)) {
    #else
    if (mqttClient.connect(mqttClientId)) {
    #endif
      debugI("connected");
    } else {
      if (Debug.isActive(RemoteDebug::INFO)) {
        Debug.print("Failed, rc=");
        Debug.print(mqttClient.state());
        Debug.println(". Trying again in 5 seconds...");
      }

      delay(5000);
    }
  }
}

float getTemperatures() {
  DeviceAddress addr;
  sensors.requestTemperatures();
  sensors.getAddress(addr, 0);
  return sensors.getTempC(addr);
}

int readHan() {
  int power = -1;

  if (hanReader.read()) {
    int listSize = hanReader.getListSize();

    if (Debug.isActive(RemoteDebug::DEBUG)) {
      Debug.println("");
      Debug.print("List size: ");
      Debug.print(listSize);
      Debug.print(": ");
    }

    if (listSize == (int)Kaifa::List1) {
      power = hanReader.getInt((int)Kaifa_List1::ActivePowerImported);
    }

    if (Debug.isActive(RemoteDebug::DEBUG)) {
      Debug.print("Power consumtion is right now: ");
      Debug.print(power);
      Debug.println(" W");
    }
  }
  return power;
}
