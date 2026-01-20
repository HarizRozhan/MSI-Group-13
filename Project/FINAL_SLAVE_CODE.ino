#include <WiFi.h>
#include <esp_now.h>

#define BUZZER_LEFT 17      // Left buzzer pin
#define BUZZER_RIGHT 32     // Right buzzer pin
#define LED_PIN 23          // LED pin (all 4 LEDs)
#define SPEAKER_PIN 25      // Speaker pin for color sounds

struct DataPacket {
  uint8_t leftBuzzer;   // 0-10 (0=OFF, 1=slowest, 10=continuous)
  uint8_t rightBuzzer;  // 0-10 (0=OFF, 1=slowest, 10=continuous)
  uint8_t ledControl;   // 0 = OFF, 1 = ON
  uint8_t colorSound;   // 0 = no sound, 1-4 = color codes (RED, GREEN, BLUE, YELLOW)
};

DataPacket receivedData = {0, 0, 0, 0}; // Initialize to OFF

// Buzzer timing arrays (in milliseconds)
int beepIntervals[] = {0, 1000, 900, 800, 700, 600, 500, 400, 300, 200, 0};
int beepDurations[] = {0, 100, 100, 100, 100, 100, 100, 100, 100, 100, 1};

// Variables for non-blocking beeping (separate for each buzzer)
unsigned long lastBeepTimeLeft = 0;
unsigned long lastBeepTimeRight = 0;
bool beepStateLeft = false;
bool beepStateRight = false;

// Debug counters
unsigned long lastDebugPrint = 0;

// Track last played color and timing
uint8_t lastColorPlayed = 0;
unsigned long lastSoundTime = 0;

// Play RED tone - DANGER! Urgent alternating high-pitched warning (3 seconds)
void playRedTone() {
  Serial.println("🔴 DANGER - Alternating urgent warning tones");
  
  // Rapid alternating between two high-pitched frequencies for 3 seconds
  // Pattern: High-Low-High-Low... (creates urgency)
  int highFreq = 988;  // B5 (high pitch)
  int lowFreq = 784;   // G5 (still high but lower)
  int beepCount = 12;  // 12 beeps in 3 seconds
  int beepDuration = 125;  // Each beep 125ms
  int gapDuration = 125;   // Gap between beeps 125ms
  
  for (int i = 0; i < beepCount; i++) {
    tone(SPEAKER_PIN, (i % 2 == 0) ? highFreq : lowFreq);
    delay(beepDuration);
    noTone(SPEAKER_PIN);
    delay(gapDuration);
  }
}

// Play YELLOW tone - GET READY! Rising anticipation (3 seconds)
void playYellowTone() {
  Serial.println("🟡 GET READY - Rising anticipation tones");
  
  // Ascending pattern that builds anticipation
  // Three rising sequences
  int frequencies[] = {523, 587, 659, 698};  // C5, D5, E5, F5
  int noteCount = 4;
  int noteDuration = 200;  // Each note 200ms
  int gapDuration = 50;    // Small gap between notes
  int sequenceCount = 3;   // Repeat 3 times
  
  for (int seq = 0; seq < sequenceCount; seq++) {
    for (int i = 0; i < noteCount; i++) {
      tone(SPEAKER_PIN, frequencies[i]);
      delay(noteDuration);
      noTone(SPEAKER_PIN);
      delay(gapDuration);
    }
  }
}

// Play GREEN tone - GO! Confident ascending progression (3 seconds)
void playGreenTone() {
  Serial.println("🟢 GO - Confident ascending progression");
  
  // Smooth ascending major chord pattern (sounds positive and affirmative)
  // C major scale going up with longer notes for confidence
  int frequencies[] = {523, 587, 659, 698, 784, 880};  // C5, D5, E5, F5, G5, A5
  int noteCount = 6;
  int noteDuration = 400;  // Longer notes for confidence
  int gapDuration = 100;   // Small gap
  
  for (int i = 0; i < noteCount; i++) {
    tone(SPEAKER_PIN, frequencies[i]);
    delay(noteDuration);
    noTone(SPEAKER_PIN);
    if (i < noteCount - 1) {  // No gap after last note
      delay(gapDuration);
    }
  }
}

// Play BLUE tone - INFO/NEUTRAL (3 seconds)
void playBlueTone() {
  Serial.println("🔵 INFO - Steady informational tone");
  
  // Two-tone pattern that's neutral and informative
  int freq1 = 659;  // E5
  int freq2 = 784;  // G5
  
  for (int i = 0; i < 3; i++) {
    tone(SPEAKER_PIN, freq1);
    delay(500);
    noTone(SPEAKER_PIN);
    delay(50);
    tone(SPEAKER_PIN, freq2);
    delay(500);
    noTone(SPEAKER_PIN);
    delay(50);
  }
}

// Main function to play color sound for 3 seconds
void playColorSound(uint8_t colorCode) {
  if (colorCode == 0) {
    return; // No sound requested
  }
  
  // Prevent playing same color too quickly (within 4 seconds to allow 3s play + 1s gap)
  unsigned long currentTime = millis();
  if (colorCode == lastColorPlayed && (currentTime - lastSoundTime < 4000)) {
    Serial.println("⏳ Same color too soon, skipping...");
    return;
  }
  
  lastColorPlayed = colorCode;
  lastSoundTime = currentTime;
  
  String colorName = "";
  
  Serial.println("================================");
  
  switch(colorCode) {
    case 1: // RED - DANGER
      colorName = "RED (DANGER!)";
      Serial.print("🔊🔊🔊 PLAYING: ");
      Serial.println(colorName);
      playRedTone();
      break;
      
    case 2: // GREEN - GO
      colorName = "GREEN (GO!)";
      Serial.print("🔊🔊🔊 PLAYING: ");
      Serial.println(colorName);
      playGreenTone();
      break;
      
    case 3: // BLUE - INFO
      colorName = "BLUE (INFO)";
      Serial.print("🔊🔊🔊 PLAYING: ");
      Serial.println(colorName);
      playBlueTone();
      break;
      
    case 4: // YELLOW - GET READY
      colorName = "YELLOW (GET READY!)";
      Serial.print("🔊🔊🔊 PLAYING: ");
      Serial.println(colorName);
      playYellowTone();
      break;
      
    default:
      Serial.print("❌ Unknown color code: ");
      Serial.println(colorCode);
      Serial.println("================================");
      return;
  }
  
  Serial.println("✓ Sound finished playing (3 seconds complete)");
  Serial.println("================================");
}

void onDataReceived(const esp_now_recv_info *info, const uint8_t *data, int len) {
  if (len == sizeof(receivedData)) {
    memcpy(&receivedData, data, sizeof(receivedData));
    
    Serial.println("================================");
    Serial.print("📥 Received - Left: ");
    Serial.print(receivedData.leftBuzzer);
    Serial.print(" | Right: ");
    Serial.print(receivedData.rightBuzzer);
    Serial.print(" | LED: ");
    Serial.print(receivedData.ledControl ? "ON" : "OFF");
    Serial.print(" | Color: ");
    Serial.println(receivedData.colorSound);
    
    // Control LEDs based on received data
    if (receivedData.ledControl == 1) {
      digitalWrite(LED_PIN, HIGH);  // Turn ON LEDs
      Serial.println("💡 LEDs turned ON");
    } else {
      digitalWrite(LED_PIN, LOW);   // Turn OFF LEDs
    }
    
    // Play color sound if a color was detected
    if (receivedData.colorSound > 0) {
      Serial.print("🎵 Color sound command received: ");
      Serial.println(receivedData.colorSound);
      playColorSound(receivedData.colorSound);
    }
    
    Serial.println("================================");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n========== SLAVE SETUP ==========");
  
  pinMode(BUZZER_LEFT, OUTPUT);
  pinMode(BUZZER_RIGHT, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  
  // Ensure buzzers, LEDs, and speaker start OFF
  digitalWrite(BUZZER_LEFT, LOW);
  digitalWrite(BUZZER_RIGHT, LOW);
  digitalWrite(LED_PIN, LOW);
  noTone(SPEAKER_PIN);
  
  Serial.print("✓ Left buzzer on pin: ");
  Serial.println(BUZZER_LEFT);
  Serial.print("✓ Right buzzer on pin: ");
  Serial.println(BUZZER_RIGHT);
  Serial.print("✓ LED on pin: ");
  Serial.println(LED_PIN);
  Serial.print("✓ Speaker on pin: ");
  Serial.println(SPEAKER_PIN);
  Serial.println("⏱️  Sound duration set to 3 SECONDS");
  Serial.println("🎵 Enhanced color tones:");
  Serial.println("   🔴 RED = DANGER (urgent alternating)");
  Serial.println("   🟡 YELLOW = GET READY (rising anticipation)");
  Serial.println("   🟢 GREEN = GO (confident ascending)");
  Serial.println("   🔵 BLUE = INFO (steady two-tone)");
  
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ Error initializing ESP-NOW");
    return;
  }
  
  Serial.println("✓ ESP-NOW initialized");
  
  // Register for receiving data
  esp_now_register_recv_cb(onDataReceived);
  
  Serial.println("✓ Receiver ready");
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("================================\n");
  
  // Play startup sound (3 beeps) - each 300ms
  Serial.println("Playing startup sound...");
  tone(SPEAKER_PIN, 523);  // C5
  delay(300);
  noTone(SPEAKER_PIN);
  delay(100);
  
  tone(SPEAKER_PIN, 659);  // E5
  delay(300);
  noTone(SPEAKER_PIN);
  delay(100);
  
  tone(SPEAKER_PIN, 784);  // G5
  delay(300);
  noTone(SPEAKER_PIN);
  
  Serial.println("✓ Startup sound complete\n");
}

void handleBuzzer(int pin, uint8_t level, bool &state, unsigned long &lastBeep, String name) {
  if (level == 0) {
    // OFF
    digitalWrite(pin, LOW);
    state = false;
  } else if (level == 10) {
    // CONTINUOUS
    digitalWrite(pin, HIGH);
    state = true;
  } else if (level >= 1 && level <= 9) {
    // BEEPING with varying speeds (level 1-9)
    unsigned long currentTime = millis();
    
    if (state) {
      // Buzzer is ON, check if it's time to turn OFF
      if (currentTime - lastBeep >= beepDurations[level]) {
        digitalWrite(pin, LOW);
        state = false;
        lastBeep = currentTime;
      }
    } else {
      // Buzzer is OFF, check if it's time to turn ON
      if (currentTime - lastBeep >= beepIntervals[level]) {
        digitalWrite(pin, HIGH);
        state = true;
        lastBeep = currentTime;
      }
    }
  }
}

void loop() {
  // Handle each buzzer independently with their own state variables
  handleBuzzer(BUZZER_LEFT, receivedData.leftBuzzer, beepStateLeft, lastBeepTimeLeft, "LEFT");
  handleBuzzer(BUZZER_RIGHT, receivedData.rightBuzzer, beepStateRight, lastBeepTimeRight, "RIGHT");
  
  // Print current status every 2 seconds for debugging
  if (millis() - lastDebugPrint > 2000) {
    Serial.print("Status: Left=");
    Serial.print(receivedData.leftBuzzer);
    Serial.print(" (");
    Serial.print(beepStateLeft ? "ON" : "OFF");
    Serial.print(") | Right=");
    Serial.print(receivedData.rightBuzzer);
    Serial.print(" (");
    Serial.print(beepStateRight ? "ON" : "OFF");
    Serial.print(") | LED=");
    Serial.print(receivedData.ledControl ? "ON" : "OFF");
    Serial.print(" | LastColor=");
    Serial.println(lastColorPlayed);
    lastDebugPrint = millis();
  }
  
  delay(1);
}