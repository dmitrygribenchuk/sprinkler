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
#define SOIL_MOISTURE_SENSOR_1_TOPIC "/sprinkler/sensors/1"
#define SOIL_MOISTURE_SENSOR_2_TOPIC "/sprinkler/sensors/2"
#define SOIL_MOISTURE_SENSOR_3_TOPIC "/sprinkler/sensors/3"

#define SOIL_MOISTURE_SENSOR_1_PIN    D0
#define SOIL_MOISTURE_SENSOR_2_PIN    D1
#define SOIL_MOISTURE_SENSOR_3_PIN    D2
  
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
  pinMode(SOIL_MOISTURE_SENSOR_1_PIN, INPUT);
  pinMode(SOIL_MOISTURE_SENSOR_2_PIN, INPUT);
  pinMode(SOIL_MOISTURE_SENSOR_3_PIN, INPUT);
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

uint32_t x=0;
long lastMsg = 0;
long sensorsDelay = 60000;
int sensor1Value = 0; // need to use array
int sensor2Value = 0;
int sensor3Value = 0;
int pompDelay = 10000;

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
        //need to use loop as well
        //read and send from 1 sensor
        sensor1Value = digitalRead(SOIL_MOISTURE_SENSOR_1_PIN);
        Serial.print("Send data from sensor 1: ");
        Serial.println(sensor1Value);
        mqtt.publish(SOIL_MOISTURE_SENSOR_1_TOPIC, String(sensor1Value).c_str(), true);

        //read and send from 2 sensor
        sensor2Value = digitalRead(SOIL_MOISTURE_SENSOR_2_PIN);
        Serial.print("Send data from sensor 2: ");
        Serial.println(sensor2Value);
        mqtt.publish(SOIL_MOISTURE_SENSOR_2_TOPIC, String(sensor2Value).c_str(), true);

        //read and send from 3 sensor
        sensor3Value = digitalRead(SOIL_MOISTURE_SENSOR_3_PIN);
        Serial.print("Send data from sensor 3: ");
        Serial.println(sensor3Value);
        mqtt.publish(SOIL_MOISTURE_SENSOR_3_TOPIC, String(sensor3Value).c_str(), true);

        lastMsg = now;
  }
 
  while ((subscription = mqtt.readSubscription(1000))) {

    // Check if its the onoff button feed
    if (subscription == &pomp) {
      
      if (strcmp((char *)pomp.lastread, "1") == 0) {
        Serial.println("Enable Water Pomp");
        analogWrite(POMP_PIN, 8);
        delay(pompDelay);
        digitalWrite(POMP_PIN, LOW);
        Serial.println("Disable Water Pomp");
        mqtt.publish(POMP_TOPIC, String(0).c_str(), true);
      }
    }
  }

  // ping the server to keep the mqtt connection alive
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
}
