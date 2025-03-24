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

// #define WIFI_SSID "Galaxy Z Flip5"
// #define WIFI_PASSWORD "vyrt2391"
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

float gyroX, gyroY, gyroZ;
float accX, accY, accZ, accNet; 
float tempr;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String uid;
String databasePath;

unsigned long sendDataPrevMillis = 0;
unsigned long lastUpdateTime = 0;

// Function to handle root URL
void handleRoot() {
  String html = "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<meta charset='UTF-8'>"
                "<title>ASL Interpreter Glove</title>"
                "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                "<meta http-equiv='refresh' content='2'>"  // Auto-refresh the entire page every 2 seconds
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
                "<p>Finger 1: <span id='f1'>" + String(f_volt1) + "</span></p>"
                "<div class='progress-bar'><div class='progress-fill' id='f1-bar' style='width: " + String(f_volt1 * 100.0 / 4095.0) + "%'></div></div>"
                "<p>Finger 2: <span id='f2'>" + String(f_volt2) + "</span></p>"
                "<div class='progress-bar'><div class='progress-fill' id='f2-bar' style='width: " + String(f_volt2 * 100.0 / 4095.0) + "%'></div></div>"
                "<p>Finger 3: <span id='f3'>" + String(f_volt3) + "</span></p>"
                "<div class='progress-bar'><div class='progress-fill' id='f3-bar' style='width: " + String(f_volt3 * 100.0 / 4095.0) + "%'></div></div>"
                "<p>Finger 4: <span id='f4'>" + String(f_volt4) + "</span></p>"
                "<div class='progress-bar'><div class='progress-fill' id='f4-bar' style='width: " + String(f_volt4 * 100.0 / 4095.0) + "%'></div></div>"
                "<p>Finger 5: <span id='f5'>" + String(f_volt5) + "</span></p>"
                "<div class='progress-bar'><div class='progress-fill' id='f5-bar' style='width: " + String(f_volt5 * 100.0 / 4095.0) + "%'></div></div>"
                "</div>"
                "<div class='sensor-box'>"
                "<h2>Motion Sensors</h2>"
                "<p>Gyro X: <span id='gx'>" + String(gyroX) + "</span></p>"
                "<p>Gyro Y: <span id='gy'>" + String(gyroY) + "</span></p>"
                "<p>Gyro Z: <span id='gz'>" + String(gyroZ) + "</span></p>"
                "<p>Net Acceleration: <span id='acc'>" + String(accNet) + "</span></p>"
                "<p>Temperature: <span id='temp'>" + String(tempr) + "</span>°C</p>"
                "</div>"
                "</div>"
                "<p class='timestamp'>Last updated: " + String(millis() / 1000.0) + "s</p>"
                "</div>"
                "</body>"
                "</html>";
  server.send(200, "text/html", html);
}

// Function to handle JSON data request
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
  json += "\"temp\":" + String(tempr);
  json += "}";
  
  server.send(200, "application/json", json);
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
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP server started");
  
  // Gyroscope searching 
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  Serial.println("SUIIIIIIII");

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

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    // Serial.println("ESP is in");
    uid = auth.token.uid.c_str();
  } else {
    // Serial.printf("ESP didn't get connected. The fuckup is here:", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback; 

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  databasePath = "/flex_sensors_Right";
}

void loop() {
  // Handle web server client requests
  server.handleClient();
  
  // Read sensors and update data
  updateSensorData();
  
  // Send data to Firebase every 2.5 seconds
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 2500 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    sendDataToFirebase();
  }
}

void updateSensorData() {
  // Update sensor data at a reasonable rate (e.g., every 100ms)
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime > 100) {
    lastUpdateTime = currentTime;
    
    f_volt1 = analogRead(F1);
    f_volt2 = analogRead(F2);
    f_volt3 = analogRead(F3);
    f_volt4 = analogRead(F4);
    f_volt5 = analogRead(F5);

    // Apply the same filtering as in the original code
    if (f_volt3 < 200){
      f_volt3 = 0;
    }
    if (f_volt4 < 200){
      f_volt4 = 0;
    }
    if (f_volt5 < 200){
      f_volt5 = 0;
    }

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

    // Output to serial for debugging
    Serial.print(f_volt1);
    Serial.print(", "); 
    Serial.print(f_volt2);
    Serial.print(", ");
    Serial.print(f_volt3);
    Serial.print(", ");
    Serial.print(f_volt4);
    Serial.print(", ");
    Serial.print(f_volt5);
    Serial.print(", ");
    Serial.print(gyroX);
    Serial.print(", ");
    Serial.print(gyroY);
    Serial.print(", ");
    Serial.print(gyroZ);
    Serial.print(", ");
    Serial.print(accNet);
    Serial.println();
  }
}

void sendDataToFirebase() {
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

  // Don't mess with this part of the code.
  // It is something i scouted out from github
  // I still don't completely understand what's going on here.

  String latestPath = databasePath;
  // String historyPath = databasePath + "/history/" + String(millis());

  if (Firebase.RTDB.setJSON(&fbdo, latestPath.c_str(), &json)) {
    Serial.println("GGWP");
  } else {
    Serial.println("Kal");
    Serial.println(": " + fbdo.errorReason());
  }
  delay(2500);
}