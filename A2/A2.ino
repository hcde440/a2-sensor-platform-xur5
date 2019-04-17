
// Melody Xu
// HCDE 440
// 4/16/19

// This sketch uses a DHT sensor to read humidity data for the local area and
// compares it with Seattle's humidity from open weather API 
// Data is sent to Arduino IO dashboard below
// https://io.adafruit.com/xur5/dashboards/a2

// The dashboard also controls an LED 


/************************** Configuration ***********************************/

#include "config.h"

// Including the Libraries used in the program  

#include <Adafruit_Sensor.h>    // for DHT                        
#include <SPI.h>                // for DHT     
#include <Wire.h>               // for DHT  
#include <ESP8266WiFi.h>        // for WiFi    
#include <ESP8266HTTPClient.h>  // for WiFi  
#include <ArduinoJson.h>        // for DHT
#include <DHT.h>                // for DHT
#include <DHT_U.h>              // for DHT
#include <Adafruit_GFX.h>       // for display
#include <Adafruit_SSD1306.h>   // for display
      
    
//#define SCREEN_WIDTH 128 // OLED display width, in pixels
//#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define LED_PIN 13 // sets the LED pin to pin 13

#define DATA_PIN 12 // pin connected to DH22 sensor

// create DHT22 
DHT_Unified dht(DATA_PIN, DHT22);
float hum;

// set up the 'temperature', 'humidity', and digital feeds on Adafruit IO
AdafruitIO_Feed *LED = io.feed("LED");
AdafruitIO_Feed *LocalHumidity = io.feed("LocalHumidity");
AdafruitIO_Feed *SeattleHumidity = io.feed("SeattleHumidity");

String weatherKey = MET_ID; //API key for open weather API

typedef struct { //a box that holds value for humidity
  String hum;
} MetData;

MetData conditions; // creates Metdata object called conditions

const char* ssid = "Fake Asians";
const char* pass = "samesame";

void setup() {
  // start the serial 
  Serial.begin(115200);
  delay(10);
  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));

  // Connecting to WiFi
  Serial.print("Connecting to "); Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  // connect to adafruit IO
  Serial.print("Connecting to Adafruit IO");
  io.connect();
  
  // wait for a connection
  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  
  dht.begin(); // begin dht sensor
  pinMode(LED_PIN, OUTPUT);//initializes the LED pin as a digital output
  
  Serial.println(); Serial.println("WiFi connected"); Serial.println();
  Serial.print("Your ESP has been assigned the internal IP address ");
  Serial.println(WiFi.localIP());
  
  getHum(); // calls openweather API and get conditions

  // we are connected
  Serial.println();
  Serial.println(io.statusText());

  LED->onMessage(handleMessage);  // tells Adafruit IO to send/receive messages through the feed called LED
                                  
  LED->get(); // gets the first value for the digital feed
  
}

void loop() {

  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();
  getHum();
  getLocalHum();
  

  // save humidity readings to Adafruit IO
  // save humidity to Adafruit IO
  LocalHumidity->save(hum);
  SeattleHumidity->save(conditions.hum);
  
 
  // wait 3 second
  delay(3000);
}
// Gets humidity for Seattle from OpenWeather API
void getHum() {
  HTTPClient theClient; // Creating the client for calling the API
  String apiCall = "http://api.openweathermap.org/data/2.5/weather?id=5809844";  //ID for Seattle is 5809844
  apiCall += "&APPID=";
  apiCall += weatherKey;
  apiCall += "&units=imperial";
  theClient.begin(apiCall); // Calling API using the URL
  int httpCode = theClient.GET(); // Getting http code
  if (httpCode > 0) {

    if (httpCode == HTTP_CODE_OK) {             // If the code is 200
      String payload = theClient.getString(); // Getting the string of information
      DynamicJsonBuffer jsonBuffer;                       // Storing JSON
      JsonObject& root = jsonBuffer.parseObject(payload); // Converting into JSON
      if (!root.success()) {                              // If parsing failed
        Serial.println("parseObject() failed in getHum()."); 
        return;
      }
      // Below is accessing the JSON library for the conditions
      conditions.hum = root["main"]["humidity"].as<String>();
      Serial.println("Seattle Humidity: " + conditions.hum + "%");     
    }
  }
  else {
    Serial.printf("Something went wrong with connecting to the endpoint in getHum().");
  }
}

// Collects surrounding area sensor data using DHT sensor
void getLocalHum() {
  sensors_event_t event;

  dht.humidity().getEvent(&event);
  hum = event.relative_humidity;

  Serial.print("Local Humidity: ");
  Serial.print(hum);
  Serial.println("%");
}

// this function is called whenever an 'LED' feed message
// is received from Adafruit IO. it was attached to
// the 'digital' feed in the setup() function above.
void handleMessage(AdafruitIO_Data *data) {
  Serial.print("received <- "); // message  received
  if(data->toPinLevel() == HIGH)// checks the incoming message for the pin level
    Serial.println("HIGH");
  else
    Serial.println("LOW");

  digitalWrite(LED_PIN, data->toPinLevel()); //writes the new value to the LED
}
