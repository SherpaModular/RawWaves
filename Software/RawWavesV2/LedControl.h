#ifndef LedControl_h
#define LedControl_h

/**
 * Raw Waves LED control
 *
 * 4 LEDs across the top and 1 reset LED.
 *
 */
#define LED0 2 // was 6 before PCB change
#define LED1 3 // was 5 before PCB change
#define LED2 4
#define LED3 5 // was 3 before PCB change
#define RESET_LED 6 // Reset LED indicator. Was 11 in Teensy 3.2 but conflicts with the SD card in Teensy 4 // was 2 before PCB change

class LedControl {

	public:
		void init();
		void single(int index);
		void multi(uint8_t bits);
		void showReset(boolean high);

};
#endif
