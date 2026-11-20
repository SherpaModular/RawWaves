#ifndef LedControl_h
#define LedControl_h

/**
 * Raw Waves LED control
 *
 * 4 LEDs across the top and 1 reset LED.
 * Names match the PCB reference designators: LED1 is leftmost, LED4 is rightmost.
 *
 */
#define LED1 5 // leftmost
#define LED2 4
#define LED3 3
#define LED4 2 // rightmost
#define RESET_LED 6 // Reset LED indicator (RESET_LED1 on PCB). Was 11 in Teensy 3.2 but conflicts with the SD card in Teensy 4

class LedControl {

	public:
		void init();
		void single(int index);
		void multi(uint8_t bits);
		void showReset(boolean high);

};
#endif
