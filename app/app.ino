
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>
#include <ArduinoJson.h>
#include "iothubtransportmqtt.h"

#define DEVICE_ID "NodeMcu"
#define MESSAGE_MAX_LEN 256
#define INTERVAL 5000
#define LED1 BUILTIN_LED
#define LED2 16

static bool messagePending = false;
static bool messageSending = true;

static int messageCount = 1;

char ssid[] = "WIFI SSID";
char pass[] = "WIFI PASSWORD";
char connectionString[] = "PRIMARY CONNECTION STRING";


static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
static IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol = MQTT_Protocol;
void setup()
{
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    digitalWrite(LED1,HIGH);
    digitalWrite(LED2,HIGH);
    Serial.begin(115200);
    initTime();
    wifi();
     iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, protocol);
    if (iotHubClientHandle == NULL)
    {
        Serial.println("Failed on IoTHubClient_CreateFromConnectionString.");
        while (1);
    }
}

void loop()
{
      if (!messagePending && messageSending)
    {
        char messagePayload[MESSAGE_MAX_LEN];
        readMessage(messageCount, messagePayload);
        sendMessage(iotHubClientHandle, messagePayload);
        delay(INTERVAL);
    }

    IoTHubClient_LL_DoWork(iotHubClientHandle);
    delay(10);
}
// Initiatizing wifi

void wifi()
{

    delay(10);

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Already connected...");
        return;
    }

    // Connect to WiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    digitalWrite(LED1,LOW);
}
void blink(){
  digitalWrite(LED2,LOW);
  delay(100);
  digitalWrite(LED2,HIGH);
  delay(100);
}


void initTime()
{
    time_t epochTime;
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    while (true)
    {
        epochTime = time(NULL);

        if (epochTime == 0)
        {
            Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
            delay(2000);
        }
        else
        {
            Serial.printf("Fetched NTP epoch time is: %lu.\r\n", epochTime);
            break;
        }
    }
}
// Getting the sensor readings and converted to json and stored in 

void readMessage(int messageId, char *payload)
{
    float temperature = random(40.,50);
    float humidity = random(40.,50);
    StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["deviceId"] = DEVICE_ID;
    
    // NAN is not the valid json, change it to NULL
    if (std::isnan(temperature))
    {
        root["temperature"] = NULL;
    }
    else
    {
        root["temperature"] = temperature;
    }

    if (std::isnan(humidity))
    {
        root["humidity"] = NULL;
    }
    else
    {
        root["humidity"] = humidity;
    }

    root.printTo(payload, MESSAGE_MAX_LEN);
}


static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char *buffer)
{
    // Creates a IotHub message from the given array
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char *)buffer, strlen(buffer));

    if (messageHandle == NULL)
    {
        Serial.println("Unable to create a new IoTHubMessage.");
    }
    else
    {
        Serial.printf("Sending message: %s.\r\n", buffer);

        // Asynchronous call to send the message specified by eventMessageHandle. Returns IOTHUB_CLIENT_OK upon success or an error code upon failure.

        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, NULL) != IOTHUB_CLIENT_OK)
        {
            Serial.println("Failed to hand over the message to IoTHubClient.");
        }
        else
        {
            messagePending = true;
            Serial.println("IoTHubClient accepted the message for delivery.");
        }
    // Frees all resources associated with the given message handle.

        IoTHubMessage_Destroy(messageHandle);
    }
}


static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
    if (IOTHUB_CLIENT_CONFIRMATION_OK == result)
    {
        Serial.println("Message sent to Azure IoT Hub");
    }
    else
    {
        Serial.println("Failed to send message to Azure IoT Hub");
    }
    messagePending = false;
    blink();
}
