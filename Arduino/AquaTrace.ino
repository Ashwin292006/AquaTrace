#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- Pin Definitions ---
#define TURBIDITY_PIN A0
#define TDS_PIN A1
#define PH_PIN A2
#define ONE_WIRE_BUS 2
#define BUZZER_PIN 3

// --- Object Initialization ---
LiquidCrystal_I2C lcd(0x27, 20, 4);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// --- pH Calibration Variable ---
float ph_calibration_value = 21.34; 

void setup() {
  Serial.begin(9600);
  
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Stark Industries");
  lcd.setCursor(0, 1);
  lcd.print("System Calibrating");
  
  sensors.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  delay(3000);
  lcd.clear();
}

void loop() {
  // 1. Fetch Temperature
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  
  // --- AVERAGING ALGORITHM ---
  long phTotal = 0;
  long tdsTotal = 0;
  long turbTotal = 0;
  
  for(int i=0; i<10; i++) {
    phTotal += analogRead(PH_PIN);
    tdsTotal += analogRead(TDS_PIN);
    turbTotal += analogRead(TURBIDITY_PIN);
    delay(10); // Short delay between samples
  }
  
  int phRaw = phTotal / 10;
  int tdsRaw = tdsTotal / 10;
  int turbRaw = turbTotal / 10;
  // ---------------------------

  // 2. pH Calculation
  float phVoltage = phRaw * (5.0 / 1024.0);
  float phValue = -5.70 * phVoltage + ph_calibration_value;
  
  // 3. TDS Calculation
  float tdsVoltage = tdsRaw * (5.0 / 1024.0);
  float compensationCoefficient = 1.0 + 0.02 * (tempC - 25.0);
  float compensationVoltage = tdsVoltage / compensationCoefficient;
  float tdsValue = (133.42 * pow(compensationVoltage, 3) - 255.86 * pow(compensationVoltage, 2) + 857.39 * compensationVoltage) * 0.5;
  
  // 4. Turbidity Calculation
  float turbVoltage = turbRaw * (5.0 / 1024.0);
  float turbidityNTU = 0;
  
  if (turbVoltage < 2.5) {
    turbidityNTU = 3000;
  } else if (turbVoltage > 4.2) {
    turbidityNTU = 0; 
  } else {
    turbidityNTU = -1120.4 * pow(turbVoltage, 2) + 5742.3 * turbVoltage - 4353.8;
  }

  // --- "OUT OF WATER" LOGIC ---
  if (tdsRaw < 5) { // If the sensors are out of the water
    tdsValue = 0;
    turbidityNTU = 0;
  }
  if (turbidityNTU < 0) turbidityNTU = 0;

  // --- SERIAL MONITOR OUTPUT ---
  Serial.println("================================");
  Serial.print("Temp: "); Serial.print(tempC); Serial.println(" C");
  Serial.print("pH Raw Avg: "); Serial.print(phRaw);
  Serial.print(" | Calc pH: "); Serial.println(phValue, 2);
  Serial.print("TDS Calc: "); Serial.print(tdsValue, 0); Serial.println(" ppm");
  Serial.print("Turb Calc: "); Serial.print(turbidityNTU, 0); Serial.println(" NTU");
  Serial.println("================================\n");

  // --- LCD OUTPUT ---
  lcd.setCursor(0, 0);
  lcd.print("Temp: "); lcd.print(tempC, 1); lcd.print(" C    ");
  lcd.setCursor(0, 1);
  lcd.print("pH  : "); lcd.print(phValue, 2); lcd.print("      ");
  lcd.setCursor(0, 2);
  lcd.print("TDS : "); lcd.print(tdsValue, 0); lcd.print(" ppm  ");
  lcd.setCursor(0, 3);
  lcd.print("Turb: "); lcd.print(turbidityNTU, 0); lcd.print(" NTU  ");
  
  delay(1500); 
}
