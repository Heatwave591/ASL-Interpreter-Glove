#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "8====D"
#define WIFI_PASSWORD "coffeebread123"

#define API_KEY "AIzaSyALoU_0xKcg2vyvX8N2G38tjX06Pq0fha0"
#define DATABASE_URL "https://asl-interpreter-glove-default-rtdb.firebaseio.com/" 

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

float randomNumbers[5];

void setup(){
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback; 
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  randomSeed(analogRead(0));
}

void generateRandomNumbers() {
  for (int i = 0; i < 5; i++) {
    randomNumbers[i] = 0.01 + random(0, 100);
  }
}

void loop(){
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    
    generateRandomNumbers();
    
    Serial.println("Generated random numbers:");
    for (int i = 0; i < 5; i++) {
      Serial.print("Number ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(randomNumbers[i]);
    }
    
    for (int i = 0; i < 5; i++) {
      String path = "randomNumbers/number" + String(i + 1);
      
      if (Firebase.RTDB.setFloat(&fbdo, path, randomNumbers[i])) {
        Serial.print("PASSED for number ");
        Serial.println(i + 1);
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else {
        Serial.print("FAILED for number ");
        Serial.println(i + 1);
        Serial.println("REASON: " + fbdo.errorReason());
      }
    }
    
    FirebaseJson json;
    for (int i = 0; i < 5; i++) {
      json.set(String(i), randomNumbers[i]);
    }
    
    if (Firebase.RTDB.setJSON(&fbdo, "randomNumbers/all", &json)) {
      Serial.println("PASSED storing all numbers as array");
    }
    else {
      Serial.println("FAILED storing all numbers as array");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    
    // Keep the counter for reference
    if (Firebase.RTDB.setInt(&fbdo, "test/int", count)){
      Serial.println("Counter updated");
    }
    count++;
  }
}