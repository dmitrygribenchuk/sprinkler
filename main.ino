#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "WIFI SSID"
#define WLAN_PASS       "WIFI PASSWORD"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "mqtt.flespi.io"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "FlespiToken TOKEN"
#define AIO_KEY         ""

#define POMP_TOPIC                   "/sprinkler/pomp"
#define POMP_PIN                      D5 
#define POMP_POWER		      8

#define SOIL_MOISTURE_SENSOR_1_PIN    D0
#define SOIL_MOISTURE_SENSOR_2_PIN    D1
#define SOIL_MOISTURE_SENSOR_3_PIN    D2

uint32_t x=0;
long lastMsg = 0;
long sensorsDelay = 60000; // check sensors every minute
int sensorsValue[3] = {0,0,0};
String soilMoistureSensorsTopic = "/sprinkler/sensors/";
const char soilMoistureSensorsPin[] = { SOIL_MOISTURE_SENSOR_1_PIN, SOIL_MOISTURE_SENSOR_2_PIN, SOIL_MOISTURE_SENSOR_3_PIN };
int pompDelay = 10000;
  
/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Subscribe pomp = Adafruit_MQTT_Subscribe(&mqtt, POMP_TOPIC);

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).

void setup() {
  for (int i=0; i<=sizeof(soilMoistureSensorsPin); i++) {
    pinMode(soilMoistureSensorsPin[i], INPUT);
  }
  pinMode(POMP_PIN, OUTPUT);

  Serial.begin(9600);
  delay(10);

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff & slider feed.
  mqtt.subscribe(&pomp);
}

void MQTT_connect(Adafruit_MQTT_Client mqtt) {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected())
    return;

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(ret);
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0)
         while (1); // basically die and wait for WDT to reset me
  }

  Serial.println("MQTT Connected!");
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect(mqtt);

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  Adafruit_MQTT_Subscribe *subscription;

  long now = millis();

  if (now - lastMsg > sensorsDelay) {
      for (int i=0; i< sizeof(soilMoistureSensorsPin); i++) {
        //read and send from sensors
        sensorsValue[i] = digitalRead(soilMoistureSensorsPin[i]);
        Serial.println("Send data from sensor " + String(i+1) + ": " + String(sensorsValue[i]));
        mqtt.publish((soilMoistureSensorsTopic + String(i+1)).c_str(), String(sensorsValue[i]).c_str(), true);
      }
      lastMsg = now;
  }
 
  while ((subscription = mqtt.readSubscription(1000))) {

    // Check if its the waterpomp feed
    if (subscription == &pomp) {
      
      if (strcmp((char *)pomp.lastread, "1") == 0) {
        Serial.println("Enable Water Pomp");
        analogWrite(POMP_PIN, POMP_POWER);
        delay(pompDelay);
        digitalWrite(POMP_PIN, LOW);
        Serial.println("Disable Water Pomp by timeout");
        mqtt.publish(POMP_TOPIC, String(0).c_str(), true);
      }
    }
  }

  // ping the server to keep the mqtt connection alive
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
}
