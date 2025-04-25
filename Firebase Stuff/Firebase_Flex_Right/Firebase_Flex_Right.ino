// Om Namo Narayana

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;
#define WIFI_SSID "SSID here"
#define WIFI_PASSWORD "Wifi password here"

//Change this and we are FUCKED
#define API_KEY "API Key here"
#define DATABASE_URL "Database URL here"

const int F1 = 37; 
const int F2 = 33; 
const int F3 = A2; 
const int F4 = A3; 
const int F5 = A4; 

int f_volt1, f_volt2, f_volt3, f_volt4, f_volt5;
float gyroX, gyroY, gyroZ, accX, accY, accZ, accNet, tempr;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String uid;
String databasePath;
String sign = "-";

unsigned long sendDataPrevMillis = 0;

struct FingerRange {
  int minVal[5];
  int maxVal[5];
  char letter;
};

FingerRange letterModels[] = {
  {{100, 117, 93, 107, 0}, {2906, 3042, 2415, 2295, 0}, 'a'},
  {{0, 0, 0, 0, 500}, {0, 199, 0, 0, 2500}, 'b'},
  {{0, 0, 0, 205, 0}, {0, 0, 0, 1500, 0}, 'f'},
  {{150, 150, 135, 0, 0}, {2000, 2467, 1899, 0, 0}, 'g'},
  {{0, 377, 402, 169, 117}, {0, 2615, 2037, 2309, 1238}, 'i'},
  {{23, 441, 194, 13, 100}, {1847, 2023, 1628, 1652, 1863}, 'o'},
  {{48, 500, 0, 0, 300}, {2134, 2134, 0, 0, 1600}, 'v'},
  {{0, 63, 13, 45, 0}, {0, 1961, 1550, 1677, 0}, 'y'},
  {{0, 100, 100, 0, 0}, {0, 2726, 2632, 0, 0}, 'k'},   //ILY
  {{0, 0, 0, 0, 10}, {0, 0, 0, 0, 500}, 'l'},   //fine
  {{100, 107, 93, 0, 30}, {2906, 3000, 2415, 0, 3042}, 'm'} //point
};


const int letter = sizeof(mapping) / sizeof(mapping[0]);

char inferLetterFromRange(int pinky, int ring, int middle, int index, int thumb) {
  for (int i = 0; i < letter; i++) {
    bool match = true;
    if (pinky  < mapping[i].minVal[0] || pinky  > mapping[i].maxVal[0]) match = false;
    if (ring   < mapping[i].minVal[1] || ring   > mapping[i].maxVal[1]) match = false;
    if (middle < mapping[i].minVal[2] || middle > mapping[i].maxVal[2]) match = false;
    if (index  < mapping[i].minVal[3] || index  > mapping[i].maxVal[3]) match = false;
    if (thumb  < mapping[i].minVal[4] || thumb  > mapping[i].maxVal[4]) match = false;
    if (match) return mapping[i].letter;
  }
  return '-';
}

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
  
  // Gyroscope searching 
  // if (!mpu.begin()) {
  //   Serial.println("Failed to find MPU6050 chip");
  //   while (1) {
  //     delay(10);
  //   }
  // }

  Serial.println("SUIIIIIIII");

  // MPU6050 setup
  // Do not change this snippet
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    uid = auth.token.uid.c_str();
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

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    gyroX = g.gyro.x;
    gyroY = g.gyro.y;
    gyroZ = g.gyro.z;
    accX = a.acceleration.x;
    accY = a.acceleration.y;
    accZ = a.acceleration.z;
    accNet = sqrt(sq(accX) + sq(accY) + sq(accZ));
    tempr = temp.temperature;

    // Predict character based on finger range logic
    char predicted = inferLetterFromRange(f_volt1, f_volt2, f_volt3, f_volt4, f_volt5);
    sign = String(predicted);

    Serial.print("Predicted letter: ");
    Serial.println(predicted);

    FirebaseJson json;
    json.set("finger1", f_volt1);
    json.set("finger2", f_volt2);
    json.set("finger3", f_volt3);
    json.set("finger4", f_volt4);
    json.set("finger5", f_volt5);
    json.set("Gyroscope X", gyroX);
    json.set("Gyroscope Y", gyroY);
    json.set("Gyroscope Z", gyroZ);
    json.set("Net Acceleration", accNet);
    json.set("Tempreture", tempr);
    json.set("What's he Spittin' ", sign);

    String latestPath = databasePath;

    if (Firebase.RTDB.setJSON(&fbdo, latestPath.c_str(), &json)) {
      Serial.println("GGWP");
    } else {
      Serial.println("Kal");
      Serial.println(": " + fbdo.errorReason());
    }
  }
}
