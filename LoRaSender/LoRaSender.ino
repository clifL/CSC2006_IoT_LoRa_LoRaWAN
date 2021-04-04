#include <SPI.h>
#include <LoRa.h>
#include "LowPower.h"
#include <dht.h>    //Include the DHT library for the temperature - humidity sensor
#define TRIG_PIN 4  //Connect TRIG pin to digital pin 4
#define ECHO_PIN 3  //Connect ECHO pin to digital pin 5
#define dht_apin A0 //Connect Signal pin from DHT11 sensor to analog pin A0
dht DHT;            //Create a DHT object

int counter = 0;
char charMsg[100]={"\0"};

// Constant 189W - 596W (0.037A ~ 0.116A) in Non-Low Power Mode
// Constant 107W - 122W (0.021A ~ 0.024A)  on LowPower.powerDown Mode and constant 327W - 596W (0.075A - 0.116A) upon wake up and processing

bool lowPowerMode = true;
bool debugMode = true;

float dataset[3] = { }; 
String sTemp = "";  
String sHumidity = "";  
String sFullness = "";

void setup() {
  // Set up default build in led
  pinMode(LED_BUILTIN, OUTPUT);
  // Turn the Ardunio Uno LED off to save power
  digitalWrite(LED_BUILTIN, LOW);

  // DHT Configuration
  pinMode(TRIG_PIN, OUTPUT);                        //Set the TRIG pin to OUTPUT mode
  pinMode(ECHO_PIN, INPUT);                         //Set the ECHO pin to INPUT mode
  
  Serial.begin(9600);
  delay(100);
  while (!Serial);
  
  // LoRa Configuration
  if (!LoRa.begin(915600000)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSyncWord(0x34);

  Serial.println("Program Began");

}

void loop() {
  // Activate Low Power Mode
  if (lowPowerMode) {
    //Change the duration of the sleep mode by adjusting the first parameter
    int sleepDuration = rand() % 4 + 1;
    switch(sleepDuration) {
      case 1:
        LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF); //Deep sleep mode
        break;
      case 2:
        LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF); //Deep sleep mode
        break;
      case 3:
        LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF); //Deep sleep mode
        break;
      default:
        LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF); //Deep sleep mode
    }    
  }

  // Check for any incoming packets
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet '");

    // read packet
    int counter = 0;    
    while (LoRa.available()) {
      charMsg[counter] = (char)LoRa.read();
      Serial.print(charMsg[counter]);
      counter++;
    }
    // Relay the msg for 20 times to ensure the other node receives it
    for (int q = 0; q < 20; q++) {
      LoRa.beginPacket();
      LoRa.print(charMsg);
      LoRa.endPacket();
      LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);
    }
    memset(charMsg, 0, sizeof(charMsg));
  }

  // Call getInput()s to get Ultrasonic, Temperature and Humidity data
  getInputs();

  // Initialise
  String sHumidity = "";
  String sFullness = "";
  String sTemp = "";

  // Conversion Temperature
  sTemp+=String(int(dataset[2]));
  //+ "."+String(getDecimal(dataset[2])); //combining both whole and decimal part in string with a fullstop between them
  char charTemp[sTemp.length()+1];                      //initialise character array to store the values
  sTemp.toCharArray(charTemp,sTemp.length()+1);     //passing the value of the string to the character array


  // Conversion Humidity
  sHumidity+=String(int(dataset[1]));
  //+ "."+String(getDecimal(dataset[1])); //combining both whole and decimal part in string with a fullstop between them
  char charHumidity[sHumidity.length()+1];                      //initialise character array to store the values
  sHumidity.toCharArray(charHumidity,sHumidity.length()+1);     //passing the value of the string to the character array

  // Conversion Ultrasonic
  sFullness+=String(int(dataset[0]));
  //+ "."+String(getDecimal(dataset[0])); //combining both whole and decimal part in string with a fullstop between them
  char charFullness[sFullness.length()+1];                      //initialise character array to store the values
  sFullness.toCharArray(charFullness,sFullness.length()+1);     //passing the value of the string to the character array

  // Send message 5 times to nearby node
  for (int q = 0; q < 5; q++) {
      LoRa.beginPacket();
      LoRa.print(charTemp);
      LoRa.print(",");
      LoRa.print(charHumidity);
      LoRa.print(",");
      LoRa.print(charFullness);
      LoRa.print(",1");
      LoRa.endPacket();
  }
  
  if (debugMode) {
    Serial.println("##Data sent##");
    delay(100);
    for(uint8_t i=0; i<sizeof(charTemp);i++) Serial.print(charTemp[i]);
    Serial.print(",");
    for(uint8_t i=0; i<sizeof(charTemp);i++) Serial.print(charHumidity[i]);
    Serial.print(",");
    for(uint8_t i=0; i<sizeof(charTemp);i++) Serial.print(charFullness[i]);
    Serial.print(",1");
    delay(100);
    Serial.println(" ");
    delay(100);
  }
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
 dataset[0] = fullLevel(cm);
 dataset[1] = humidity;
 dataset[2] = temp;
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
  long decPart = 1000*(val-intPart); //I am multiplying by 1000 assuming that the foat values will have a maximum of 3 decimal places. 
                                    //Change to match the number of decimal places you need
  if(decPart>0)return(decPart);           //return the decimal part of float number if it is available 
  else if(decPart<0)return((-1)*decPart); //if negative, multiply by -1
  else if(decPart=0)return(00);           //return 0 if decimal part of float number is not available
}
