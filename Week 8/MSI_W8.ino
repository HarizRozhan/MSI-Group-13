#include "BluetoothSerial.h"
#include <DHT.h>
BluetoothSerial SerialBT;
#define DHTPIN 22    
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
const int LED = 12;
char command;
void setup() 
{
  Serial.begin(9600);                   
  SerialBT.begin("Group 13");         
  Serial.println("Bluetooth initialized");
  pinMode(LED,OUTPUT);
  dht.begin();
}

void loop() 
{
  if(SerialBT.available())
  {
    command = SerialBT.read();
    Serial.print("Command received: ");
    Serial.println(command);

    switch (command)
    {
      case 'ON':
        digitalWrite(LED,HIGH);break;
      case 'OFF':
        digitalWrite(LED,LOW);break;
    }

    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();
    if (!isnan(temp) && !isnan(hum)) 
    {
      SerialBT.print("Temp: ");
      SerialBT.print(temp);
      SerialBT.print(" °C | Hum: ");
      SerialBT.print(hum);
      SerialBT.println(" %");
      Serial.print("Temp: ");
      Serial.print(temp);
      Serial.print(" °C | Hum: ");
      Serial.print(hum);
      Serial.println(" %");
    }
    else {
      Serial.println("Failed to read from DHT11 sensor!");
    }
  }
  delay(2000);
}