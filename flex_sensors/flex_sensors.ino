const int F1 = A0; 
const int F2 = A1;
int f_volt1;
int f_volt2;

void setup(){
  Serial.begin(9600);      
}

void loop(){
  f_volt1 = analogRead(F1);
  f_volt2 = analogRead(F2);

  Serial.print("Finger 1: ");
  Serial.print(f_volt1);
  Serial.print("\t"); 
  Serial.print("Finger 2:");
  Serial.print(f_volt2);
  Serial.print("\t");

  
  if (f_volt1 > 200 && f_volt2 < 2000){
    Serial.println("Water");
  }

  else if(f_volt2 > 2000 && f_volt1 < 200){
    Serial.println("Food");
  }

  else if (f_volt1 > 200 && f_volt2 > 2000){
    Serial.println("Water and Food");
  }

  else{
    Serial.println("Nothing");
  }
  delay(2500);                   
}