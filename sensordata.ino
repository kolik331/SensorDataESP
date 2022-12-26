#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include "time.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define RXp2 16
#define TXp2 17
#define DHTPIN 4
#define DHTTYPE DHT22

// Insert your network credentials
#define WIFI_SSID  "Kolevi"  //"AndroidAPB4C2" "MITKO"
#define WIFI_PASSWORD "squidward1" //"0887703894" "12345678" 

// Insert Firebase project API Key
#define API_KEY "AIzaSyA-a7VfmGpooRHk4-BpeWOo_3B4psDGgGQ"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "kolik331@gmail.com"
#define USER_PASSWORD "kolik1"

#define DATABASE_URL "https://sensordata-4833d-default-rtdb.europe-west1.firebasedatabase.app",

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

DHT dht(DHTPIN, DHTTYPE);

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;
const int   daylightOffset_sec = 3600;

// Variable to save USER UID
String uid;

// Variables to save database paths
String databasePath;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 6000;

String inputWateringTime = "20";

String inputData;
String parcedData[10];
String InputStatus = "status";
float dhtTemp;
float dhtHum;

String dhtTempPath;
String dhtHumPath;
String inputWateringTimePath;
String inputStatusPath;
String inputHum1Path;
String inputHum2Path;
String inputHum3Path;
String inputHum4Path;
String inputSens1Path;
String inputSens2Path;
String inputSens3Path;
String inputSens4Path;
String inputStartTimePath;


// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

float getFloatFromBase(String path){
  if (Firebase.RTDB.getFloat(&fbdo, path)) {
      if (fbdo.dataType() == "float") {
        float floatValue = fbdo.floatData();
        Serial.print("Reading from DB - ");
        Serial.println(floatValue);
        return floatValue;
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
  }

  String getStringFromBase(String path){
  if (Firebase.RTDB.getString(&fbdo, path)) {
      if (fbdo.dataType() == "String") {
        String stringValue = fbdo.stringData();
        Serial.print("Reading from DB - ");
        Serial.println(stringValue);
        return stringValue;
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
  }

  
// Write float values to the database
void sendString(String path, String value){
  if (value.indexOf(".") > 0){
    value = value.substring(value.indexOf("."));
  }
  if (Firebase.RTDB.setString(&fbdo, path.c_str(), value)){
    Serial.print("Writing value: ");
    Serial.println (value);
    //Serial.print(" on the following path: ");
    //Serial.println(path);
    //Serial.println("PASSED");
    //Serial.println("PATH: " + fbdo.dataPath());
    //Serial.println("TYPE: " + fbdo.dataType());
  }
  else {
    Serial.println("FAILED : " + value);
    Serial.println("REASON: " + fbdo.errorReason());
  }
}
void sendFloat(String path, float value){
  if (Firebase.RTDB.setFloat(&fbdo, path.c_str(), value)){
    Serial.print("Writing value: ");
    Serial.println (value);
    //Serial.print(" on the following path: ");
    //Serial.println(path);
    //Serial.println("PASSED");
    //Serial.println("PATH: " + fbdo.dataPath());
    //Serial.println("TYPE: " + fbdo.dataType());
  }
  else {
    Serial.println("FAILED: ");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}



    float readDHT(char kind) {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  
  if (isnan(t) || isnan(h)) {    
    Serial.println("Failed to read from DHT sensor!");
    //return "-- --";
  }
  else {
    if(kind == 't'){return t;}
    if(kind == 'h'){return h;}
  }
}
  

String printLocalTime()
{
  struct tm timeinfo;
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    //return;
  }
  return(String(timeinfo.tm_hour)+" " + String(timeinfo.tm_min));
  //return ("0 "+String(timeinfo.tm_min));
}


void setup(){
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);
  initWiFi();
Serial.println("WiFi init");
delay(1000);
  // Assign the api key (required)
  config.api_key = API_KEY;


  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid;

  // Update database path for sensor readings
  inputWateringTimePath = databasePath + "/inputWateringTime"; // --> UsersData/<user_uid>/wateringTime
  inputStatusPath = databasePath + "/inputStatus";
  
  dhtTempPath = databasePath + "/dhtTemp";
  dhtHumPath = databasePath + "/dhtHum";
  
  inputHum1Path = databasePath + "/inputHum1";
  inputHum2Path = databasePath + "/inputHum2";
  inputHum3Path = databasePath + "/inputHum3";
  inputHum4Path = databasePath + "/inputHum4";

  inputSens1Path = databasePath + "/inputSens1";
  inputSens2Path = databasePath + "/inputSens2";
  inputSens3Path = databasePath + "/inputSens3";
  inputSens4Path = databasePath + "/inputSens4";

  
    dht.begin();
}

void parceInputData(String inputData){
  if(inputData.length() > 52){  
    inputData =inputData.substring(0,inputData.length()/2);
  }
  int j = 0;
  for(int i = 0; i<inputData.length(); i++){
    //Serial.println("Letters - "+String(inputData.length()));
     if(inputData.charAt(i) == ' '){
        //Serial.println("Space found at - "+String(i));
        parcedData[j] = inputData.substring(0,i);
        //Serial.println("Data at pos"+ String(j)+"="+ String(parcedData[j]));
        inputData = inputData.substring(i+1);
        //Serial.println("New string is - "+String(inputData));
        j++;
        i=0;
        }
      if(j == 10){
        break;
        }      
      }
}

void loop(){
  
   Serial2.flush();
  
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    
    Serial.println("Loop Startedd");
    
    inputData = Serial2.readString();
    Serial2.flush();
    parceInputData(inputData);
    dhtTemp = readDHT('t');
    sendFloat(dhtTempPath, dhtTemp);
    dhtHum = readDHT('h');
    sendFloat(dhtHumPath, dhtHum);
    sendString(inputStatusPath,parcedData[0]);
    sendString(inputWateringTimePath,parcedData[1]);
    
    sendFloat(inputHum1Path,parcedData[2].toFloat());
    sendFloat(inputHum2Path,parcedData[3].toFloat());
    sendFloat(inputHum3Path,parcedData[4].toFloat());
    sendFloat(inputHum4Path,parcedData[5].toFloat());

    sendFloat(inputSens1Path,parcedData[6].toFloat());
    sendFloat(inputSens2Path,parcedData[7].toFloat());
    sendFloat(inputSens3Path,parcedData[8].toFloat());
    sendFloat(inputSens4Path,parcedData[9].toFloat());

    String output = getStringFromBase(databasePath+"/outputWateringTime") + " " 
                  + getStringFromBase(databasePath+"/outputValveStart")+ " "
                  + getStringFromBase(databasePath+"/outputSens1") + " "
                  + getStringFromBase(databasePath+"/outputSens2") + " "
                  + getStringFromBase(databasePath+"/outputSens3") + " "
                  + getStringFromBase(databasePath+"/outputSens4") + " "
                  + printLocalTime() + " "
                  + getStringFromBase(databasePath+"/outputStartTime").substring(0,2);
                  
  Serial.println(output); 
    Serial2.println(output);


  }
}
