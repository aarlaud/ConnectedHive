/*********************************************************
Author: Antoine Arlaud - New York Bee Sanctuary
**********************************************************/
 
// DHT ----------------------------------------------------------------------------------------
#include "DHT.h"
#define DHTPIN 0     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define DHTCOUNT 3      // 3 for 8Mhz bootloaders and 6 for 16 Mhz
DHT dht(DHTPIN, DHTTYPE, DHTCOUNT); // Initialize DHT sensor for normal corresponding frequency

// DHT ----------------------------------------------------------------------------------------
#include "DHT.h"
#define DHTPIN 1     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define DHTCOUNT 3      // 3 for 8Mhz bootloaders and 6 for 16 Mhz
DHT dht2(DHTPIN, DHTTYPE, DHTCOUNT); // Initialize DHT sensor for normal corresponding frequency


// Battery ------------------------------------------------------------------------------------
#define BATTERYSENSORPIN A3
float BatteryVoltage; // Store Battery value to transmit only every BATTERYSAMPLINGRATE measurements as battery
//value is not to fluctuates so quickly
int batteryReadingsCount = 0;
#define BATTERYSAMPLINGRATE 8 //8 but 0 for debug

//RFM69  --------------------------------------------------------------------------------------
#include <RFM69.h>
#include <SPI.h>
#include <LowPower.h>

#define NODEID        8    //unique for each node on same network
#define NETWORKID     101  //the same on all nodes that talk to each other
#define GATEWAYID     1
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
#define FREQUENCY   RF69_433MHZ
//#define FREQUENCY   RF69_868MHZ
//#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "xxxxxxxxxxxxxxxx" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ACK_TIME      40 // max # of ms to wait for an ack
#define LED           9  // we have LED on D9
#define SERIAL_BAUD   115200  //must be 9600 for GPS, use whatever if no GPS

boolean debug = 0;

//struct for wireless data transmission
typedef struct {		
  int       nodeID; 		//node ID (1xx, 2xx, 3xx);  1xx = basement, 2xx = main floor, 3xx = outside
  int       deviceID;		//sensor ID (2, 3, 4, 5)
  unsigned long   var1_usl; 		//uptime in ms
  float     var2_float;   	//sensor data?
  float     var3_float;		//battery condition?
} Payload;
Payload theData;

char buff[20];
byte sendSize=0;
boolean requestACK = false;
#define RFM69_SS  8
RFM69 radio(RFM69_SS);
//RFM69 radio;

//end RFM69 ------------------------------------------



void setup() {
  
  // Setting Led Pin
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  //Serial.begin(SERIAL_BAUD);
 
  // Building DHT instance
  dht.begin();
  
  //RFM69-------------------------------------------
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  
  #ifdef IS_RFM69HW
    radio.setHighPower(); //uncomment only for RFM69HW!
  #endif
  radio.encrypt(ENCRYPTKEY);
  char buff[50];
  //sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  // Serial.println(buff);
  theData.nodeID = NODEID;  //this node id should be the same for all devices in this node
  //end RFM--------------------------------------------
  
 // Serial.println("Finished setup");
  delay(1000);
  digitalWrite(LED, LOW);
  
}

void loop() {
      radio.sleep();
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); //LowPower.powerDown(SLEEP_FOREVERSLEEP_8S, ADC_OFF, BOD_OFF); 
      
      delay(250);
      readDHT();
      delay(500);
      readDHT2();
      
      batteryReadingsCount++;
      if(batteryReadingsCount > BATTERYSAMPLINGRATE){
        batteryReadingsCount = 0;
        readBatteryVoltage();
      }
      
      //check for any received packets
      if (radio.receiveDone())
      {
        if (radio.ACKRequested())
        {
          radio.sendACK();
        }
      }
      delay(100);
      
}


void readBatteryVoltage(){
//  return map(analogRead(BATTERYSENSORPIN), 0, 1023, 0, readVcc()) * 2;
  unsigned int BatteryADCValue;
  float Vcc;

  Vcc = readVcc()/1000.0;
  for(int i=0;i<4;i++){
    BatteryADCValue = analogRead(BATTERYSENSORPIN);// ignore 3 first readings due to high resistor values
  }
  BatteryADCValue = analogRead(BATTERYSENSORPIN);
  BatteryVoltage = (BatteryADCValue / 1023.0) * Vcc *2.0; //x2 as we have a voltage divider (2 1MOhm resistors to split voltage in half)
  
  
}


void readDHT(){

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    digitalWrite(LED, HIGH);
  }

  theData.deviceID = 9;
  theData.var1_usl = h;
  theData.var2_float = t; //convertedTempValue;
  theData.var3_float = BatteryVoltage;
  delay(1000); // Give time to ADC to grab value
  radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
  //radio.send(GATEWAYID, (const void*)(&theData), sizeof(theData));
  // Serial.println("Sent");
 
  delay(700);
  //digitalWrite(LED, LOW);
}


void readDHT2(){

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht2.readHumidity();
  // Read temperature as Celsius
  float t = dht2.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    digitalWrite(LED, HIGH);
  }

  theData.deviceID = 7;
  theData.var1_usl = h;
  theData.var2_float = t; //convertedTempValue;
  theData.var3_float = BatteryVoltage;
  delay(1000); // Give time to ADC to grab value
  radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
  //radio.send(GATEWAYID, (const void*)(&theData), sizeof(theData));
  // Serial.println("Sent");
 
  delay(700);
  //digitalWrite(LED, LOW);
}


long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}



