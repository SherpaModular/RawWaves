#include "Interface.h"

#include <Arduino.h>
#include <Bounce2.h>
#include "RawWaves.h"

#ifdef DEBUG_INTERFACE
#define D(x) x
#else
#define D(x)
#endif

// SETUP VARS TO STORE CONTROLS
// A separate variable for tracking reset CV only
volatile boolean resetCVHigh = false;

// Called by interrupt on rising edge, for RESET_CV pin
void resetcv() {
	resetCVHigh = true;
}

void Interface::init(int fileSize, int channels, const Settings& settings, PlayState* state) {

    analogReadRes(ADC_BITS);
	pinMode(RESET_CV, INPUT);

	// Add an interrupt on the RESET_CV pin to catch rising edges
	attachInterrupt(RESET_CV, resetcv, RISING);

	uint16_t bounceInterval = 5; // Debounce interval in milliseconds
	resetButton.attach(RESET_BUTTON, INPUT_PULLUP); // Attach the reset button to Bounce2, with a pullup resistor
	resetButton.interval(bounceInterval);
	resetButton.setPressedState(LOW);

	// make it backwards compatible with the old 10-bit cv and divider
	startCVDivider = settings.startCVDivider * (ADC_MAX_VALUE / 1024);

	pitchMode = settings.pitchMode;

    if(pitchMode) {
        quantiseRootCV = settings.quantiseRootCV;
        quantiseRootPot = settings.quantiseRootPot;

        float lowNote = settings.lowNote + 0.5;
        startCVInput.setRange(lowNote, lowNote + settings.noteRange, quantiseRootCV);
        startPotInput.setRange(0.0,48, quantiseRootPot);
        startCVInput.borderThreshold = 64;
        startPotInput.borderThreshold = 64;
    } else {
    	D(Serial.print("Set Start Range ");Serial.println(ADC_MAX_VALUE / startCVDivider););
    	startPotInput.setRange(0.0, ADC_MAX_VALUE / startCVDivider, false);
    	startCVInput.setRange(0.0, ADC_MAX_VALUE / startCVDivider, false);
        startPotInput.setAverage(true);
        startCVInput.setAverage(true);
        startCVInput.borderThreshold = 32;
        startPotInput.borderThreshold = 32;
    }

	channelPotImmediate = settings.chanPotImmediate;
	channelCVImmediate = settings.chanCVImmediate;

	startPotImmediate = settings.startPotImmediate;
	startCVImmediate = settings.startCVImmediate;

	setChannelCount(channels);

	playState = state;
	buttonTimer = 0;
	buttonHoldTime = 0;
}

void Interface::setChannelCount(uint16_t count) {
	channelCount = count;
	channelCVInput.setRange(0, channelCount - 1, true);
	channelPotInput.setRange(0, channelCount - 1, true);
	D(Serial.print("Channel Count ");Serial.println(channelCount););
}

uint16_t Interface::update() {

	// If we're in pitch mode, update the root controls, otherwise update the start controls
	uint16_t startChanged = pitchMode ? updateRootControls() : updateStartControls();

	changes = updateChannelControls();  // set changes bitmap to value of channelChanged
	changes |= startChanged;   // bitwise OR to combine changes and startChanged into changes
	changes |= updateButton(); // bitwise OR to combine changes and updateButton into changes

	if(resetCVHigh || (changes & BUTTON_SHORT_PRESS)) {
		changes |= RESET_TRIGGERED;
	}
	resetCVHigh = false;

	return changes;
}

uint16_t Interface::updateChannelControls() {

	boolean channelCVChanged = channelCVInput.update();
	boolean channelPotChanged = channelPotInput.update();

	uint16_t channelChanged = 0;

	if(channelCVChanged || channelPotChanged) {
		int channel = (int) constrain(channelCVInput.currentValue + channelPotInput.currentValue, 0, channelCount - 1);

		if (channel != playState->currentChannel) {
			D(Serial.print("Channel ");Serial.println(channel););
			playState->nextChannel = channel;
			channelChanged |= CHANNEL_CHANGED;
			if((channelPotImmediate && channelPotChanged) || (channelCVImmediate && channelCVChanged)) {
				playState->channelChanged = true;
			}
		} else {
			D(
				Serial.print("Channel change flag but channel is the same: ");
				Serial.print(channel);
				Serial.print(" ");
				Serial.print(channelCVInput.currentValue);
				Serial.print(" ");
				Serial.print(channelPotInput.currentValue);
				Serial.print(" ");
				Serial.println(playState->currentChannel);
			);
		}
	}

    return channelChanged;
}

uint16_t Interface::updateStartControls() {
	// Returns a bitmap of changes
	// no change 			0 		//binary 000000000000
	// TIME_POT_CHANGED 	1       //binary 000000000001
	// TIME_CV_CHANGED 	    1 << 1  //binary 000000000010
	// CHANGE_START_NOW 	1 << 3  //binary 000000001000

	uint16_t changes = 0;

	boolean cvChanged = startCVInput.update();
	boolean potChanged = startPotInput.update();

	if(potChanged) {
		changes |= TIME_POT_CHANGED;
		D(startPotInput.printDebug(););
		if(startPotImmediate) {
			changes |= CHANGE_START_NOW;
		}
	}

	if(cvChanged) {
		changes |= TIME_CV_CHANGED;
		D(startCVInput.printDebug(););
		if(startCVImmediate) {
			changes |= CHANGE_START_NOW;
		}
	}

	// We need to constrain the start value to the range of the ADC
	// not sure why we're doing this.
	float number_to_constain = ((startCVInput.currentValue * startCVDivider) + (startPotInput.currentValue * startCVDivider));
	float lower_end_of_range = 0;
	float upper_end_of_range = ADC_MAX_VALUE;
	start = constrain(number_to_constain, lower_end_of_range, upper_end_of_range);
	//start = constrain(((startCVInput.currentValue * startCVDivider) + (startPotInput.currentValue * startCVDivider)),0,ADC_MAX_VALUE);
 	if (changes) {
		D(
			Serial.print("  start: ");
			Serial.println(start);
		);
	}

	return changes;
}

// return bitmap of state of changes for CV, Pot and combined Note.
uint16_t Interface::updateRootControls() {
	// Returns a bitmap of changes
	// no change 			0 		//binary 000000000000
	// ROOT_CV_CHANGED		1 << 9  //binary 001000000000
	// ROOT_POT_CHANGED	    1 << 10 //binary 010000000000
	// ROOT_NOTE_CHANGED	1 << 11 //binary 100000000000
	uint16_t change = 0;

	boolean cvChanged = startCVInput.update();
	boolean potChanged = startPotInput.update();

    // early out if no changes
    if(!cvChanged && !potChanged) {
    	return change;
    }

    float rootPot = startPotInput.currentValue;
    float rootCV = startCVInput.currentValue;

    if(cvChanged) {
    	D(
    		Serial.println("CV Changed");
    	);
    	if(quantiseRootCV) {
        	rootNoteCV = floor(rootCV);
        	if(rootNoteCV != rootNoteCVOld) {
        		D(
					Serial.print("CV ");Serial.println(startCVInput.inputValue);
        		);
        		change |= ROOT_CV_CHANGED;
        	}
    	} else {
    		rootNoteCV = rootCV;
    		change |= ROOT_CV_CHANGED;
    	}
    }

    if(potChanged) {
    	D(
    		Serial.println("Pot Changed");
    	);
    	if(quantiseRootPot) {
        	rootNotePot = floor(rootPot);
        	if(rootNotePot != rootNotePotOld) {
        		D(
					Serial.print("Pot ");Serial.println(startPotInput.inputValue);
        		);
        		change |= ROOT_POT_CHANGED;
        	}
    	} else {
    		rootNotePot = rootPot;
    		change |= ROOT_POT_CHANGED;
    	}
    }

	rootNote = rootNoteCV + rootNotePot;

    // Flag note changes when the note index itself changes
    if(floor(rootNote) != rootNoteOld) {
    	change |= ROOT_NOTE_CHANGED;
    	rootNoteOld = floor(rootNote);
    }

	return change;
}

uint16_t Interface::updateButton() {
	// Returns a bitmap of the state of the button
	// no change 			0 		//binary 000000000000
	// BUTTON_SHORT_PRESS 	1 << 4  //binary 000000010000
	// BUTTON_LONG_PRESS 	1 << 5  //binary 000000100000
	// BUTTON_LONG_RELEASE  1 << 6  //binary 000001000000
	// BUTTON_PULSE 		1 << 7  //binary 000010000000 Note: A BUTTON_PULSE is a long press that is held for longer than LONG_PRESS_DURATION

	resetButton.update(); //Bounce does not use interrupts. So we need to call update() regularly.
	uint16_t buttonState = 0;

	// Button pressed. Start tracking it.
	if(resetButton.pressed()) {
		buttonTimer = 0;
	}

	// Button released. Check how long it was held for.
	if(resetButton.released()) {
        if (buttonTimer >= SHORT_PRESS_DURATION && buttonTimer < LONG_PRESS_DURATION){ // This was a SHORT_PRESS_DURATION event
        	buttonState |= BUTTON_SHORT_PRESS;
        } else if(buttonTimer > LONG_PRESS_DURATION) { // This was a LONG_PRESS_DURATION event
        	buttonState |= BUTTON_LONG_RELEASE;
        }
        buttonTimer = 0;
    }

    if(resetButton.isPressed() && buttonTimer >= LONG_PRESS_DURATION) { // If the button is being held down, and we have passed the LONG_PRESS_DURATION...
    	buttonState |= BUTTON_LONG_PRESS; // We are in the middle of a LONG_PRESS_DURATION event. May generate multiple BUTTON_PULSE events.

    	uint32_t diff = buttonTimer - LONG_PRESS_DURATION;
    	if(diff >= LONG_PRESS_PULSE_DELAY) { // If we have passed the LONG_PRESS_PULSE_DELAY, generate a BUTTON_PULSE event.
    		buttonState |= BUTTON_PULSE;
    		buttonTimer = LONG_PRESS_DURATION; // Reset the timer to the LONG_PRESS_DURATION point.
    	}
    }

    return buttonState;
}
