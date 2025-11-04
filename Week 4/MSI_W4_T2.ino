#include <Wire.h>
#include <MPU6050.h>
#include <Servo.h>

MPU6050 mpu;
Servo myServo;

const int GREEN_LED = 11;
const int RED_LED = 9;
const int SERVO_PIN = 10;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1);
  }

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  myServo.attach(SERVO_PIN);
  myServo.write(0);

  Serial.println("System Ready - Waiting for PC...");
}

void loop() {
  // --- Continuously send MPU data for Python to read ---
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  Serial.print("AcX="); Serial.print(ax);
  Serial.print("|AcY="); Serial.print(ay);
  Serial.print("|AcZ="); Serial.println(az);
  delay(50);

  // --- Listen for Python commands ---
  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == '1') {   // Authorized + Motion detected
      Serial.println(" Authorized & Motion detected!");
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      myServo.write(60);
      delay(3000);
      myServo.write(0);
      digitalWrite(GREEN_LED, LOW);
    } 
    else if (cmd == '2') {  // Unauthorized or No motion
      Serial.println(" Access denied!");
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, HIGH);
      myServo.write(0);
      delay(2000);
      digitalWrite(RED_LED, LOW);
    }
  }
}