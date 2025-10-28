void setup() {
  Serial.begin(9600);
  
  // Set the LED pin (12) as an output
  pinMode(11, OUTPUT); 
}

void loop() {
  // Read the analog value from the potentiometer connected to pin A0
  int potValue = analogRead(A0); 
  
  // Print the value to the Serial Monitor
  Serial.println(potValue);
  
  delay(100); // Wait for 100 milliseconds
  
  // Check if the potentiometer reading is above the threshold (500)
  if (potValue > 500) // The 'if' condition must be in parentheses ( )
  {
    // Turn the LED ON
    digitalWrite(11, HIGH); 
  }
  else
  {
    // Turn the LED OFF
    digitalWrite(11, LOW);
  }
}