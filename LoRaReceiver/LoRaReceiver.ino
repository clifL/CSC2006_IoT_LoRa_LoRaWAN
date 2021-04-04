#include <SPI.h>
#include <LoRa.h>
#include "LowPower.h"
#include <SPI.h>
#include <RH_RF95.h>
#include <dht.h>    //Include the DHT library for the temperature - humidity sensor
#define TRIG_PIN 4  //Connect TRIG pin to digital pin 4
#define ECHO_PIN 3  //Connect ECHO pin to digital pin 5
#define dht_apin A0 //Connect Signal pin from DHT11 sensor to analog pin A0
dht DHT;            //Create a DHT object

// Constant 241 to 311W in Non-Low Power Mode
// Constant 176W to 311W in Low Power Mode

// Singleton instance of the radio driver
RH_RF95 rf95;
float frequency = 868.0;  //frequency settings
//dht DHT;
#define DHT11_PIN A0
float temperature,humidity,tem,hum,full,tem1,hum2,full3;
char tem_1[8]={"\0"},hum_1[8]={"\0"}, full_1[8]={"\0"}; 
char tem_2[8]={"\0"},hum_2[8]={"\0"}, full_2[8]={"\0"};
char *node_id = "<1336133>";  //From LG01 via web Local Channel settings on MQTT.Please refer <> dataformat in here.
uint8_t datasend[64];
unsigned int count = 1;
byte loraMsg[100];
char charMsg[50]={"\0"};

// Constant 226W (0.045A) at low power mode and 311W (0.062A) upon wakeup and processing with spikes of 400W+.
// Constant 311W - 336W (0.062A ~ 0.067A) at Non-Low Power Mode with spikes of 400W+.

bool lowPowerMode = true;
bool packetAvailable = false;

// Reset function for cleanup
void(* resetFunc) (void) = 0;

void setup() {
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW

  // DHT Configuration
  pinMode(TRIG_PIN, OUTPUT);                        //Set the TRIG pin to OUTPUT mode
  pinMode(ECHO_PIN, INPUT);                         //Set the ECHO pin to INPUT mode
  Serial.begin(9600);
  while (!Serial);
  if (!LoRa.begin(915600000)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
   LoRa.setSyncWord(0x34);
}

void loop() {
  // Activate Low Power Mode
  if (lowPowerMode) {
    //Change the duration of the sleep mode by adjusting the first parameter
    int sleepDuration = rand() % 2 + 1;
    switch(sleepDuration) {
      case 1:
        LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF); //Deep sleep mode
        break;
      default:
        LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF); //Deep sleep mode
    }    
  }
  packetAvailable = false;
  
  // Try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet '");

    // read packet
    int counter = 0;
    for (int k = 0; k < 100; k++) {
      loraMsg[k] = -1;
    }
    
    while (LoRa.available()) {
      //msg[counter] = (char)LoRa.read();
      loraMsg[counter] = LoRa.read();
      charMsg[counter] = (char)loraMsg[counter];
      Serial.print((char)loraMsg[counter]);
      counter++;
      packetAvailable = true;
    }
    if (packetAvailable) {
      // print RSSI of packet
      Serial.print("' with RSSI ");
      Serial.println(LoRa.packetRssi());
    
      LoRa.end();
  
      // Delimit LoRa packets
      int field = 0;
      int fieldCounter =0;
      for (int x = 0; x < 50; x++) {
        if (charMsg[x] != ',') {
          if (field == 0) {
            tem_2[fieldCounter] = charMsg[x];
          }
          else if (field == 1) {
            hum_2[fieldCounter] = charMsg[x];
          }
          else if (field ==2) {
            full_2[fieldCounter] = charMsg[x];
          }
          fieldCounter++;
        }
        if (charMsg[x] == ',') {
          field ++;
          fieldCounter = 0;
        }
      }
  
      // LoRaWAN Setup 
      if (!rf95.init())
        Serial.println(F("init failed"));
      rf95.setFrequency(frequency);
      rf95.setTxPower(13);
      rf95.setSyncWord(0x34);
      // LoRaWAN - (Field 1,2,3 is from nearby node)
      writeRelayData();
      SendData();

      // LoRaWAN - (Field 4,5,6 is from current node)
      // Get current node sensors data
      getInputs();
      writeData();
      SendData();    
      packetAvailable = false;
      resetFunc();
    }
    else {
      // LoRaWAN Setup (Field 4,5,6 is from current node)
      if (!rf95.init())
        Serial.println(F("init failed"));
      rf95.setFrequency(frequency);
      rf95.setTxPower(13);
      rf95.setSyncWord(0x34);
      getInputs();
      // LoRaWAN
      writeData();
      SendData();
      resetFunc();
    }
  }
}

void writeRelayData()
{
  char data[64] = "\0";
  for(int i = 0; i < 64; i++)
  {
     data[i] = node_id[i];
  }  
  strcat(data,"field1=");
  strcat(data,tem_2);
  strcat(data,"&field2=");
  strcat(data,hum_2);
  strcat(data,"&field3=");
  strcat(data,full_2);
  strcpy((char *)datasend,data);
}

void writeData()
{
  char data[64] = "\0";
  for(int i = 0; i < 64; i++)
  {
     data[i] = node_id[i];
  }  
  dtostrf(tem,0,0,tem_1);
  dtostrf(hum,0,0,hum_1);
  dtostrf(full,0,0,full_1);  
  strcat(data,"field4=");
  strcat(data,tem_1);
  strcat(data,"&field5=");
  strcat(data,hum_1);
  strcat(data,"&field6=");
  strcat(data,full_1);
  strcpy((char *)datasend,data); 
}

void SendData()
{
      Serial.println(F("Sending data to LG01"));
      rf95.send((char *)datasend,sizeof(datasend));  
      rf95.waitPacketSent();  // Now wait for a reply
    
      uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);

     if (rf95.waitAvailableTimeout(5000)) { 
      // Should be a reply message for us now   
      if (rf95.recv(buf, &len)) {
          Serial.print("got reply from LG01: ");
          Serial.println((char*)buf);
          Serial.print("RSSI: ");
          Serial.println(rf95.lastRssi(), DEC);    
        }
      else {
        Serial.println("recv failed");
      }
     }
  else
  {
    Serial.println("No reply, is LoRa server running?");
  }
  delay(5000);
}

void getInputs() {
 DHT.read11(dht_apin);           //Read the data from the DHT sensor
 float p = 101000;               //Set atmospheric pressure to 101.000 kPa
 float temp = DHT.temperature;   //Get temperature from sensor  
 float humidity = DHT.humidity;  //Get humidity from sensor
 //Use the fromula from http://gsd.ime.usp.br/~yili/SpeedOfSound/Speed.html to evaluate speed of sound
 float a0 = 331.5024;
 float a1 = 0.603055;
 float a2 = -0.000528;
 float a3 = 51.471935;
 float a4 = 0.1495874;
 float a5 = -0.000782;
 float a6 = -1.82e-7;       
 float a7 = 3.73e-8;         
 float a8 = -2.93e-10;     
 float a9 = -85.20931;
 float a10 = -0.228525;
 float a11 = 5.91e-5;  
 float a12 = -2.835149;
 float a13 = -2.15e-13; 
 float a14 = 29.179762;
 float a15 = 0.000486;
 float T = temp + 273.15;
 float h = humidity /100.0;
 float f = 1.00062 + 0.0000000314 * p + 0.00000056 * temp * temp;
 float Psv = exp(0.000012811805 * T * T - 0.019509874 * T + 34.04926034 - 6353.6311 / T);
 float Xw = h * f * Psv / p;
 float c = 331.45 - a0 - p * a6 - a13 * p * p;
 c = sqrt(a9 * a9 + 4 * a14 * c);
 float Xc = ((-1) * a9 - c) / ( 2 * a14);
 float speedOfSound = a0 + a1 * temp + a2 * temp * temp + (a3 + a4 * temp + a5 * temp * temp) * Xw + (a6 + a7 * temp + a8 * temp * temp) * p + (a9 + a10 * temp + a11 * temp * temp) * Xc + a12 * Xw * Xw + a13 * p * p + a14 * Xc * Xc + a15 * Xw * p * Xc;
 //Send a short (10 microseconds) ultrasonic burst 
 digitalWrite(TRIG_PIN, HIGH);
 delayMicroseconds(10);
 digitalWrite(TRIG_PIN, LOW);
 float microseconds = pulseIn(ECHO_PIN, HIGH, 100000); //Mesure the duration of a HIGH pulse in echo pin in microseconds. Timeout in 0,1 seconds
 float seconds = microseconds / 1000000;               //Convert microseconds to seconds
 float meters = seconds * speedOfSound;                //Get the distance in meters using the speed of sound calculated earlier
 float cm = meters * 100;                              //Convert meters to cm
 cm = cm/2;                                            //We only want the distance to the obstacle and not the roundtrip
 tem = temp;
 hum = humidity;
 full = fullLevel(cm);
 
 if (cm <= 20){
  Serial.println("Bin Full, Please Clear Bin");
 }
 if (temp >= 50){
  Serial.println("Bin Temperature too High, Please Check");
 }
}

float fullLevel (float cm){
  float full = 15;
  if (cm > 15.0) {
    cm = full;
  }
  float fullness = ((full - cm) / full) * 100;
  if (fullness >= 0.8){
    Serial.println("Bin Full, Please Clear Bin");
  }
  return fullness;
}

//function to extract decimal part of float
long getDecimal(float val)
{
  int intPart = int(val);
  long decPart = 1000*(val-intPart);            
  if(decPart>0)return(decPart);          
  else if(decPart<0)return((-1)*decPart); 
  else if(decPart=0)return(00);           
}
