#include "LowPower.h"

void setup() {
  
  //Set up default build in led
  pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {

  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);

  //Change the duration of the sleep mode by adjusting the first parameter
  //LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF); //Normal sleep mode
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); //Deep sleep mode
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(8000);                       // wait for 8 second

}

//Power saving mini experiments
//On normal case, when led is lit, consume average of 0.223W to 0.238W & 110.2ohm to 110.5ohm

//LowPower.powerDown (save the most power)
//When device put to power down, consume average of 0.144W & 171.2ohm to 184.1ohm

//LowPower.idle
//When device put to idle, consume average of 0.173W to  & 141.8 ohm to 141.9 ohm
