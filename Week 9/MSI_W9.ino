#include <SoftwareSerial.h>
#include <DFRobot_HuskyLens.h>

SoftwareSerial mySerial(4, 5);  // RX, TX
HUSKYLENS huskylens;

// RGB LED pin definitions
const int redPin = 9;
const int greenPin = 10;
const int bluePin = 11;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  // Turn off all colors initially
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
  digitalWrite(bluePin, LOW);

  while (!huskylens.begin(mySerial)) {
    Serial.println("HuskyLens not connected!");
    delay(500);
  }

  Serial.println("HuskyLens Ready");
}

void loop() {
  if (!huskylens.request()) {
    Serial.println("Request failed");
    return;
  }

  if (huskylens.available()) {
    HUSKYLENSResult result = huskylens.read();

    Serial.print("ID: ");
    Serial.print(result.ID);
    Serial.print("  X: ");
    Serial.print(result.xCenter);
    Serial.print("  Y: ");
    Serial.println(result.yCenter);

    // Turn off all LEDs first
    digitalWrite(redPin, LOW);
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin, LOW);

    // Light up based on detected ID
    if (result.ID == 1) {
      digitalWrite(redPin, HIGH);      // RED ON
      Serial.println("→ Showing RED");
    } 
    else if (result.ID == 2) {
      digitalWrite(greenPin, HIGH);    // GREEN ON
      Serial.println("→ Showing GREEN");
    } 
    else if (result.ID == 3) {
      digitalWrite(bluePin, HIGH);     // BLUE ON
      Serial.println("→ Showing BLUE");
    }

    delay(1000);
  }
}
