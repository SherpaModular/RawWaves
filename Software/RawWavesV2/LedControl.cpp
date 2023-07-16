#include <Arduino.h>
#include "LedControl.h"

void LedControl::init() {
	pinMode(RESET_LED, OUTPUT);
    pinMode(LED0,OUTPUT);
    pinMode(LED1,OUTPUT);
    pinMode(LED2,OUTPUT);
    pinMode(LED3,OUTPUT);
}

// Set any combination of top row LEDS from the bottom 4 bits
// bit position is the opposite of light position
// e.g. if bottom 4 bits are 0001 the LEDs are * O O O
void LedControl::multi(uint8_t bits) {
    digitalWrite(LED3, HIGH && (bits & 1));
    digitalWrite(LED2, HIGH && (bits & 2));
    digitalWrite(LED1, HIGH && (bits & 4));
    digitalWrite(LED0, HIGH && (bits & 8));
}

// Set a single LED on the top row, others will be unlit
void LedControl::single(int index) {
    digitalWrite(LED3, HIGH && (index==0));
    digitalWrite(LED2, HIGH && (index==1));
    digitalWrite(LED1, HIGH && (index==2));
    digitalWrite(LED0, HIGH && (index==3));
}

void LedControl::showReset(boolean high) {
    digitalWrite(RESET_LED, high ? HIGH : LOW);
}
