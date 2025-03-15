#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Provide the token generation process info
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions
#include "addons/RTDBHelper.h"

// Define WiFi credentials
#define WIFI_SSID "8====D"
#define WIFI_PASSWORD "coffeebread123"

// Insert Firebase project API Key
#define API_KEY "AIzaSyALoU_0xKcg2vyvX8N2G38tjX06Pq0fha0"

// Insert RTDB URL
#define DATABASE_URL "asl-interpreter-glove-default-rtdb.firebaseio.com"

// Define finger sensor pins
const int F1 = A0; // VP (ADC0)
const int F2 = A1; // VN (ADC3)
const int F3 = A2; // (ADC6)
const int F4 = A3; // (ADC7)
const int F5 = A4; // (ADC4)

// Variables to store sensor values
int f_volt1;
int f_volt2;
int f_volt3;
int f_volt4;
int f_volt5;

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database path
String databasePath;

// Current timestamp
unsigned long sendDataPrevMillis = 0;

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(A5));

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  
  // Configure ADC
  analogReadResolution(12); // ESP32 has 12-bit ADC resolution (0-4095)
  
  // Assign the API key
  config.api_key = API_KEY;

  // Assign the RTDB URL
  config.database_url = DATABASE_URL;

  // Sign up as anonymous user
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Authentication successful");
    uid = auth.token.uid.c_str();
  } else {
    Serial.printf("Authentication failed: %s\n", config.signer.signupError.message.c_str());
  }

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; // See addons/TokenHelper.h

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Database path for finger sensor data
  databasePath = "/finger_sensors";
}

void loop() {
  // Only send data if Firebase is ready and if 2.5 seconds has passed
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 2500 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    
    // Read sensor values
    f_volt1 = analogRead(F1);
    f_volt2 = analogRead(F2);
    f_volt3 = analogRead(F3);
    f_volt4 = analogRead(F4);
    f_volt5 = analogRead(F5);

    // Print values to Serial Monitor
    Serial.print("Finger 1: ");
    Serial.print(f_volt1);
    Serial.print("\t"); 

    Serial.print("Finger 2: ");
    Serial.print(f_volt2);
    Serial.print("\t");

    Serial.print("Finger 3: ");
    Serial.print(f_volt3);
    Serial.print("\t");

    Serial.print("Finger 4: ");
    Serial.print(f_volt4);
    Serial.print("\t");

    Serial.print("Finger 5: ");
    Serial.print(f_volt5);
    Serial.println();
    
    f_volt1 = random(1024);
    f_volt2 = random(1024);
    f_volt3 = random(1024);
    f_volt4 = random(1024);
    f_volt5 = random(1024);
    // Create paths for data
    String latestPath = databasePath + "/latest";
    
    // Create FirebaseJson object
    FirebaseJson json;
    json.set("finger1", f_volt1);
    json.set("finger2", f_volt2);
    json.set("finger3", f_volt3);
    json.set("finger4", f_volt4);
    json.set("finger5", f_volt5);
    
    // Send latest data to Firebase
    if (Firebase.RTDB.setJSON(&fbdo, latestPath.c_str(), &json)) {
      Serial.println("Latest data sent successfully");
    } else {
      Serial.println("Failed to send latest data");
      Serial.println("Reason: " + fbdo.errorReason());
    }
    
    // Send history data to Firebase
    // if (Firebase.RTDB.setJSON(&fbdo, historyPath.c_str(), &json)) {
    //   Serial.println("History data sent successfully");
    // } else {
    //   Serial.println("Failed to send history data");
    //   Serial.println("Reason: " + fbdo.errorReason());
    // }
  }
}