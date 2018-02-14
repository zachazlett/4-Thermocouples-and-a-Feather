// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Cobbled together by Zac Hazlett with code from the libraries below and their included examples.
// 
//
// All text above must be included in any redistribution.

// I used this thermocouple board. http://www.playingwithfusion.com/productview.php?pdid=72&catid=1001
// and this Arduino board. https://www.adafruit.com/product/2821

//Make sure you have the following libraries and drivers installed:
//#1 https://github.com/esp8266/Arduino
//#2 Adafruit_MAX31856
//#3 Adafruit_MQTT
//#4 WiFiClient
//#5 https://github.com/tzapu/WiFiManager
//#6 http://www.pushetta.com/


/***********Include These*************/

#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_MAX31856.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>     


/************************* Adafruit.io Setup *********************************/
// You'll need to setup an account at http://io.adafruit.com.
// Enter you Adafruit IO Username and Key Below:
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "Username"
#define AIO_KEY         "Key"


/************************* Pushetta Setup *********************************/
// You'll need to setup an account at www.pushetta.com and download the iOS app to have
// push notifications and alarms.

// Enter your Pushetta Key and Channel name Below:
char APIKEY[] = ("Key"); // Put here your API key. It's in the dashboard
char CHANNEL[] = "Channel Name";                               // and here your channel name

char serverName[] = "api.pushetta.com";
boolean lastConnected = false;

void sendToPushetta(char channel[], String text) {
  WiFiClient client;
  client.stop();

  if (client.connect(serverName, 80))
  {
    client.print("POST /api/pushes/");
    client.print(channel);
    client.println("/ HTTP/1.1");
    client.print("Host: ");
    client.println(serverName);
    client.print("Authorization: Token ");
    client.println(APIKEY);
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(text.length() + 46);
    client.println();
    client.print("{ \"body\" : \"");
    client.print(text);
    client.println("\", \"message_type\" : \"text/plain\" }");
    client.println();
  } 
  }

/************************* Setup SPI and pins *********************************/
  // use hardware SPI, just pass in the CS pin
  Adafruit_MAX31856 max0 = Adafruit_MAX31856(4);
  Adafruit_MAX31856 max1 = Adafruit_MAX31856(5);
  Adafruit_MAX31856 max2 = Adafruit_MAX31856(16);
  Adafruit_MAX31856 max3 = Adafruit_MAX31856(2);


  
/************************* Set Variables to be used later *************************/
  double tmp0;
  double F0;
  double tmp1;
  double F1;
  double tmp2;
  double F2;
  double tmp3;
  double F3;
  int lastSensorState0 = 0;
  int lastSensorState1 = 0;
  int lastSensorState2 = 0;
  int lastSensorState3 = 0;
  int lastSensorState4 = 0;
  int sensorState0;
  int sensorState1;
  int sensorState2;
  int sensorState3;
  int sensorState4;
  // We will set starting alarm values below. If you want them hardcored put the values here.
  // They will be changed on the io.adafruit.com site later.
  int t0almval = 1000;
  int t1almval = 1000;
  int t2almval = 1000;
  int t3almval = 1000;
  int t4almval = 1000;


/****************************** Adafruit IO Feeds Subscribe and Publish ***************************************/
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Setup the feeds for 4 temperatures and 5 alarms for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish t0 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/t0");
Adafruit_MQTT_Publish t1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/t1");
Adafruit_MQTT_Publish t2 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/t2");
Adafruit_MQTT_Publish t3 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/t3");
Adafruit_MQTT_Subscribe t0alm = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/t0alm");
Adafruit_MQTT_Subscribe t1alm = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/t1alm");
Adafruit_MQTT_Subscribe t2alm = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/t2alm");
Adafruit_MQTT_Subscribe t3alm = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/t3alm");
Adafruit_MQTT_Subscribe t4alm = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/t4alm");

  
// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
  void MQTT_connect();


 
void setup() {
  
  
  /************************* Wifi Setup *********************************/
  // This code will run the WifiManager Library and let you chose your SSID to connect to
  // if it can't find the last connected SSID.
  
  // put your setup code here, to run once:
    delay(5000);
    Serial.begin(115200);

    //WiFiManager
    //Local initialization. Once its business is done, there is no need to keep it around
    delay(5000);
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    delay(5000);
    wifiManager.autoConnect("AutoConnect");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();

    
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    delay(20000);
    
    //This will push a notification once Wifi is connected.
    Serial.println("Connecting to Pushetta");
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect("api.pushetta.com", httpPort)) {
    Serial.println("Connection failed");
    return;
  }

    sendToPushetta(CHANNEL, "Wifi is Alive!");
    delay(20000);


//******************MQTT Subscribe Setup******************************/
mqtt.subscribe(&t0alm);
mqtt.subscribe(&t1alm);
mqtt.subscribe(&t2alm);
mqtt.subscribe(&t3alm);
mqtt.subscribe(&t4alm);

  
delay(10);

  
/******************Thermocouple Setup******************************/
  max0.begin();

  max0.setThermocoupleType(MAX31856_TCTYPE_K);

  Serial.print("Thermocouple type: ");
  switch ( max0.getThermocoupleType() ) {
    case MAX31856_TCTYPE_B: Serial.println("B Type"); break;
    case MAX31856_TCTYPE_E: Serial.println("E Type"); break;
    case MAX31856_TCTYPE_J: Serial.println("J Type"); break;
    case MAX31856_TCTYPE_K: Serial.println("K Type"); break;
    case MAX31856_TCTYPE_N: Serial.println("N Type"); break;
    case MAX31856_TCTYPE_R: Serial.println("R Type"); break;
    case MAX31856_TCTYPE_S: Serial.println("S Type"); break;
    case MAX31856_TCTYPE_T: Serial.println("T Type"); break;
    case MAX31856_VMODE_G8: Serial.println("Voltage x8 Gain mode"); break;
    case MAX31856_VMODE_G32: Serial.println("Voltage x8 Gain mode"); break;
    default: Serial.println("Unknown"); break;
  }
  
  max1.begin();

  max1.setThermocoupleType(MAX31856_TCTYPE_K);

  Serial.print("Thermocouple type: ");
  switch ( max1.getThermocoupleType() ) {
    case MAX31856_TCTYPE_B: Serial.println("B Type"); break;
    case MAX31856_TCTYPE_E: Serial.println("E Type"); break;
    case MAX31856_TCTYPE_J: Serial.println("J Type"); break;
    case MAX31856_TCTYPE_K: Serial.println("K Type"); break;
    case MAX31856_TCTYPE_N: Serial.println("N Type"); break;
    case MAX31856_TCTYPE_R: Serial.println("R Type"); break;
    case MAX31856_TCTYPE_S: Serial.println("S Type"); break;
    case MAX31856_TCTYPE_T: Serial.println("T Type"); break;
    case MAX31856_VMODE_G8: Serial.println("Voltage x8 Gain mode"); break;
    case MAX31856_VMODE_G32: Serial.println("Voltage x8 Gain mode"); break;
    default: Serial.println("Unknown"); break;
  }
  
  max2.begin();

  max2.setThermocoupleType(MAX31856_TCTYPE_K);

  Serial.print("Thermocouple type: ");
  switch ( max2.getThermocoupleType() ) {
    case MAX31856_TCTYPE_B: Serial.println("B Type"); break;
    case MAX31856_TCTYPE_E: Serial.println("E Type"); break;
    case MAX31856_TCTYPE_J: Serial.println("J Type"); break;
    case MAX31856_TCTYPE_K: Serial.println("K Type"); break;
    case MAX31856_TCTYPE_N: Serial.println("N Type"); break;
    case MAX31856_TCTYPE_R: Serial.println("R Type"); break;
    case MAX31856_TCTYPE_S: Serial.println("S Type"); break;
    case MAX31856_TCTYPE_T: Serial.println("T Type"); break;
    case MAX31856_VMODE_G8: Serial.println("Voltage x8 Gain mode"); break;
    case MAX31856_VMODE_G32: Serial.println("Voltage x8 Gain mode"); break;
    default: Serial.println("Unknown"); break;
  }

  max3.begin();

  max3.setThermocoupleType(MAX31856_TCTYPE_K);

  Serial.print("Thermocouple type: ");
  switch ( max3.getThermocoupleType() ) {
    case MAX31856_TCTYPE_B: Serial.println("B Type"); break;
    case MAX31856_TCTYPE_E: Serial.println("E Type"); break;
    case MAX31856_TCTYPE_J: Serial.println("J Type"); break;
    case MAX31856_TCTYPE_K: Serial.println("K Type"); break;
    case MAX31856_TCTYPE_N: Serial.println("N Type"); break;
    case MAX31856_TCTYPE_R: Serial.println("R Type"); break;
    case MAX31856_TCTYPE_S: Serial.println("S Type"); break;
    case MAX31856_TCTYPE_T: Serial.println("T Type"); break;
    case MAX31856_VMODE_G8: Serial.println("Voltage x8 Gain mode"); break;
    case MAX31856_VMODE_G32: Serial.println("Voltage x8 Gain mode"); break;
    default: Serial.println("Unknown"); break;
  }


}


 void loop() {
  
  
 /************ Thermocouple Read and Convert ************/
 
 //This code will read the thermocouple values and convert them to Fahrenheit if you need them.
 // tmp is celsius and F is Fahrenheit.
  tmp0 = max0.readThermocoupleTemperature();
  F0 = (tmp0 * 1.8) + 32;
  
  tmp1 = max1.readThermocoupleTemperature();
  F1 = (tmp1 * 1.8) + 32;
  

  tmp2 = max2.readThermocoupleTemperature();
  F2 = (tmp2 * 1.8) + 32;
  
  
  tmp3 = max3.readThermocoupleTemperature();
  F3 = (tmp3 * 1.8) + 32;



 MQTT_connect();

// This delay keeps you from overloading Adafruit IO
 delay(1200);
  /************ Thermocouple Value Publish ************/


  // This code will get the values print them to Serial for debugging 
  // Then it will publish them to Adafruit IO to be used on the gauges.
  // Change the F# to tmp# for celsius. 
  
  //Thermocouple #0
  Serial.print(F("\nSending t0 val "));
  Serial.print(F0);
  Serial.print("...");
  
  if (! t0.publish(F0)) {
  
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
  
  //Thermocouple #1
  Serial.print(F("\nSending t1 val "));
  Serial.print(F1);
  Serial.print("...");
  
  if (! t1.publish(F1)) {
    
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  } 
  
  //Thermocouple #2
  Serial.print(F("\nSending t2 val "));
  Serial.print(F2);
  Serial.print("...");
  
  if (! t2.publish(F2)) {
    
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
  
  //Thermocouple #3
  Serial.print(F("\nSending t3 val "));
  Serial.print(F3);
  Serial.print("...");
  
  if (! t3.publish(F3)) {
    
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }  
  

    
 
 
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */
  



 /************ Alarm Code ************/
  
  //This code will read the current alarm set points. You'll need to change them on io.adafruit.com after each reboot so they update.
  
 
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(4500))) {
    
    if (subscription == &t0alm) {
      Serial.print(F("t0alm: "));
      Serial.println((char *)t0alm.lastread);
     t0almval = atoi((char *)t0alm.lastread);  // convert to a number
    } 
    if (subscription == &t1alm) {
      Serial.print(F("t1alm: "));
      Serial.println((char *)t1alm.lastread);
    t1almval = atoi((char *)t1alm.lastread);  // convert to a number
    }
    if (subscription == &t2alm) {
      Serial.print(F("t2alm: "));
      Serial.println((char *)t2alm.lastread);
     t2almval = atoi((char *)t2alm.lastread);  // convert to a number
    }
    if (subscription == &t3alm) {
      Serial.print(F("t3alm: "));
      Serial.println((char *)t3alm.lastread);
     t3almval = atoi((char *)t3alm.lastread);  // convert to a number
    }  
    if (subscription == &t4alm) {
      Serial.print(F("t4alm: "));
      Serial.println((char *)t4alm.lastread);
     t4almval = atoi((char *)t4alm.lastread);  // convert to a number
    }  
  
  }
  //This is debugging and making sure you're getting the alarm set points from Adafruit IO.
  Serial.println(lastSensorState0);
  Serial.println(t0almval);
  Serial.println(t1almval);
  Serial.println(t2almval);
  Serial.println(t3almval);
  Serial.println(t4almval);
  Serial.println(sensorState0);

  //Set these to the thermocouples of your choosing and pick if you want to use Fahrenheit or Celsius.
  sensorState0 = F0;
  sensorState1 = F1;
  sensorState2 = F2;
  sensorState3 = F3;
  sensorState4 = F1;

  // Push alarm code starts here. 
  // Alarms #0-#3 will work as a high limit alarm. 
  // Alarm #4 is a low limit alarm.
  
  //Alarm #0
  
  if (sensorState0 >= t0almval) {
    // check that the previous value was below the threshold:
  if (lastSensorState0 < t0almval) {
    // the sensor just crossed the threshold
    Serial.println("Alarm #0");
    
    // Push notification setup and send.
    Serial.println("Connecting to Pushetta");
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect("api.pushetta.com", httpPort)) {
    Serial.println("Connection failed");
    return;
  }
  // Put in your message below.
  sendToPushetta(CHANNEL, "Alarm #1");
  delay(20000);
     }
  }
  // save state for next comparison:
  lastSensorState0 = sensorState0;



  //Alarm #1
  
  if (sensorState1 >= t1almval) {
    // check that the previous value was below the threshold:
  if (lastSensorState1 < t1almval) {
    // the sensor just crossed the threshold
    Serial.println("Sensor #1 crossed the threshold");
    
    // Push notification setup and send.
    Serial.println("Connecting to Pushetta");
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect("api.pushetta.com", httpPort)) {
    Serial.println("Connection failed");
    return;
  }
  // Put in your message below.
  sendToPushetta(CHANNEL, "Alarm #1");
  delay(20000);
     }
  }
  // save state for next comparison:
  lastSensorState1 = sensorState1;



  //Alarm #2
  
  if (sensorState2 >= t2almval) {
    // check that the previous value was below the threshold:
  if (lastSensorState2 < t2almval) {
    // the sensor just crossed the threshold
    Serial.println("Sensor #2 crossed the threshold");
    
    // Push notification setup and send.
    Serial.println("Connecting to Pushetta");
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect("api.pushetta.com", httpPort)) {
    Serial.println("Connection failed");
    return;
  }
  // Put in your message below.
  sendToPushetta(CHANNEL, "Alarm #2");
  delay(20000);
     }
  }
  // save state for next comparison:
  lastSensorState2 = sensorState2;
 
  
  
  //Alarm #3
  
  if (sensorState3 >= t3almval) {
    // check that the previous value was below the threshold:
  if (lastSensorState3 < t3almval) {
    // the sensor just crossed the threshold
    Serial.println("Sensor #3 crossed the threshold");
    
    // Push notification setup and send.
    Serial.println("Connecting to Pushetta");
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect("api.pushetta.com", httpPort)) {
    Serial.println("Connection failed");
    return;
  }
  // Put in your message below.
  sendToPushetta(CHANNEL, "Alarm #3");
  delay(20000);
     }
  }
  // save state for next comparison:
  lastSensorState3 = sensorState3;


  //Alarm #4
  
  if (sensorState4 >= t4almval) {
    // check that the previous value was below the threshold:
  if (lastSensorState4 < t4almval) {
    // the sensor just crossed the threshold
    Serial.println("Sensor #4 crossed under the threshold");
    
    // Push notification setup and send.
    Serial.println("Connecting to Pushetta");
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect("api.pushetta.com", httpPort)) {
    Serial.println("Connection failed");
    return;
  }
  // Put in your message below.
  sendToPushetta(CHANNEL, "Alarm #4");
  delay(20000);
     }
  }
  // save state for next comparison:
  lastSensorState4 = sensorState4;
 }
  /************* MQTT Reconnect *************/
   // Function to connect and reconnect as necessary to the MQTT server.
  // Should be called in the loop function and it will take care if connecting.
 
  void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
 
  
}
  
 
  
  
 
  


  



  




  
  
  
  
  

  

