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

float gyroX, gyroY, gyroZ;
float accX, accY, accZ, accNet; 
float tempr;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String uid;
String databasePath;
String predictedLetter = "-";

unsigned long sendDataPrevMillis = 0;

// === Character Range Detection Struct ===
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

const int modelCount = sizeof(letterModels) / sizeof(letterModels[0]);

char inferLetterFromRange(int pinky, int ring, int middle, int index, int thumb) {
  for (int i = 0; i < modelCount; i++) {
    bool match = true;
    for (int j = 0; j < 5; j++) {
      int val;
      switch(j) {
        case 0: val = thumb; break;
        case 1: val = index; break;
        case 2: val = middle; break;
        case 3: val = ring; break;
        case 4: val = pinky; break;
      }

      int minVal = letterModels[i].minVal[j];
      int maxVal = letterModels[i].maxVal[j];

      // Add 10% margin below min and 15% margin above max
      int extendedMin = minVal - (minVal * 15) / 100;
      int extendedMax = maxVal + (maxVal * 15) / 100;

      if (val < extendedMin || val > extendedMax) {
        match = false;
        break;
      }
    }

    if (match) return letterModels[i].letter;
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
  
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ESP is in");
    uid = auth.token.uid.c_str();
  } else {
    Serial.printf("ESP didn't get connected. The fuckup is here:", config.signer.signupError.message.c_str());
  }

  // Gyroscope searching 
  // if (!mpu.begin()) {
  //   Serial.println("Failed to find MPU6050 chip");
  //   while (1) {
  //     delay(10);
  //   }
  // }

  // // MPU6050 setup
  // // Do not change this snippet
  // mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  // switch (mpu.getAccelerometerRange()) {
  //   case MPU6050_RANGE_2_G:
  //     Serial.println("+-2G");
  //     break;
  //   case MPU6050_RANGE_4_G:
  //     Serial.println("+-4G");
  //     break;
  //   case MPU6050_RANGE_8_G:
  //     Serial.println("+-8G");
  //     break;
  //   case MPU6050_RANGE_16_G:
  //     Serial.println("+-16G");
  //     break;
  //   }
  //   mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  //   Serial.print("Gyro range set to: ");
  //   switch (mpu.getGyroRange()) {
  //   case MPU6050_RANGE_250_DEG:
  //     Serial.println("+- 250 deg/s");
  //     break;
  //   case MPU6050_RANGE_500_DEG:
  //     Serial.println("+- 500 deg/s");
  //     break;
  //   case MPU6050_RANGE_1000_DEG:
  //     Serial.println("+- 1000 deg/s");
  //     break;
  //   case MPU6050_RANGE_2000_DEG:
  //     Serial.println("+- 2000 deg/s");
  //     break;
  //   }

  //   mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  //   Serial.print("Filter bandwidth set to: ");
  //   switch (mpu.getFilterBandwidth()) {
  //   case MPU6050_BAND_260_HZ:
  //     Serial.println("260 Hz");
  //     break;
  //   case MPU6050_BAND_184_HZ:
  //     Serial.println("184 Hz");
  //     break;
  //   case MPU6050_BAND_94_HZ:
  //     Serial.println("94 Hz");
  //     break;
  //   case MPU6050_BAND_44_HZ:
  //     Serial.println("44 Hz");
  //     break;
  //   case MPU6050_BAND_21_HZ:
  //     Serial.println("21 Hz");
  //     break;
  //   case MPU6050_BAND_10_HZ:
  //     Serial.println("10 Hz");
  //     break;
  //   case MPU6050_BAND_5_HZ:
  //     Serial.println("5 Hz");
  //     break;
  //   }
    
    // MPU6050 initialization ends here

  config.token_status_callback = tokenStatusCallback; 

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  databasePath = "/flex_sensors_Left";
}

void loop() {
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 2500 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    
    f_volt1 = analogRead(A0);
    f_volt2 = analogRead(A1);
    f_volt3 = analogRead(A2);
    f_volt4 = analogRead(A3);
    f_volt5 = analogRead(A4);

    // sensors_event_t a, g, temp;
    // mpu.getEvent(&a, &g, &temp);
    
    // gyroX = g.gyro.x;
    // gyroY = g.gyro.y;
    // gyroZ = g.gyro.z;

    // accX = a.acceleration.x;
    // accY = a.acceleration.y;
    // accZ = a.acceleration.z;
    // accNet = (sqrt(sq(accX)+sq(accY)+sq(accZ)));

    // tempr = temp.temperature;

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

    // Predict character based on finger range logic
    char predicted = inferLetterFromRange(f_volt1, f_volt2, f_volt3, f_volt4, f_volt5);
    predictedLetter = String(predicted);

    Serial.print("Predicted letter: ");
    Serial.println(predicted);

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
    // json.set("Gyroscope X", gyroX);
    // json.set("Gyroscope Y", gyroY);
    // json.set("Gyroscope Z", gyroZ);
    // json.set("Net Acceleration", accNet);
    // json.set("Tempreture", tempr);
    json.set("Predicted Letter", predictedLetter);

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
