    
#define BLYNK_TEMPLATE_ID "TMPL6lVHyxhtI"
#define BLYNK_TEMPLATE_NAME "Kualitas Air Depot Air Isi Ulang Barokah Tirta"
#define BLYNK_AUTH_TOKEN "_fYmfxuiACjgowde-tL6FxWG5qPLZSZe"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "iotsaya";
char pass[] = "suksesTA2023";


#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

// library suhu
#include <OneWire.h>
#include <DallasTemperature.h>
// library TDS



// pin suhu 
#define ONE_WIRE_BUS 12

// tds
#define TdsSensorPin 32
#define VREF 3.3              // analog reference voltage(Volt) of the ADC
#define SCOUNT  30            // sum of sample point

// ph
#define sensorPinPH  35

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// FLOAT suhu
float Suhu ;

int analogBuffer[SCOUNT];     // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25;       // current temperature for compensation

// median filtering algorithm
int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

// float po pH;
float Po = 0;
float PH_Step;
int analogPH;
double TeganganPH;

float PH7 = 2.5;
float PH4 = 3.2;

void setup(){
  Serial.begin(115200);
  sensors.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  pinMode(ONE_WIRE_BUS, INPUT);
  pinMode(TdsSensorPin,INPUT);
  pinMode(sensorPinPH, INPUT);
  lcd.init();
  lcd.clear();
  lcd.backlight();
}

void loop(){

  // suhu 
  sensors.requestTemperatures();
  Suhu = sensors.getTempCByIndex(0);
  
  // TDS
  static unsigned long analogSampleTimepoint = millis();
  if(millis()-analogSampleTimepoint > 40U){     //every 40 milliseconds,read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if(analogBufferIndex == SCOUNT){ 
      analogBufferIndex = 0;
    }
  }   
  
  static unsigned long printTimepoint = millis();
  if(millis()-printTimepoint > 800U){
    printTimepoint = millis();
    for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      
      // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0;
      
      //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
      float compensationCoefficient = 1.0+0.02*(temperature-25.0);
      //temperature compensation
      float compensationVoltage=averageVoltage/compensationCoefficient;
      
      //convert voltage value to tds value
      tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5 - 350;
      
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
    }
  }


  // pH 
  analogPH = analogRead(sensorPinPH);
  TeganganPH = (3.3 / 4096) * analogPH;
  PH_Step = (PH4 - PH7)/ 3;
  Po = 7.00 + ((PH7 - TeganganPH) / PH_Step);

  // Serial Monitor 
  Serial.print("Suhu : ");
  Serial.print(Suhu);
  Serial.println("C");
  Serial.print("TDS Value : ");
  Serial.print(tdsValue,0);
  Serial.println(" ppm");
  Serial.print("pH : ");
  Serial.println(Po,2);


  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Suhu");
  lcd.setCursor(6, 0);
  lcd.print("TDS");
  lcd.setCursor(12,0);
  lcd.print("PH");
  lcd.setCursor(0, 1);
  lcd.print(Suhu,0);
  lcd.setCursor(6 , 1);
  lcd.print(tdsValue,0);
  lcd.setCursor(12, 1);
  lcd.print(Po,2);
  
  Blynk.virtualWrite(V0, Suhu);
  Blynk.virtualWrite(V1, tdsValue);
  Blynk.virtualWrite(V2, Po,2);

  delay(2000);
  // pengkondisian air baik atau buruk 
  if(tdsValue < 1000){
    Serial.print("Kualitas Air Layak Minum");
    lcd.setCursor(0, 0);
    lcd.print(" KUALITAS  AIR  ");
    lcd.setCursor(0 , 1);
    lcd.print("  LAYAK MINUM   ");
    Blynk.virtualWrite(V3, "Air Layak Minum");
    }
   else if(tdsValue >= 1000){
    Serial.print("Air Tidak Layak Minum");
    lcd.setCursor(0, 0);
    lcd.print("   AIR TIDAK   ");
    lcd.setCursor(0 , 1);
    lcd.print("  LAYAK MINUM  ");
    Blynk.virtualWrite(V3,"Air Tidak Layak Minum");
    }
  delay(2000);

  Blynk.run();
}
