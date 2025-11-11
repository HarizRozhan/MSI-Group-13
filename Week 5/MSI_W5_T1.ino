// Run 2 DC Motors (Motor A & B) in sync on L298P shield

// Motor A pins
int dirA1 = 12;
int dirA2 = 13;
int pwmA  = 10;



// Motor B pins
int dirB1 = 8;
int dirB2 = 9;
int pwmB  = 11;

void setup() {
  pinMode(dirA1, OUTPUT);
  pinMode(dirA2, OUTPUT);
  pinMode(pwmA, OUTPUT);

  pinMode(dirB1, OUTPUT);
  pinMode(dirB2, OUTPUT);
  pinMode(pwmB, OUTPUT);

}

void loop() {
  // === Forward ===
  digitalWrite(dirA1, HIGH);
  digitalWrite(dirA2, LOW);
  digitalWrite(dirB1, HIGH);
  digitalWrite(dirB2, LOW);
  
  analogWrite(pwmA, 255);  // Speed (0–255)
  analogWrite(pwmB, 255);
  delay(2000);

  // === Stop ===
  analogWrite(pwmA, 0);
  analogWrite(pwmB, 0);
  delay(1000);

  // === Reverse ===
  digitalWrite(dirA1, LOW);
  digitalWrite(dirA2, HIGH);
  digitalWrite(dirB1, LOW);
  digitalWrite(dirB2, HIGH);
  
  analogWrite(pwmA, 255);
  analogWrite(pwmB, 255);
  delay(2000);

  // === Stop again ===
  analogWrite(pwmA, 0);
  analogWrite(pwmB, 0);
  delay(1000);
}