#include <ESP32Servo.h>

#define BUTTON_PIN 2
#define SERVO_PIN 12

Servo myservo;

int servoPos = 0;           // Current servo angle
bool movingForward = true;  // Direction of movement

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Fast Bounce Servo Test Start");

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  myservo.setPeriodHertz(50);
  myservo.attach(SERVO_PIN, 1000, 2000);

  myservo.write(servoPos);
}

void loop() {
  bool state = digitalRead(BUTTON_PIN);

  if (state == LOW) {           // Button pressed
    // Move servo faster (e.g., 5° per loop)
    if (movingForward) {
      servoPos += 5;
      if (servoPos >= 180) {
        servoPos = 180;
        movingForward = false;  // Reverse direction at max
      }
    } else {
      servoPos -= 5;
      if (servoPos <= 0) {
        servoPos = 0;
        movingForward = true;   // Reverse direction at min
      }
    }

    myservo.write(servoPos);
    Serial.print("Servo angle: ");
    Serial.println(servoPos);
    delay(50); // Small delay for smooth movement
  }
}
