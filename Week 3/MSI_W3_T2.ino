#include <Servo.h> // Include the Servo library to control the motor

// Define the pins
const int POT_PIN = A0;    // Potentiometer connected to Analog Pin A0
const int LED_PIN = 11;    // LED connected to Digital Pin 11
const int SERVO_PIN = 9;   // Servo signal wire connected to Digital Pin 9 (often a PWM pin)

// Create a Servo object
Servo myServo;

void setup() {
  Serial.begin(9600);
  
  // Set the LED pin as an output
  pinMode(LED_PIN, OUTPUT); 

  // Attach the servo object to the specified pin
  myServo.attach(SERVO_PIN);
}

void loop() {
  // 1. Read the analog value from the potentiometer (0 to 1023)
  int potValue = analogRead(POT_PIN); 
  
  // Print the value to the Serial Monitor (optional, but helpful for debugging)
  Serial.println(potValue);
  
  // 2. Map the potentiometer value (0-1023) to the servo angle range (0-180)
  // The map() function is essential for scaling the input to the output.
  int servoAngle = map(potValue, 0, 1023, 0, 180); 
  
  // 3. Write the calculated angle to the servo motor
  myServo.write(servoAngle); 

  // 4. LED Control Logic (Maintain the original requirement)
  if (potValue > 500) 
  {
    // Turn the LED ON if the potentiometer reading is above 500
    digitalWrite(LED_PIN, HIGH); 
  }
  else
  {
    // Turn the LED OFF otherwise
    digitalWrite(LED_PIN, LOW);
  }
  
  // Small delay to prevent jitter and avoid sending data too fast
  delay(100); 
}