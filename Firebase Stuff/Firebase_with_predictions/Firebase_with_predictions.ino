#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Provide the token generation process info
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info
#include <addons/RTDBHelper.h>

// WiFi credentials
#define WIFI_SSID "SSID here"
#define WIFI_PASSWORD "Wifi password here"

// Firebase project settings
#define API_KEY "API Key here"
#define DATABASE_URL "Database URL here"

// Define Firebase Data object, auth and config
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Pin definitions for flex sensors - ESP32 pins
const int F1 = 37; // VP - ADC1_CH0
const int F2 = 33; // VN - ADC1_CH3
const int F3 = A2; // ADC1_CH6
const int F4 = A3; // ADC1_CH7
const int F5 = A4; // ADC1_CH4

int nig;
int ler;
int f_volt3;
int f_volt4;
int f_volt5;

String predictedLetter = "-";
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

struct FingerRange {
  int minVal[5];
  int maxVal[5];
  char letter;
};

FingerRange letterModels[] = {
  {{100, 117, 93, 107, 0}, {2906, 3042, 2415, 2295, 0}, 'a'},
  {{0, 0, 0, 0, 500}, {0, 199, 0, 0, 2500}, 'b'},
  // {{76, 77, 73, 78, 0}, {1424, 1885, 1185, 995, 0}, 'c'},  
  // {{50, 50, 50, 0, 0}, {1854, 1500, 1300, 0, 0}, 'd'},
  // {{453, 1680, 1111, 859, 380}, {1967, 2470, 1671, 1855, 1828}, 'e'},
  {{0, 0, 0, 205, 0}, {0, 0, 0, 1500, 0}, 'f'},
  {{150, 150, 135, 0, 0}, {2000, 2467, 1899, 0, 0}, 'g'},
  // {{150, 150, 0, 0, 0}, {2303, 1924, 0, 0, 0}, 'h'},
  {{0, 377, 402, 169, 117}, {0, 2615, 2037, 2309, 1238}, 'i'},
  // {{29, 308, 0, 0, 0}, {1870, 1136, 0, 0, 0}, 'k'},
  // {{111, 111, 111, 0, 17}, {1421, 2294, 1719, 0, 987}, 'l'},
  // {{214, 1296, 244, 51, 2}, {2183, 2339, 1392, 1279, 1150}, 'm'},
  // {{64, 228, 138, 146, 39}, {2093, 2493, 1408, 1117, 1488}, 'n'},
  {{23, 441, 194, 13, 100}, {1847, 2023, 1628, 1652, 1863}, 'o'},
  // {{91, 32, 0, 0, 0}, {1492, 1657, 0, 0, 0}, 'p'},
  // {{221, 1793, 586, 0, 0}, {1761, 2533, 1900, 0, 6}, 'q'},
  // {{159, 772, 0, 64, 0}, {2154, 2163, 1143, 1078, 626}, 'r'},
  // {{71, 1424, 43, 400, 6}, {2377, 2293, 1792, 1233, 1251}, 's'},
  // {{113, 305, 343, 31, 54}, {1794, 1385, 1600, 894, 1387}, 't'},
  // {{33, 291, 0, 0, 0}, {2076, 1774, 0, 0, 0}, 'u'},
  {{48, 500, 0, 0, 300}, {2134, 2134, 0, 0, 1600}, 'v'},
  // {{90, 0, 0, 0, 00}, {2500, 0, 0, 0, 2600}, 'w'},
  // {{31, 941, 133, 0, 158}, {1614, 2559, 1607, 961, 1271}, 'x'},
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

      // Add 15% margin below min and above max
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

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  
  // Set the ADC resolution to 12 bits (0-4095)
  analogReadResolution(12);
  
  // Connect to WiFi
  connectWiFi();
  
  // Assign the API key
  config.api_key = API_KEY;

  // Assign the RTDB URL
  config.database_url = DATABASE_URL;

  // Sign up
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase signup OK");
    signupOK = true;
  } else {
    Serial.printf("Firebase signup failed: %s\n", config.signer.signupError.message.c_str());
  }

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  
  // Begin connection
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Firebase setup complete!");
}

void loop() {
  // Read flex sensor values
  nig = analogRead(F1);
  ler = analogRead(F2);
  f_volt3 = analogRead(F3);
  f_volt4 = analogRead(F4);
  f_volt5 = analogRead(F5);

  // if(nig > 200) {
  //   nig += 1000;
  // }

  // if(ler > 200) {
  //   ler += 1000;
  // }

  // Debug output
  Serial.print("Finger 1: ");
  Serial.print(nig);
  Serial.print("\t"); 

  Serial.print("Finger 2: ");
  Serial.print(ler);
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

  // Predict letter based on sensor values
  char predicted = inferLetterFromRange(nig, ler, f_volt3, f_volt4, f_volt5);

  if (predicted != '-') {
    predictedLetter = String(predicted);
  }

  Serial.print("Predicted letter: ");
  Serial.println(predictedLetter);

  // Only send data if Firebase signup was successful
  if (signupOK && Firebase.ready() && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    
    // Write sensor values to Firebase
    Firebase.RTDB.setInt(&fbdo, "signLanguageData/finger1", nig);
    Firebase.RTDB.setInt(&fbdo, "signLanguageData/finger2", ler);
    Firebase.RTDB.setInt(&fbdo, "signLanguageData/finger3", f_volt3);
    Firebase.RTDB.setInt(&fbdo, "signLanguageData/finger4", f_volt4);
    Firebase.RTDB.setInt(&fbdo, "signLanguageData/finger5", f_volt5);

    if(predictedLetter == "k"){
      Firebase.RTDB.setString(&fbdo, "signLanguageData/predictedLetter", "I Love You");
    }

    else if(predictedLetter == "l")
    {
      Firebase.RTDB.setString(&fbdo, "signLanguageData/predictedLetter", "Fine");
    }

    else if(predictedLetter == "m")
    {
      Firebase.RTDB.setString(&fbdo, "signLanguageData/predictedLetter", "Point");
    }

    else
    {
      Firebase.RTDB.setString(&fbdo, "signLanguageData/predictedLetter", predictedLetter);
    }
    
    Firebase.RTDB.setInt(&fbdo, "signLanguageData/timestamp", millis());
    
    Serial.println("Firebase update successful");
  }
}