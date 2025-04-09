// Om Namo Narayana

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <WebServer.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;
WebServer server(80);  // Create a web server on port 80

#define WIFI_SSID "8====D"
#define WIFI_PASSWORD "coffeebread123"

#define API_KEY "AIzaSyALoU_0xKcg2vyvX8N2G38tjX06Pq0fha0"
#define DATABASE_URL "asl-interpreter-glove-default-rtdb.firebaseio.com"

const int F1 = A0; 
const int F2 = A1; 
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
String predictedLetter = "-";

unsigned long sendDataPrevMillis = 0;
unsigned long lastUpdateTime = 0;

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

void handleRoot() {
  String html = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<meta charset='UTF-8'>"
                "<title>ASL Interpreter Glove</title>"
                "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                "<meta http-equiv='refresh' content='2'>"
                "<style>"
                "body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 20px; background-color: #f0f0f0; }"
                "h1 { color: #333; }"
                ".container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }"
                ".data-container { display: flex; flex-wrap: wrap; justify-content: space-around; margin: 20px 0; }"
                ".sensor-box { width: 45%; margin: 10px; padding: 15px; background-color: #e7f3ff; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }"
                ".progress-bar { height: 20px; background-color: #ddd; border-radius: 10px; margin: 10px 0; overflow: hidden; }"
                ".progress-fill { height: 100%; background-color: #4CAF50; width: 0%; transition: width 0.5s ease-in-out; }"
                "@media (max-width: 600px) { .sensor-box { width: 100%; } }"
                ".timestamp { font-size: 12px; color: #666; margin-top: 20px; }"
                "</style>"
                "</head>"
                "<body>"
                "<div class='container'>"
                "<h1>ASL Interpreter Glove</h1>"
                "<div class='data-container'>"
                "<div class='sensor-box'>"
                "<h2>Finger Flexion</h2>"
                "<p>Finger 1: " + String(f_volt1) + "</p>"
                "<p>Finger 2: " + String(f_volt2) + "</p>"
                "<p>Finger 3: " + String(f_volt3) + "</p>"
                "<p>Finger 4: " + String(f_volt4) + "</p>"
                "<p>Finger 5: " + String(f_volt5) + "</p>"
                "</div>"
                "<div class='sensor-box'>"
                "<h2>Motion Sensors</h2>"
                "<p>Gyro X: " + String(gyroX) + "</p>"
                "<p>Gyro Y: " + String(gyroY) + "</p>"
                "<p>Gyro Z: " + String(gyroZ) + "</p>"
                "<p>Net Acceleration: " + String(accNet) + "</p>"
                "<p>Temperature: " + String(tempr) + " °C</p>"
                "</div>"
                "</div>"
                "<h2>Predicted Letter</h2>"
                "<p style='font-size: 48px; font-weight: bold; color: #4CAF50;'>" + predictedLetter + "</p>"
                "<p class='timestamp'>Last updated: " + String(millis() / 1000.0) + "s</p>"
                "</div>"
                "</body>"
                "</html>";
  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"f1\":" + String(f_volt1) + ",";
  json += "\"f2\":" + String(f_volt2) + ",";
  json += "\"f3\":" + String(f_volt3) + ",";
  json += "\"f4\":" + String(f_volt4) + ",";
  json += "\"f5\":" + String(f_volt5) + ",";
  json += "\"gx\":" + String(gyroX) + ",";
  json += "\"gy\":" + String(gyroY) + ",";
  json += "\"gz\":" + String(gyroZ) + ",";
  json += "\"acc\":" + String(accNet) + ",";
  json += "\"temp\":" + String(tempr) + ",";
  json += "\"predicted\":\"" + predictedLetter + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// rest of setup(), loop(), updateSensorData(), sendDataToFirebase() unchanged
