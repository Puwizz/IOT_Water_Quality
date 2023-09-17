

// Add comments and documentation as needed

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <DallasTemperature.h>
#include <OneWire.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
#define TdsSensorPin 33
#define VREF 1 // Set the correct VREF value
#define SCOUNT 30
int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 0;
bool reCheck = false;

#define ONE_WIRE_BUS 26
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define CHANNEL_ID 2249915
#define CHANNEL_API_KEY "M5WBIVWBK18Z303H"
#define WIFI_TIMEOUT_MS 20000
#define WIFI_NETWORK "Pattana_2.4G"
#define WIFI_PASSWORD "12345678"
WiFiClient client;

int led = 17;
void connectToWifi() {
  // Add error handling for Wi-Fi connection
  lcd.setCursor(0, 0);
  lcd.print("ConnectingToWifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
    delay(100);
    lcd.setCursor(0, 1);
    lcd.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connection Failed!");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(5000);
    lcd.clear();
    reCheck = true;
  }
  Serial.println(WiFi.status());
}

void setup() {
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("WaterQualityIOT");
  delay(2000);
  lcd.clear();

  Serial.begin(115200);
  connectToWifi();
  ThingSpeak.begin(client);

  pinMode(TdsSensorPin, INPUT);
  pinMode(led, OUTPUT);
  //pinMode(buzzer, OUTPUT);
  //pinMode(led2, OUTPUT);
}

void loop() {
  while (!reCheck) {
    connectToWifi();
  }

  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT)
      analogBufferIndex = 0;
  }

  // Temp
  static unsigned long check_Temp = millis();
  if (millis() - check_Temp > 800U) {
    check_Temp = millis();
    sensors.requestTemperatures();
    temperature = sensors.getTempCByIndex(0);

    lcd.setCursor(0, 0);
    lcd.print("Temp :");
    lcd.setCursor(7, 0);
    lcd.print(temperature);
    lcd.setCursor(13, 0);
    lcd.print("C");

    ThingSpeak.writeField(CHANNEL_ID, 1, temperature, CHANNEL_API_KEY);
  }

  // TDS code
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U)
  {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 4096.0;
    float compensationCoefficient = 1.0 + 0.019 * (temperature); // ปรับค่าความชดเชยอุณหภูมิตามต้องการ
    float compensationVoltage = averageVoltage / compensationCoefficient;
    tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;

    lcd.setCursor(0, 1);
    lcd.print("TDS :");
    lcd.setCursor(6, 1);
    lcd.print(tdsValue, 0);
    lcd.setCursor(12, 1);
    lcd.print("ppm");

    if (tdsValue > 500) 
    {
     digitalWrite(led, HIGH);
    //digitalWrite(buzzer, HIGH);
    } else {
    digitalWrite(led, LOW);
    //digitalWrite(buzzer, LOW);
   }

    ThingSpeak.writeField(CHANNEL_ID, 2, tdsValue, CHANNEL_API_KEY);
  }


  static unsigned long reSet_lcd = millis();
  if (millis() - reSet_lcd > 3000U) {
    reSet_lcd = millis();
    lcd.clear();
  }
}

int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
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
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}
