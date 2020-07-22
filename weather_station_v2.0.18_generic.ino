/*
 * Created by: David Windham
 * 
 * Arduino board used: ESP8266
 * Sensors used:  DHT22
 *                BMP180
 *                LDR
 * 
 * V0.1 - Basic reading in from DHT11 established. Data is transmitted as a CSV via serial to be read in and stored in excel spreadsheet using python 
 * V0.2 - Switched to wifi arduino, switched to DHT22, data is now sent to python-flask server and stored in SQL server.
 * V0.3 - LDR Light sensor added
 * V1.0 - Data is now sent as a JSON rather than CSV. Code has been optimized, still more work to do
 * V2.0 - Big jump, BMP180 pressure sensor added. Code has been heavily tidied up, wifi credentials abstracted, loop runs off declared variables
 * V2.1 - more tidying, more abstraction. Full comments added
 *        removal of a lot of old code that wasn't doing much and replaced with better written code
 */

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_BMP085.h>



// DHT22 Temperature/Humidity/Heat Index
#define DHTPIN 14     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22 decleration
DHT dht(DHTPIN, DHTTYPE);  // init the DHT22


// BMP180 Pressure/Temperature-redundant definition
// sets up D1 and D2 pins as SDA/SCL for the BMP180 sensor
Adafruit_BMP085 bmp;
#define BMPSDA
#define BMPSCL


// LDR Light Sensor Definition
int sensor_pin = A0; // select the input pin for LDR
int sensor_value = 0; // variable to store the value coming from the sensor


// WIFI credentials
String wifi_name = "ENTER_WIFI_NAME_HERE";
String wifi_pass = "ENTER_WIFI_PASSWORD_HERE";
String host_ip =  "http://ENTER_LOCAL_IP_HERE";
String host_port = "ENTER_PORT_HERE(default is often 5000)";
String path_component = "postjson";
String http_constructed = host_ip + ":" + host_port + "/" + path_component;


// Sensor read loop settings (in main 'loop' function)
int itteration_counter = 58;
int loop_delay = 50;


void setup() {
  Serial.begin(9600);
  
  WiFi.begin(wifi_name, wifi_pass);  // change what's here to your wifi name/password
  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion
    delay(500);
    Serial.println("Waiting for connection");
  }

  // Sensor init
  dht.begin();  // begins DHT logging
  if (!bmp.begin()) //checks to see if the BMP180 is present
  {
    Serial.println("Could not find BMP180 or BMP085 sensor at 0x77");
    while (1) {}
  }
  
  //End of setup, loop will now run
}

float get_average(float passed_total){ // function calculates the average
  return(passed_total/itteration_counter);
}

void send_JSON(float temperature, float humidity, float heat_index, float light, float pressure, float bmp_temperature){
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;    //Declare object of class HTTPClient
    
    http.begin(http_constructed);  // begins communication with the webserver. Adjust IP address here to your local server
    http.addHeader("Content-Type", "application/json");  // specify content-type header
    
    // here the JSON is constructed (bodged) as a string. I believe there is a JSON library for arduino but this works fine
    int http_code = http.POST("{\"temp\":" + String(temperature) + ", \"humidity\":" + String(humidity) + ", \"heatIndex\":" + String(heat_index) + ", \"light\":" + String(light) + ", \"pressure\":" + String(pressure) + ", \"bmp_temp\":" + String(bmp_temperature) + "}");
    
    Serial.println(http_code); // 200 = good. Anything else = bad
    http.end();  // closes the connection
  } else {
    Serial.println("Error in WiFi connection");
  }
}

void loop() {
  // inits variables to be used to calculate averages
  int total_light = 0;
  float total_humidity = 0.0;
  float total_temperature = 0.0;
  float bmp_total_temperature = 0.0;
  float bmp_total_pressure = 0.0;

  // for loop captures 58 pieces of data over 2.9 seconds. Adjust the delay or itter counter to your hearts content where the variables are declared above the setup function
  for(int i = 0; i < itteration_counter; i++){
    sensor_value = analogRead(sensor_pin);
    total_light += sensor_value;
    total_humidity += dht.readHumidity();
    total_temperature += dht.readTemperature();
    bmp_total_temperature += bmp.readTemperature();
    bmp_total_pressure += bmp.readPressure();
    // waits a short period before taking the next reading
    delay(loop_delay);
  }

  // averages out all the different variables
  float light = get_average(total_light);
  float humidity = get_average(total_humidity);
  float temperature = get_average(total_temperature);
  float bmp_temperature = get_average(bmp_total_temperature);
  float bmp_pressure = get_average(bmp_total_pressure);

  // calculates heat index, which is a LUT (Look Up Table) that uses temperature and humidity to calculate what temperature it feels like
  float heat_index = dht.computeHeatIndex(temperature, humidity, false);

  // passes the averages to the function sendJSON that, as you can guess, sends a JSON to the webserver
  send_JSON(temperature, humidity, heat_index, light, bmp_pressure, bmp_temperature);
}
