// Om Namo Narayana

#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "8====D"
#define WIFI_PASSWORD "coffeebread123"

// Change this and we are FUCKED
#define API_KEY "AIzaSyALoU_0xKcg2vyvX8N2G38tjX06Pq0fha0"
#define DATABASE_URL "asl-interpreter-glove-default-rtdb.firebaseio.com"

const int F1 = A0; 
const int F2 = A1; 
const int F3 = A2; 
const int F4 = A3; 
const int F5 = A4; 

int f_volt1;
int f_volt2;
int f_volt3;
int f_volt4;
int f_volt5;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String uid;
String databasePath;

unsigned long sendDataPrevMillis = 0;

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(A5));

  // same ass boring wifi connection shii
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ESP is in");
    uid = auth.token.uid.c_str();
  } else {
    Serial.printf("ESP didn't get connected. The fuckup is here:", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback; 

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  databasePath = "/flex_sensors_Right";
}

void loop() {
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 2500 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    
    f_volt1 = analogRead(F1);
    f_volt2 = analogRead(F2);
    f_volt3 = analogRead(F3);
    f_volt4 = analogRead(F4);
    f_volt5 = analogRead(F5);

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
    
    FirebaseJson json;

    // This is the same testing code from
    // some of the other files.
    // Remove this while using sensors

    json.set("finger1", f_volt1);
    json.set("finger2", f_volt2);
    json.set("finger3", f_volt3);
    json.set("finger4", f_volt4);
    json.set("finger5", f_volt5);

    //Testing code ends here

    // Don't mess with this part of the code.
    // It is something i scouted out from github
    // I still don't completely understand what's going on here.
    // For some reason, line 127 and 128 is necessary, no idea why

    String latestPath = databasePath;
    // String historyPath = databasePath + "/history/" + String(millis());

    if (Firebase.RTDB.setJSON(&fbdo, latestPath.c_str(), &json)) {
      Serial.println("GGWP");
    } else {
      Serial.println("Kal");
      Serial.println(": " + fbdo.errorReason());
    }
    
  }
}