#include "./main.h"

void setup() {

  // ---- Serial port --------------------------------- //
  // Serial.setDebugOutput(true);
  Serial.begin(2400, SERIAL_8E1);
  Serial.println("Serial starts...");


  // ---- Temperature sensor initialization ----------- //
  sensors.begin();

  // locate devices on the bus
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");


  // ---- WiFi ---------------------------------------- //
  Serial.print("Wifi connecting...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(wifiSSID, wifiPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi, IP address: ");
  Serial.println(WiFi.localIP());
  
  #if useSSL
    // ---- Setting time from NTP
    setClock();

    // ---- Verifying MQTT Host Certificate
    Serial.print("Free RAM pre TLS: ");
    Serial.println(ESP.getFreeHeap());
  
    espClient.setTrustAnchors(&caCertX509);
    verifylts();

    Serial.print("Free RAM post TLS: ");
    Serial.println(ESP.getFreeHeap());
  #endif
  mqttClient.setServer(mqttHost, mqttPort);
}

void loop() {

  // ---- MQTT
  if (!mqttClient.connected()) {
    reconnectMqtt();
  }
  if (millis() - lastTransmission > 10000) {
    String json = "{\"TransmissionCounter\":";
    json += String(getTemperatures()) + "}";

    mqttClient.publish(mqttTopic,  json.c_str());
    lastTransmission = millis();

    Serial.print("Sent json: ");
    Serial.println(json);
  }
  mqttClient.loop();
}

// ------------- Helper Functions ------------------------------- //

void setClock() {
  configTime(2 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

#if useSSL
bool verifylts() {
  bool success = false;
  
  Serial.println("Verifying TLS connection to MQTT host");

  success = espClient.connect(mqttHost, mqttPort);

  if (success) {
    Serial.println("Connection complete, valid cert.");
  }
  else {
    Serial.println("Connection failed!");
    char buf[64];
    espClient.getLastSSLError(buf, 64);
    Serial.print(buf);
  }

  return (success);
}
#endif

void reconnectMqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT broker...");
    #if useSSL
    if (mqttClient.connect(mqttClientId, mqttUser, mqttPass)) {
    #else
    if (mqttClient.connect(mqttClientId)) {
    #endif
      Serial.println("connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Trying again in 5 seconds...");

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