// Om namo Narayana


#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#define WebServerClass WebServer

const char* ssid = "8====D";
const char* password = "coffeebread123";

WebServerClass server(80);

const int F1 = A0;
const int F2 = A1;
const int F3 = A2;
const int F4 = A4;
const int F5 = A5;

int f_volt1;
int f_volt2;
int f_volt3;
int f_volt4;
int f_volt5;

String decision = "";

unsigned long previousMillis = 0;
const long interval = 2500; 

// doing HTML in different file....
// idk if this shii will work, trying
// it for the first time...
// go easy on me
// Om namo Narayana

extern const char index_html[] PROGMEM;

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(A5));
  

  if(!SPIFFS.begin(true)) {
    Serial.println("Shit ain't goin down");
    return;
  }

  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    updateSensorValues();
  }
}

void updateSensorValues() {

  f_volt1 = analogRead(A0);
  f_volt2 = analogRead(A1);
  f_volt3 = analogRead(A2);
  f_volt4 = analogRead(A3);
  f_volt5 = analogRead(A4);
  
  // testing code
  // remove this while using the sensors
  // code will not work if not removed
  // using this as placeholders until
  // we get 10k ohm resistors.

  f_volt1 = random(1024);
  f_volt2 = random(1024);
  f_volt3 = random(1024);
  f_volt4 = random(1024);
  f_volt5 = random(1024);
  
  // End of testing code
  // Looks like the sensors are working
  // Still keeping the testing code in case
  // might use it when fucking around without sensors

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
  Serial.print("\t");
  
  if (f_volt1 > 200 && f_volt2 < 200)
    decision = "Water";
  
  else if (f_volt2 > 200 && f_volt1 < 200) 
    decision = "Food";
  
  else if (f_volt1 > 200 && f_volt2 > 200) 
    decision = "Water and Food";
  
  else
    decision = "3, 4, 5";
  
  
  Serial.println(decision);
}

void handleRoot() {
  // Method 1: Serve the HTML directly from PROGMEM
  // server.send(200, "text/html", index_html);
  
  // Method 2: Serve the HTML from SPIFFS file
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    Serial.println("Failed to open file");
    server.send(500, "text/plain", "Internal Server Error");
    return;
  } 
  server.streamFile(file, "text/html");
  file.close();
}

// Handle data request for AJAX updates
// Need this for asynch JSON management
//
void handleData() {
  String jsonData = "{";
  jsonData += "\"f1\":" + String(f_volt1) + ",";
  jsonData += "\"f2\":" + String(f_volt2) + ",";
  jsonData += "\"f3\":" + String(f_volt3) + ",";
  jsonData += "\"f4\":" + String(f_volt4) + ",";
  jsonData += "\"f5\":" + String(f_volt5) + ",";
  jsonData += "\"decision\":\"" + decision + "\"";
  jsonData += "}";
  
  server.send(200, "application/json", jsonData);
}


void handleNotFound() {
  server.send(404, "text/plain", "404: Not found");
}