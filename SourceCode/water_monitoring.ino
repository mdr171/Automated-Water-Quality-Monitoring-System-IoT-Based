// DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>

const int oneWireBus = 2; // Misalnya, gunakan pin digital 2
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

// pH_meter
const int sensorPin = A0;
float Kadar_pH = 0; 

//LCD_I2C
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

// firebase
#if defined(ESP32)
    #include <WiFi.h>
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

// Provide the token generation process info and the RTDB payload printing info and other helper functions.
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// insert your network credentials.
#define WIFI_SSID "ASUS_X00TD"
#define WIFI_PASSWORD "12345678"

// insert Firebase project API key and RTDB URLefine the RTDB URL.
#define API_KEY "AIzaSyB331SCnBRhHhONWYklEgtHNFeC07O-2NI"
#define DATABASE_URL "https://water-monitoring-project-25b0d-default-rtdb.firebaseio.com/"

// define the Firebase Data object.
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long previousMillis_DS18B20 = 0;
unsigned long interval_DS18B20 = 1000;

unsigned long previousMillis_pH_meter = 0;
unsigned long interval_pH_meter = 1000;

unsigned long previousMillis_displayTemp = 0;
unsigned long interval_displayTemp = 1000;

unsigned long previousMillis_displaypH = 0;
unsigned long interval_displaypH = 1000;

unsigned long previousMillis_display = 0;
unsigned long interval_display = 200;

int count = 0;

bool signupOK = false;

//Icon di arduino
byte Temp[8] = //icon for termometer
{
 B00100,
 B01010,
 B01010,
 B01110,
 B01110,
 B11111,
 B11111,
 B01110
};

byte pH[8] = //icon for water droplet
{
 B00100,
 B00100,
 B01110,
 B01110,
 B11111,
 B11111,
 B11111,
 B01110,
};

void setup() {
  Serial.begin(115200);

  // pH_meter
  pinMode (sensorPin, INPUT);

  // LCD_I2C
  lcd.init();         // initialize the lcd
  lcd.backlight();    // Turn on the LCD screen backlight

  lcd.createChar(1, Temp); // create a new icon for termometer
  lcd.createChar(2, pH);   // create a new icon for water droplet

  // firebase
  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {

      modeOffline();
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  

  // assign the API key and RTDB URL to the config object.
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // sign up
  if (Firebase.signUp(&config, &auth, "", "")){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sign up OK");
      Serial.println("ok");
      signupOK = true;
      delay(1500);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Online");
      delay(1000);
  }
  else {
      lcd.clear();
      lcd.print("Sign up FAILED");
      delay(1500);
      
      while (signupOK != true) {
        modeOffline();
        }
  }

  // assign the callback function for the long running token generation task.
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  unsigned long currentMillis = millis();

  // task 1 : DS18B20
  if (Firebase.ready() && signupOK && (currentMillis - previousMillis_DS18B20 >= interval_DS18B20 || previousMillis_DS18B20 == 0 )) {
    
    sensors.requestTemperatures();

    // Membaca suhu dari sensor
    float temperatureCelsius = sensors.getTempCByIndex(0);
    float temperatureFahrenheit = sensors.getTempFByIndex(0);

    // Mengkonversi temperatureCelsius ke string dengan satu angka desimal
    char temperatureCelsiusStr[10];
    dtostrf(temperatureCelsius, 4, 1, temperatureCelsiusStr); // 4 karakter total, 1 angka desimal

    if (Firebase.RTDB.setFloat(&fbdo, "Water_Monitoring_Project_v1/Celsius", atof(temperatureCelsiusStr))){
        Serial.print("Suhu dalam Celsius: ");
        Serial.print(temperatureCelsiusStr);
        Serial.println(" °C");
    }
    else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
    }

    // Mengkonversi temperatureFahrenheit ke string dengan satu angka desimal
    char temperatureFahrenheitStr[10];
    dtostrf(temperatureFahrenheit, 4, 1, temperatureFahrenheitStr); // 4 karakter total, 1 angka desimal

    if (Firebase.RTDB.setFloat(&fbdo, "Water_Monitoring_Project_v1/Fahrenheit", atof(temperatureFahrenheitStr))) {
        Serial.print("Suhu dalam Fahrenheit: ");
        Serial.print(temperatureFahrenheitStr);
        Serial.println(" °F");
    } else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
    }

    previousMillis_DS18B20 = currentMillis;
  }

  // task 2 : pH_meter
  if (Firebase.ready() && signupOK && (currentMillis - previousMillis_pH_meter >= interval_pH_meter || previousMillis_pH_meter == 0 )) {
    
    int ADC_pH = analogRead(sensorPin);
    double Tegangan_pH = 5 / 1024.0 * ADC_pH;

    // Po = 7.00 + ((teganganPh7 - TeganganPh) / PhStep);
    Kadar_pH = 7.00 + ((5 - Tegangan_pH) / 0.17);

    // Mengkonversi ADC_pH ke string dengan satu angka desimal
    char ADC_pHStr[10];
    dtostrf(ADC_pH, 4, 1, ADC_pHStr); // 4 karakter total, 1 angka desimal

    if (Firebase.RTDB.setFloat(&fbdo, "Water_Monitoring_Project_v1/ADC_pH", atof(ADC_pHStr))) {
        Serial.print("Nilai ADC Ph: ");
        Serial.println(ADC_pHStr);
    } else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
    }

    char Tegangan_pHStr[15];
    dtostrf(Tegangan_pH, 4, 1, Tegangan_pHStr);

    if (Firebase.RTDB.setFloat(&fbdo, "Water_Monitoring_Project_v1/Tegangan_pH", atof(Tegangan_pHStr))) {
        Serial.print("Tegangan pH : ");
        Serial.println(Tegangan_pHStr);
    } else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
    }

    char Kadar_pHStr[15];
    dtostrf(Kadar_pH, 4, 1, Kadar_pHStr);

    if (Firebase.RTDB.setFloat(&fbdo, "Water_Monitoring_Project_v1/Kadar_pH", atof(Kadar_pHStr))) {
        Serial.print("Kadar pH : ");
        Serial.println(Kadar_pHStr);
    } else {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo.errorReason());
    }

    previousMillis_pH_meter = currentMillis;
  }

  // task 3 : displayTemp
  if (Firebase.ready() && signupOK && (currentMillis - previousMillis_displayTemp >= interval_displayTemp || previousMillis_displayTemp == 0 )) {

    sensors.requestTemperatures();

    // Membaca suhu dari sensor
    float temperatureCelsius = sensors.getTempCByIndex(0);
    float temperatureFahrenheit = sensors.getTempFByIndex(0);

    // Mengkonversi temperatureCelsius ke string dengan satu angka desimal
    char temperatureCelsiusStr[10];
    dtostrf(temperatureCelsius, 4, 1, temperatureCelsiusStr); // 4 karakter total, 1 angka desimal

    lcd.setCursor(0, 1);
    lcd.write(1); // write the termometer icon
    lcd.setCursor(2, 1);
    lcd.print(temperatureCelsiusStr);
    lcd.print((char)223);
    lcd.print("C");

    previousMillis_displayTemp = currentMillis;
  }

  // task 4 : displaypH
  if (Firebase.ready() && signupOK && (currentMillis - previousMillis_displaypH >= interval_displaypH || previousMillis_displaypH == 0 )) {

    int ADC_pH = analogRead(sensorPin);
    double Tegangan_pH = 5 / 1024.0 * ADC_pH;

    // Po = 7.00 + ((teganganPh7 - TeganganPh) / PhStep);
    Kadar_pH = 7.00 + ((5 - Tegangan_pH) / 0.17);
    
    char Kadar_pHStr[15];
    dtostrf(Kadar_pH, 4, 1, Kadar_pHStr);

    lcd.setCursor(9, 1);
    lcd.write(2); // write the water droplet icon
    lcd.setCursor(11, 1);
    lcd.print(Kadar_pHStr);

    previousMillis_displaypH = currentMillis;
  }

  // task 5 : display
  if (Firebase.ready() && signupOK && (currentMillis - previousMillis_display >= interval_display || previousMillis_display == 0 )) {
    
    // Gelombang air dari kolom 1 ke kolom 8
    for (int kolom = 1; kolom <= 19; kolom++) {
      lcd.setCursor(kolom - 1, 0);   // Set kursor ke kolom yang sesuai di baris pertama
      lcd.print("~");                // Tampilkan gelombang (~) di kolom yang ditunjuk
      delay(150);                    // Jeda antara setiap langkah
      lcd.setCursor(kolom - 1, 0);   // Set kursor ke kolom yang ditunjuk kembali
      lcd.print(" ");                // Hapus gelombang (~) dengan mengganti dengan spasi
  }
    
    previousMillis_display = currentMillis;
  }
}

void modeOffline() {
  
  unsigned long currentMillis = millis();

  // task 3 : displayTemp
  if ((currentMillis - previousMillis_displayTemp >= interval_displayTemp || previousMillis_displayTemp == 0 )) {

    sensors.requestTemperatures();

    // Membaca suhu dari sensor
    float temperatureCelsius = sensors.getTempCByIndex(0);
    float temperatureFahrenheit = sensors.getTempFByIndex(0);

    // Mengkonversi temperatureCelsius ke string dengan satu angka desimal
    char temperatureCelsiusStr[10];
    dtostrf(temperatureCelsius, 4, 1, temperatureCelsiusStr); // 4 karakter total, 1 angka desimal

    lcd.setCursor(0, 1);
    lcd.write(1); // write the termometer icon
    lcd.setCursor(2, 1);
    lcd.print(temperatureCelsiusStr);
    lcd.print((char)223);
    lcd.print("C");

    previousMillis_displayTemp = currentMillis;
  }

  // task 4 : displaypH
  if ((currentMillis - previousMillis_displaypH >= interval_displaypH || previousMillis_displaypH == 0 )) {

    int ADC_pH = analogRead(sensorPin);
    double Tegangan_pH = 5 / 1024.0 * ADC_pH;

    // Po = 7.00 + ((teganganPh7 - TeganganPh) / PhStep);
    Kadar_pH = 7.00 + ((5 - Tegangan_pH) / 0.17);

    char Kadar_pHStr[15];
    dtostrf(Kadar_pH, 4, 1, Kadar_pHStr);

    lcd.setCursor(9, 1);
    lcd.write(2); // write the water droplet icon
    lcd.setCursor(11, 1);
    lcd.print(Kadar_pHStr);

    previousMillis_displaypH = currentMillis;
  }

  // task 5 : display
  if ((currentMillis - previousMillis_display >= interval_display || previousMillis_display == 0 )) {
    
    // Gelombang air dari kolom 1 ke kolom 8
    for (int kolom = 1; kolom <= 19; kolom++) {
      lcd.setCursor(kolom - 1, 0);   // Set kursor ke kolom yang sesuai di baris pertama
      lcd.print("~");                // Tampilkan gelombang (~) di kolom yang ditunjuk
      delay(150);                    // Jeda antara setiap langkah
      lcd.setCursor(kolom - 1, 0);   // Set kursor ke kolom yang ditunjuk kembali
      lcd.print(" ");                // Hapus gelombang (~) dengan mengganti dengan spasi
  }
    
    previousMillis_display = currentMillis;
  }
}
