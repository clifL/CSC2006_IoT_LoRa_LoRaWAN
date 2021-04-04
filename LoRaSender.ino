#include <SPI.h>
#include <LoRa.h> // reference to the LoRa library

int counter = 0;

void setup() {

  // opens serial port, sets data rate to 9600 bps
  Serial.begin(9600);
  
  while (!Serial);//waits for an active serial connection 

  Serial.println("LoRa Sender");

  
/***************************************************************************************
* initialize the LoRa radio on the shield with LoRa.begin                                *
* (915600000) represent the operating freq for SG                                        *
* The syncwords used for public networks such as LoRaWAN\TTN are 0x34 for SX127x devices *
* and 0x3444 for SX126x devices.                                                         *
* SX127x devices are the Arduino Shield devices                                          *
*                                                                                        *
*                                                                                        *
****************************************************************************************/
  if (!LoRa.begin(915600000)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSyncWord(0x34);
}

/***************************************************************************************
* Voidloop function()                                                                    *
* transmit the LoRa packet containing the sensor value using LoRa.beginPacket, LoRa.print*
* and LoRa.endPacket                                                                     *
* BeginPacket() Start the sequence of sending a packet.                                  *
* LoRa.print() Write data into the packet                                                *
* endPacket() End the sequence of sending a packet.                                      *
* Delay(5000) Message will send over every 5000 miliseconds                              *
*                                                                                        *
****************************************************************************************/
void loop() {
  Serial.print("Sending packet: ");
  Serial.println(counter);

  // send packet
  LoRa.beginPacket();
  LoRa.print("hello ");
  LoRa.print(counter);
  LoRa.endPacket();

  counter++;

  delay(5000);
}
