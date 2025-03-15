// Om Namo Narayana

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

void setup(){
  Serial.begin(115200);      
}

void loop(){
  f_volt1 = analogRead(F1);
  f_volt2 = analogRead(F2);
  f_volt3 = analogRead(F3);
  f_volt4 = analogRead(F4);
  f_volt5 = analogRead(F5);

  Serial.print("Finger 1: ");
  Serial.print(f_volt1);
  Serial.print("\t"); 

  Serial.print("Finger 2:");
  Serial.print(f_volt2);
  Serial.print("\t");

  Serial.print("Finger 3:");
  Serial.print(f_volt3);
  Serial.print("\t");

  Serial.print("Finger 4:");
  Serial.print(f_volt4);
  Serial.print("\t");

  Serial.print("Finger 5:");
  Serial.print(f_volt5);
  Serial.print("\t");


    delay(2500);                   
  }