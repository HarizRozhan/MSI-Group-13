#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <Wire.h>
#include <MPU6050.h>
#include <HUSKYLENS.h>

// WiFi credentials
const char* ssid = "rolinkano";
const char* password = "leraumanton";

// Telegram Bot
#define BOTtoken "8537073557:AAHSyMzeRSozw4lv1Z2aWyW5VxdzirJjr7w"
#define CHAT_ID "1187788663"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// MPU6050 - I2C Bus 1
#define SDA_PIN_MPU 33
#define SCL_PIN_MPU 32
TwoWire I2C_MPU = TwoWire(1);      // Declare I2C bus FIRST
MPU6050 mpu(0x68, &I2C_MPU);       // THEN create MPU object using it

// HuskyLens - I2C Bus 0 (CORRECTED PINS - SWAPPED!)
#define SDA_PIN_HUSKY 22  // CHANGED FROM 23
#define SCL_PIN_HUSKY 23  // CHANGED FROM 22
HUSKYLENS huskylens;

// Fall detection thresholds
const float FALL_THRESHOLD = 2.5;
const float FREE_FALL_THRESHOLD = 0.5;
const unsigned long FALL_COOLDOWN = 10000;

bool fallDetected = false;
unsigned long lastFallTime = 0;
bool mpuWorking = false;
bool huskylensWorking = false;

// Panic Button
const int buttonPin = 25;
bool lastStableState = HIGH;
bool lastReading = HIGH;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Location coordinates
float latitude = 3.253454;
float longitude = 101.732513;

// Ultrasonic sensors
#define TRIG_PIN_LEFT 21
#define ECHO_PIN_LEFT 19
#define TRIG_PIN_RIGHT 5
#define ECHO_PIN_RIGHT 18
#define TRIG_PIN_FRONT 26
#define ECHO_PIN_FRONT 27

// LDR (on Master ESP32)
#define LDR_PIN 34

// ESP-NOW slave address
uint8_t slaveAddress[] = {0x4C, 0xC3, 0x82, 0x0D, 0x11, 0x6C};

struct DataPacket {
  uint8_t leftBuzzer;
  uint8_t rightBuzzer;
  uint8_t ledControl;
  uint8_t colorSound;
};

DataPacket data = {0, 0, 0, 0};

// Color detection timing
unsigned long lastColorCheckTime = 0;
const unsigned long COLOR_CHECK_INTERVAL = 500;
uint8_t lastDetectedColor = 0;

long getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

uint8_t calculateBuzzerSpeed(long distance) {
  if (distance < 0 || distance >= 150) {
    return 0;
  }
  
  if (distance >= 135) return 1;
  else if (distance >= 120) return 2;
  else if (distance >= 105) return 3;
  else if (distance >= 90) return 4;
  else if (distance >= 75) return 5;
  else if (distance >= 60) return 6;
  else if (distance >= 45) return 7;
  else if (distance >= 30) return 8;
  else if (distance >= 15) return 9;
  else return 10;
}

void controlLEDs() {
  int ldrValue = analogRead(LDR_PIN);
  
  Serial.print("LDR Value: ");
  Serial.print(ldrValue);
  
  if (ldrValue < 500) {
    data.ledControl = 1;
    Serial.println(" - DARK (LEDs ON)");
  } else if (ldrValue >= 2000) {
    data.ledControl = 0;
    Serial.println(" - BRIGHT (LEDs OFF)");
  } else {
    data.ledControl = 0;
    Serial.println(" - MEDIUM (LEDs OFF)");
  }
}

void detectColorAndPlaySound() {
  if (!huskylensWorking) {
    Serial.println("⚠️ HuskyLens not working - skipping color detection");
    return;
  }
  
  unsigned long currentTime = millis();
  if (currentTime - lastColorCheckTime < COLOR_CHECK_INTERVAL) {
    return;
  }
  lastColorCheckTime = currentTime;
  
  Serial.println("🔍 Checking HuskyLens...");
  
  if (!huskylens.request()) {
    Serial.println("⚠️ HuskyLens request failed - no data available");
    data.colorSound = 0;
    lastDetectedColor = 0;
    return;
  }
  
  Serial.println("✓ HuskyLens request successful");
  
  if (!huskylens.isLearned()) {
    Serial.println("⚠️ HuskyLens has no learned colors");
    data.colorSound = 0;
    lastDetectedColor = 0;
    return;
  }
  
  Serial.println("✓ HuskyLens has learned colors");
  
  if (!huskylens.available()) {
    Serial.println("⚠️ No color blocks available");
    data.colorSound = 0;
    lastDetectedColor = 0;
    return;
  }
  
  Serial.println("✓ Color blocks available");
  
  HUSKYLENSResult result = huskylens.read();
  
  Serial.print("📦 Block command: ");
  Serial.println(result.command);
  
  if (result.command == COMMAND_RETURN_BLOCK) {
    int colorID = result.ID;
    
    Serial.print("🔢 Raw Color ID received: ");
    Serial.println(colorID);
    
    if (colorID >= 1 && colorID <= 4) {
      
      Serial.print("🎨 Color detected - ID: ");
      Serial.print(colorID);
      Serial.print(" | ");
      
      switch(colorID) {
        case 1: Serial.print("RED"); break;
        case 2: Serial.print("GREEN"); break;
        case 3: Serial.print("BLUE"); break;
        case 4: Serial.print("YELLOW"); break;
      }
      
      if (colorID != lastDetectedColor) {
        lastDetectedColor = colorID;
        data.colorSound = colorID;
        Serial.println(" - 🔊 SENDING SOUND");
      } else {
        data.colorSound = 0;
        Serial.println(" - (same color, no sound)");
      }
      
    } else {
      Serial.print("⚠️ Invalid color ID detected: ");
      Serial.println(colorID);
      data.colorSound = 0;
      lastDetectedColor = 0;
    }
  } else {
    Serial.println("⚠️ Not a block command");
    data.colorSound = 0;
    lastDetectedColor = 0;
  }
}

void sendDistressAlert() {
  Serial.println("Sending distress alert...");
  
  bool messageSent = bot.sendMessage(CHAT_ID, "🚨 ALERT: Person in distress! 🚨\nEmergency button has been pressed!", "");
  
  if (messageSent) {
    Serial.println("✓ Distress alert message sent!");
  } else {
    Serial.println("✗ Failed to send distress alert message");
  }
  
  delay(1000);
  
  Serial.println("Sending location...");
  
  String url = "https://api.telegram.org/bot";
  url += BOTtoken;
  url += "/sendLocation?chat_id=";
  url += CHAT_ID;
  url += "&latitude=";
  url += String(latitude, 6);
  url += "&longitude=";
  url += String(longitude, 6);

  WiFiClientSecure telegramClient;
  telegramClient.setInsecure();
  
  if (telegramClient.connect("api.telegram.org", 443)) {
    telegramClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: api.telegram.org\r\n" +
                 "User-Agent: ESP32\r\n" +
                 "Connection: close\r\n\r\n");
    
    Serial.println("✓ Location sent to Telegram!");
    
    while (telegramClient.connected()) {
      String line = telegramClient.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }
    telegramClient.stop();
  } else {
    Serial.println("✗ Connection to Telegram failed for location!");
  }
}

void sendFallAlert() {
  Serial.println("Sending fall detection alert...");
  
  bool messageSent = bot.sendMessage(CHAT_ID, "🚨 FALL DETECTED! 🚨\nPerson may have fallen down!", "");
  
  if (messageSent) {
    Serial.println("✓ Fall alert message sent!");
  } else {
    Serial.println("✗ Failed to send fall alert message");
  }
  
  delay(1000);
  
  Serial.println("Sending location...");
  
  String url = "https://api.telegram.org/bot";
  url += BOTtoken;
  url += "/sendLocation?chat_id=";
  url += CHAT_ID;
  url += "&latitude=";
  url += String(latitude, 6);
  url += "&longitude=";
  url += String(longitude, 6);

  WiFiClientSecure telegramClient;
  telegramClient.setInsecure();
  
  if (telegramClient.connect("api.telegram.org", 443)) {
    telegramClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: api.telegram.org\r\n" +
                 "User-Agent: ESP32\r\n" +
                 "Connection: close\r\n\r\n");
    
    Serial.println("✓ Fall location sent to Telegram!");
    
    while (telegramClient.connected()) {
      String line = telegramClient.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }
    telegramClient.stop();
  } else {
    Serial.println("✗ Connection to Telegram failed for location!");
  }
}

void detectFall() {
  if (!mpuWorking) return;
  
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  
  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;
  
  float totalAccel = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);
  
  if (totalAccel > FALL_THRESHOLD || totalAccel < FREE_FALL_THRESHOLD) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastFallTime > FALL_COOLDOWN) {
      fallDetected = true;
      lastFallTime = currentTime;
      
      Serial.println("⚠️ FALL DETECTED!");
      Serial.print("Total Accel: ");
      Serial.print(totalAccel);
      Serial.println(" g");
      
      sendFallAlert();
      
      data.leftBuzzer = 10;
      data.rightBuzzer = 10;
      esp_now_send(slaveAddress, (uint8_t *)&data, sizeof(data));
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\n========== STARTING SETUP ==========");
  
  // Setup panic button
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.println("✓ Button configured");
  
  // Setup LDR
  pinMode(LDR_PIN, INPUT);
  Serial.println("✓ LDR configured on GPIO 34 (Master)");
  
  // Setup ultrasonic sensors
  pinMode(TRIG_PIN_LEFT, OUTPUT);
  pinMode(ECHO_PIN_LEFT, INPUT);
  pinMode(TRIG_PIN_RIGHT, OUTPUT);
  pinMode(ECHO_PIN_RIGHT, INPUT);
  pinMode(TRIG_PIN_FRONT, OUTPUT);
  pinMode(ECHO_PIN_FRONT, INPUT);
  Serial.println("✓ Ultrasonic sensors configured (Left, Right, Front)");
  
  // In your setup() function, replace the MPU initialization section with this:

// Initialize I2C Bus 1 for MPU6050 - ROBUST INITIALIZATION
Serial.println("Initializing I2C Bus 1 for MPU6050...");
I2C_MPU.begin(SDA_PIN_MPU, SCL_PIN_MPU, 100000);  // Start at 100kHz for stability
delay(1000);  // Longer delay for stability

// Initialize MPU6050 with retry logic
Serial.println("Initializing MPU6050...");
int mpuAttempts = 0;
bool mpuConnected = false;

// Set the MPU6050 to use I2C_MPU (Bus 1) instead of Wire (Bus 0)


while (mpuAttempts < 5 && !mpuConnected) {
  Serial.print("Attempt ");
  Serial.print(mpuAttempts + 1);
  Serial.print("/5... ");
  
  mpu.initialize();
  delay(500);
  
  if (mpu.testConnection()) {
    mpuConnected = true;
    Serial.println("SUCCESS!");
  } else {
    Serial.println("Failed, retrying...");
    delay(1000);
    mpuAttempts++;
  }
}

if (mpuConnected) {
  Serial.println("✓ MPU6050 connected successfully on I2C Bus 1");
  Serial.print("  SDA: GPIO ");
  Serial.print(SDA_PIN_MPU);
  Serial.print(" | SCL: GPIO ");
  Serial.println(SCL_PIN_MPU);
  
  // Increase I2C speed after successful connection
  I2C_MPU.setClock(400000);  // Now safe to use 400kHz
  Serial.println("  I2C speed set to 400kHz");
  
  mpuWorking = true;
} else {
  Serial.println("✗ MPU6050 connection failed after 5 attempts");
  Serial.println("  Continuing without fall detection");
  mpuWorking = false;
}
  
  // Initialize I2C Bus 0 for HuskyLens (CORRECTED PINS!)
  Serial.println("\nInitializing I2C Bus 0 for HuskyLens...");
  Wire.begin(SDA_PIN_HUSKY, SCL_PIN_HUSKY);
  Wire.setClock(100000);  // 100kHz for HuskyLens (slower, more reliable)
  delay(1000);  // Longer delay for stability
  
  // Initialize HuskyLens with retry logic
  Serial.println("Initializing HuskyLens...");
  
  int huskyAttempts = 0;
  bool huskyConnected = false;
  
  while (huskyAttempts < 5 && !huskyConnected) {
    Serial.print("Attempt ");
    Serial.print(huskyAttempts + 1);
    Serial.print("/5... ");
    
    if (huskylens.begin(Wire)) {
      huskyConnected = true;
      Serial.println("SUCCESS!");
    } else {
      Serial.println("Failed, retrying...");
      delay(1000);
      huskyAttempts++;
    }
  }
  
  if (huskyConnected) {
    Serial.println("✓ HuskyLens connected successfully on I2C Bus 0");
    Serial.print("  SDA: GPIO ");
    Serial.print(SDA_PIN_HUSKY);
    Serial.print(" | SCL: GPIO ");
    Serial.println(SCL_PIN_HUSKY);
    
    delay(500);
    
    // Set to Color Recognition mode
    Serial.println("Setting Color Recognition mode...");
    huskylens.writeAlgorithm(ALGORITHM_COLOR_RECOGNITION);
    Serial.println("✓ HuskyLens set to Color Recognition mode");
    huskylensWorking = true;
    
  } else {
    Serial.println("✗ HuskyLens connection failed - continuing without color detection");
    Serial.println("  Check wiring:");
    Serial.print("    SDA should be on GPIO ");
    Serial.println(SDA_PIN_HUSKY);
    Serial.print("    SCL should be on GPIO ");
    Serial.println(SCL_PIN_HUSKY);
    huskylensWorking = false;
  }
  
  // Connect to WiFi
  Serial.print("\nConnecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    // Initialize Telegram
    client.setInsecure();
    
    Serial.println("Sending startup message to Telegram...");
    bool startupSent = bot.sendMessage(CHAT_ID, "✅ ESP32 ready for emergency button, fall detection, and color recognition", "");
    
    if (startupSent) {
      Serial.println("✓ Startup message sent!");
    } else {
      Serial.println("✗ Failed to send startup message");
    }
  } else {
    Serial.println("\n✗ WiFi connection failed - continuing without Telegram");
  }
  
  // Initialize ESP-NOW
  Serial.println("\nInitializing ESP-NOW...");
  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ ESP-NOW init failed");
  } else {
    Serial.println("✓ ESP-NOW initialized");
    
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, slaveAddress, 6);
    peer.channel = 0;
    peer.encrypt = false;
    
    if (esp_now_add_peer(&peer) == ESP_OK) {
      Serial.println("✓ Slave added successfully");
    } else {
      Serial.println("✗ Failed to add slave");
    }
  }
  
  // Initialize button state
  lastStableState = digitalRead(buttonPin);
  lastReading = lastStableState;
  
  Serial.println("\n========== SETUP COMPLETE ==========");
  Serial.println("\nI2C Configuration:");
  Serial.println("  Bus 0 (HuskyLens): SDA=GPIO22, SCL=GPIO23");
  Serial.println("  Bus 1 (MPU6050):   SDA=GPIO33, SCL=GPIO32");
  Serial.println("\nHuskyLens Color Training Guide:");
  Serial.println("  ID 1 = RED");
  Serial.println("  ID 2 = GREEN");
  Serial.println("  ID 3 = BLUE");
  Serial.println("  ID 4 = YELLOW\n");
}

void loop() {
  // ==================== PANIC BUTTON LOGIC ====================
  int reading = digitalRead(buttonPin);

  if (reading != lastReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != lastStableState) {
      lastStableState = reading;
      
      if (lastStableState == LOW) {
        Serial.println("🔴 Button pressed - sending distress alert");
        sendDistressAlert();
      }
    }
  }

  lastReading = reading;

  // ==================== FALL DETECTION LOGIC ====================
  detectFall();

  // ==================== COLOR DETECTION LOGIC ====================
  detectColorAndPlaySound();

  // ==================== LDR AND LED CONTROL ====================
  controlLEDs();

  // ==================== ULTRASONIC SENSOR LOGIC ====================
  long distanceLeft = getDistance(TRIG_PIN_LEFT, ECHO_PIN_LEFT);
  long distanceRight = getDistance(TRIG_PIN_RIGHT, ECHO_PIN_RIGHT);
  long distanceFront = getDistance(TRIG_PIN_FRONT, ECHO_PIN_FRONT);

  uint8_t leftSpeed = calculateBuzzerSpeed(distanceLeft);
  uint8_t rightSpeed = calculateBuzzerSpeed(distanceRight);
  uint8_t frontSpeed = calculateBuzzerSpeed(distanceFront);

  data.leftBuzzer = max(leftSpeed, frontSpeed);
  data.rightBuzzer = max(rightSpeed, frontSpeed);

  // Debug output
  Serial.println("=================================");
  Serial.print("LEFT  - Distance: ");
  Serial.print(distanceLeft);
  Serial.print(" cm | Speed: ");
  Serial.println(leftSpeed);
  
  Serial.print("RIGHT - Distance: ");
  Serial.print(distanceRight);
  Serial.print(" cm | Speed: ");
  Serial.println(rightSpeed);
  
  Serial.print("FRONT - Distance: ");
  Serial.print(distanceFront);
  Serial.print(" cm | Speed: ");
  Serial.println(frontSpeed);
  
  Serial.println("---------------------------------");
  Serial.print("📤 Sending: Left Buzzer = ");
  Serial.print(data.leftBuzzer);
  Serial.print(" | Right Buzzer = ");
  Serial.print(data.rightBuzzer);
  Serial.print(" | LED = ");
  Serial.print(data.ledControl);
  Serial.print(" | Color Sound = ");
  Serial.println(data.colorSound);

  esp_err_t result = esp_now_send(slaveAddress, (uint8_t *)&data, sizeof(data));
  
  if (result == ESP_OK) {
    Serial.println("✓ Data sent successfully");
  } else {
    Serial.println("✗ Error sending data");
  }
  Serial.println("=================================\n");

  delay(100);
}