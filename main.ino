#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// the on off button feed turns this LED on/off
#define LED 2  
// the slider feed sets the PWM output of this pin

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "WIFI SSID"
#define WLAN_PASS       "WIFI PASSWORD"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "mqtt.flespi.io"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "FlespiToken TOKEN"
#define AIO_KEY         ""

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, "/sprinkler/pomp");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(D0, INPUT);
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);

  Serial.begin(115200);
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
  mqtt.subscribe(&onoffbutton);
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
        sensor1Value = digitalRead(D0);
        Serial.print("Send data from sensor 1: ");
        Serial.println(sensor1Value);
        mqtt.publish("/sprinkler/sensors/1", String(sensor1Value).c_str(), true);

        //read and send from 2 sensor
        sensor2Value = digitalRead(D1);
        Serial.print("Send data from sensor 2: ");
        Serial.println(sensor2Value);
        mqtt.publish("/sprinkler/sensors/2", String(sensor2Value).c_str(), true);

        //read and send from 3 sensor
        sensor3Value = digitalRead(D2);
        Serial.print("Send data from sensor 3: ");
        Serial.println(sensor3Value);
        mqtt.publish("/sprinkler/sensors/3", String(sensor3Value).c_str(), true);

        lastMsg = now;
  }
 
  while ((subscription = mqtt.readSubscription(1000))) {

    // Check if its the onoff button feed
    if (subscription == &onoffbutton) {
      //Serial.print(F("On-Off button: "));
      //Serial.println((char *)onoffbutton.lastread);
      
      if (strcmp((char *)onoffbutton.lastread, "1") == 0) {
        Serial.println("Enable Water Pomp");
        digitalWrite(LED, HIGH); 
        delay(pompDelay);
        digitalWrite(LED, LOW);
        Serial.println("Disable Water Pomp");
        mqtt.publish("/sprinkler/pomp", String(0).c_str(), true);
      }
    }
  }

  // ping the server to keep the mqtt connection alive
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
}
