const int sensorPin = 5;     // the number of the Sensor pin (D1)

void setup() {
  Serial.begin(9600);
  // initialize the Sensor pin as an input
  pinMode(sensorPin, INPUT);
}

void loop() {
  // read the state of the sensor value
  Serial.println(digitalRead(sensorPin));
  delay(1000);
}