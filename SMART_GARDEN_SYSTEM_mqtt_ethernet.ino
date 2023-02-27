/***************************************************
  Adafruit MQTT Library Ethernet Example

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Alec Moore
  Derived from the code written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <SPI.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include <Ethernet.h>
#include <EthernetClient.h>
#include <Dns.h>
#include <Dhcp.h>

/************************ Sensor library******************* */
#include <LiquidCrystal_I2C.h>
#include <dht.h>

//define an object
dht dhtSen;

//calibration values
#define sWet 300   // Define Moisture MAX value "WET"
#define sDry 600   // Define Moisture MIN value "DRY"

LiquidCrystal_I2C lcd(0x3F,16,2);
//LiquidCrystal_I2C lcd(0x27,16,2);

// Sensor pins
#define moistureSen A0
#define relayPin 6
#define DHTPin 7


/************************* Ethernet Client Setup *****************************/
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

//Uncomment the following, and set to a valid ip if you don't have dhcp available.
//IPAddress iotIP (192, 168, 0, 42);
//Uncomment the following, and set to your preference if you don't have automatic dns.
//IPAddress dnsIP (8, 8, 8, 8);
//If you uncommented either of the above lines, make sure to change "Ethernet.begin(mac)" to "Ethernet.begin(mac, iotIP)" or "Ethernet.begin(mac, iotIP, dnsIP)"


/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "enter your adafruit username"
#define AIO_KEY         "enter the key"


/************ Global State (you don't need to change this!) ******************/

//Set up the ethernet client
EthernetClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// You don't need to change anything below this line!
#define halt(s) { Serial.println(F( s )); while(1);  }


/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish tempFd = Adafruit_MQTT_Publish(&mqtt,  AIO_USERNAME "/feeds/tempFd");
Adafruit_MQTT_Publish humidFd = Adafruit_MQTT_Publish(&mqtt,  AIO_USERNAME "/feeds/humidFd");
Adafruit_MQTT_Publish moisFd = Adafruit_MQTT_Publish(&mqtt,  AIO_USERNAME "/feeds/mFd");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe pumpOnOff = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");

/*************************** Sketch Code ************************************/

void setup() {
  Serial.begin(115200);

  Serial.println(F("Adafruit MQTT demo"));

  // Initialise the Client
  Serial.print(F("\nInit the Client..."));
  Ethernet.begin(mac);
  delay(1000); //give the ethernet a second to initialize
  

  mqtt.subscribe(&pumpOnOff);

    pinMode(relayPin, OUTPUT);

    lcd.init();
    lcd.backlight();
    lcd.setCursor(2, 0);
    lcd.print("SMART GARDEN");
    lcd.setCursor(0, 1); 
    lcd.print("WATERING SYSTEM.");
    delay(3000);
    lcd.clear();
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(1000))) {
    if (subscription == &pumpOnOff) {
      Serial.print(F("Got: "));
      Serial.println((char *)pumpOnOff.lastread);
    }
  
  }

  //********** Now we can publish stuff!*********************
  
  // get the readings from DHT sensor
    int x = dhtSen.read11(DHTPin);
    float temp = dhtSen.temperature;
    float humid = dhtSen.humidity;
    

    Serial.print(temp);
    Serial.print(" c\t");
    Serial.print(humid);
    Serial.println(" %");

    //get the readings from the function below and print it
    int moisture = readSen();
    Serial.print("Soil Moisture Level: ");
    Serial.println(readSen());

    int m_per;
    m_per = ( 100 - ( (readSen()/1023.00) * 100 ) );
    Serial.print("Soil Moisture percentage: ");
    Serial.print(m_per);
    Serial.println("%");
    
    lcd.setCursor(0, 0); 
    lcd.print("S-MOISTURE:");
  
  // status of the soil
  if (moisture < sWet) {
    Serial.println("Status: Soil is too wet");
    
    lcd.setCursor(12, 0);
    //lcd.print(readSen());
    lcd.print(m_per);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("SOIL IS TOO WET ");
    delay(2000);
    lcd.setCursor(0, 1); 
    lcd.print("PUMP STATUS: OFF");

    digitalWrite(relayPin,HIGH);// switch OFF the water pump
    delay(200); 
  }
  else if (moisture >= sWet && moisture < sDry) {
    Serial.println("Status: Soil moisture is perfect");
    
    lcd.setCursor(12, 0);
    lcd.print(m_per);
    lcd.print("%");
    lcd.setCursor(0, 1); 
    lcd.print("PERFECT MOISTURE");
    delay(2000);
    lcd.setCursor(0, 1); 
    lcd.print("PUMP STATUS: OFF");
    
    digitalWrite(relayPin,HIGH);// switch OFF the water pump
    delay(200);
  
  //read the feed and respond 
  if(strcmp((char *)pumpOnOff.lastread,"1")==0){
    Serial.println("Switch ON");
    digitalWrite(relayPin,LOW);// switch ON the water pump
  }
  if(strcmp((char *)pumpOnOff.lastread,"0")==0){
    Serial.println("Switch OFF");
    digitalWrite(relayPin,HIGH);// switch OFF the water pump
  }

  } 
  else {
    Serial.println("Status: Soil is too dry - time to water!");
    
    lcd.setCursor(12, 0);
    //lcd.print(readSen());
    lcd.print(m_per);
    lcd.print("%");
    lcd.setCursor(0, 1); 
    lcd.print("SOIL IS TOO DRY ");
    delay(2000);
    lcd.setCursor(0, 1); 
    lcd.print("PUMP STATUS: ON ");
    
    digitalWrite(relayPin,LOW);// switch ON the water pump
    delay(200);

     //read the feed and respond 
    if(strcmp((char *)pumpOnOff.lastread,"1")==0){
      Serial.println("Switch ON");
      digitalWrite(relayPin,LOW);// switch ON the water pump
    }
    if(strcmp((char *)pumpOnOff.lastread,"0")==0){
      Serial.println("Switch OFF");
      digitalWrite(relayPin,HIGH);// switch OFF the water pump
    }
  }
  
  delay(1000);  // Take a reading every second
  
  Serial.println();

    float mois = moisture;

      if (! tempFd.publish(temp)) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
    }
    if (! humidFd.publish(humid)) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
    }
    if (! moisFd.publish(mois)) {
      Serial.println(F("Failed"));
    } else {
      Serial.println(F("OK!"));
    }
    delay(6000);

  // ping the server to keep the mqtt connection alive
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }

}

  //  This function returns the analog soil moisture readings
  int readSen() {
  int val = analogRead(moistureSen);  // Read the analog value form sensor
  return val;             // Return analog moisture value
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
}
