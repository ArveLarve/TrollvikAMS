#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <PubSubClient.h>

#include <DallasTemperature.h>
#include <OneWire.h>

// -------------  WiFi ------------------------------------------ //

const char wifiSSID[] = "";
const char wifiPassword[] = "";


// ------------- MQTT ------------------------------------------- //

const uint16_t  mqttPort        = 8883;
const char      mqttHost[]      = "{iot-hub}.azure-devices.net";

const char      mqttUser[]      = "{iot-hub}.azure-devices.net/{deviceId}/?api-version=2018-06-30";
const char      mqttPass[]      = "SharedAccessSignature sr={iot-hub}.azure-devices.net%2Fdevices%2F{deviceId}&sig=lz5uMJJOQU7JJdkPE...Neg%2BVOYffuI%3D&se=16200000000";

const char      mqttClientId[]  = "{deviceId}";
const char      mqttTopic[]     = "devices/{deviceId}/messages/events/";
#define useSSL true


// ------------- SSL/TLS ---------------------------------------- //

const char caCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ
RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD
VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX
DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y
ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy
VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr
mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr
IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK
mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu
XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy
dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye
jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1
BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3
DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92
9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx
jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0
Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz
ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS
R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp
-----END CERTIFICATE-----
)EOF";

X509List caCertX509(caCert);


// ------------- Variables -------------------------------------- //

#if useSSL
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif
PubSubClient mqttClient(espClient);

ulong lastTransmission = 0;
uint32_t transmissionCounter = 0;

#define ONE_WIRE_PIN 5
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

// ------------- Function declarations -------------------------- //

void setClock();
bool verifylts();
void reconnectMqtt();
float getTemperatures();

// ------------- Structs ---------------------------------------- //
