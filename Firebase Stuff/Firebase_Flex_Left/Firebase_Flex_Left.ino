// Om Namo Narayana

#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

#define WIFI_SSID "Galaxy Z Flip5"
#define WIFI_PASSWORD "vyrt2391"

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
  {{1554, 2017, 1293, 1067, 0}, {2906, 3042, 2415, 2295, 0}, 'a'},
  {{0, 0, 0, 0, 774}, {0, 199, 0, 0, 1252}, 'b'},
  {{76, 717, 123, 198, 0}, {1424, 1885, 1185, 995, 0}, 'c'},
  {{346, 1521, 640, 0, 0}, {1854, 2527, 1671, 0, 0}, 'd'},
  {{453, 1680, 1111, 859, 380}, {1967, 2470, 1671, 1855, 1828}, 'e'},
  {{0, 0, 0, 205, 0}, {0, 0, 0, 991, 0}, 'f'},
  {{727, 1771, 1035, 0, 0}, {2000, 2467, 1899, 0, 0}, 'g'},
  {{1059, 1023, 0, 0, 0}, {2303, 1924, 0, 0, 0}, 'h'},
  {{0, 377, 402, 169, 117}, {0, 2615, 2037, 2309, 1238}, 'i'},
  {{29, 308, 0, 0, 0}, {1870, 1136, 53, 0, 0}, 'k'},
  {{111, 1554, 903, 0, 17}, {1421, 2294, 1719, 0, 987}, 'l'},
  {{214, 1296, 244, 51, 2}, {2183, 2339, 1392, 1279, 1150}, 'm'},
  {{64, 228, 138, 146, 39}, {2093, 2493, 1408, 1117, 1488}, 'n'},
  {{23, 441, 194, 13, 0}, {1447, 2023, 1528, 952, 163}, 'o'},
  {{91, 32, 0, 0, 0}, {1492, 1657, 0, 0, 0}, 'p'},
  {{221, 1793, 586, 0, 0}, {1761, 2533, 1900, 0, 6}, 'q'},
  {{159, 772, 0, 64, 0}, {2154, 2163, 1143, 1078, 626}, 'r'},
  {{71, 1424, 43, 400, 6}, {2377, 2293, 1792, 1233, 1251}, 's'},
  {{113, 305, 343, 31, 54}, {1794, 1385, 1600, 894, 1387}, 't'},
  {{33, 291, 0, 0, 0}, {2076, 1774, 0, 0, 0}, 'u'},
  {{48, 752, 0, 0, 0}, {2134, 2134, 0, 0, 434}, 'v'},
  {{90, 0, 0, 0, 0}, {1844, 0, 0, 0, 780}, 'w'},
  {{31, 941, 133, 0, 158}, {1614, 2559, 1607, 961, 1271}, 'x'},
  {{0, 63, 13, 45, 0}, {16, 1961, 1250, 677, 0}, 'y'},
  {{0, 1006, 922, 0, 0}, {0, 1726, 1632, 0, 0}, 'ILY'}
};
const int modelCount = sizeof(letterModels) / sizeof(letterModels[0]);

char inferLetterFromRange(int pinky, int ring, int middle, int index, int thumb) {
  for (int i = 0; i < modelCount; i++) {
    bool match = true;
    if (pinky  < letterModels[i].minVal[0] || pinky  > letterModels[i].maxVal[0]) match = false;
    if (ring   < letterModels[i].minVal[1] || ring   > letterModels[i].maxVal[1]) match = false;
    if (middle < letterModels[i].minVal[2] || middle > letterModels[i].maxVal[2]) match = false;
    if (index  < letterModels[i].minVal[3] || index  > letterModels[i].maxVal[3]) match = false;
    if (thumb  < letterModels[i].minVal[4] || thumb  > letterModels[i].maxVal[4]) match = false;
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
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  // MPU6050 setup
  // Do not change this snippet
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  switch (mpu.getAccelerometerRange()) {
    case MPU6050_RANGE_2_G:
      Serial.println("+-2G");
      break;
    case MPU6050_RANGE_4_G:
      Serial.println("+-4G");
      break;
    case MPU6050_RANGE_8_G:
      Serial.println("+-8G");
      break;
    case MPU6050_RANGE_16_G:
      Serial.println("+-16G");
      break;
    }
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    Serial.print("Gyro range set to: ");
    switch (mpu.getGyroRange()) {
    case MPU6050_RANGE_250_DEG:
      Serial.println("+- 250 deg/s");
      break;
    case MPU6050_RANGE_500_DEG:
      Serial.println("+- 500 deg/s");
      break;
    case MPU6050_RANGE_1000_DEG:
      Serial.println("+- 1000 deg/s");
      break;
    case MPU6050_RANGE_2000_DEG:
      Serial.println("+- 2000 deg/s");
      break;
    }

    mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
    Serial.print("Filter bandwidth set to: ");
    switch (mpu.getFilterBandwidth()) {
    case MPU6050_BAND_260_HZ:
      Serial.println("260 Hz");
      break;
    case MPU6050_BAND_184_HZ:
      Serial.println("184 Hz");
      break;
    case MPU6050_BAND_94_HZ:
      Serial.println("94 Hz");
      break;
    case MPU6050_BAND_44_HZ:
      Serial.println("44 Hz");
      break;
    case MPU6050_BAND_21_HZ:
      Serial.println("21 Hz");
      break;
    case MPU6050_BAND_10_HZ:
      Serial.println("10 Hz");
      break;
    case MPU6050_BAND_5_HZ:
      Serial.println("5 Hz");
      break;
    }
    
    // MPU6050 initialization ends here

  config.token_status_callback = tokenStatusCallback; 

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  databasePath = "/flex_sensors_Left";
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
    accNet = (sqrt(sq(accX)+sq(accY)+sq(accZ)));

    tempr = temp.temperature;

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
    json.set("Gyroscope X", gyroX);
    json.set("Gyroscope Y", gyroY);
    json.set("Gyroscope Z", gyroZ);
    json.set("Net Acceleration", accNet);
    json.set("Tempreture", tempr);
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
