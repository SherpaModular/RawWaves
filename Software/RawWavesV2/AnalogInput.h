#ifndef AnalogInput_h
#define AnalogInput_h

#include <Arduino.h>

#define ADC_BITS 10	// Teensy 4.0 onboard ADC only supports 8, 10 and 12 bit resolutions. 10-bit accuracy. See Chapter 66, https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf 
					// If you change this, also update AudioEngine::skipTo in AudioEngine.cpp, as it uses this value to convert the ADC value to a sample position.
#define ADC_MAX_VALUE (1 << ADC_BITS) // = 1024 for 10-bit ADC.

#define START_VALUE -100

class AnalogInput {
	public:
		AnalogInput(int pinIndex);
		boolean update();
		void setRange(float outLow, float outHigh, boolean quantise);
		void setAverage(boolean avg);
		void setSmoothSteps(int steps);

		void printDebug();
		float getRatio();
		float currentValue = START_VALUE;
		int32_t inputValue = 0;
		uint16_t borderThreshold = 16;
	private:
		int pin;
		float outputLow = 0.0;
		float outputHigh = 1.0;
		float inToOutRatio = 0.0;
		float inverseRatio = 0.0;

		// Set out of range to trigger a change status on first call.
		int32_t oldInputValue = -1000;
		int32_t valueAtLastChange = -1000;

		// Use hysteresis thresholds at value boundaries
		boolean hysteresis = false;

		// Clamp output to int
		boolean quantise = false;

		// Smooth input
		boolean average = false;
		uint8_t smoothSteps = 40;
};

#endif
