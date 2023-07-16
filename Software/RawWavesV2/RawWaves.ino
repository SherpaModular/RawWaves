/*
 RAW WAVES v2.0
 https://github.com/SherpaModular/RawWaves
 
 Teensy 4.0
 Audio out: PT8211 external DAC
 
 Bank Button: 2
 Bank LEDs 3,4,5,6
 Reset Button: 8
 Reset LED 2
 Reset CV input: 9
 Channel Pot: A9
 Channel CV: A8 // check
 Time Pot: A7
 Time CV: A6 // check

 SD Card Connections:
 SCK  13
 MISO 12
 MOSI 11
 SS   10
 
 NB: Compile using modified versions of: 
 SD.cpp (found in the main Arduino package) 
 play_sd_raw.cpp  - In Teensy Audio Library 
 play_sc_raw.h    - In Teensy Audio Library 

 */
#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
//#include <Wire.h>
#include "RawWaves.h"
#include "AudioSystemHelpers.h"
#include "Settings.h"
#include "LedControl.h"
#include "FileScanner.h"
		// which will bring:
		// #include "AudioFileInfo.h"
		// #include "WavHeaderReader.h"
		// #include "Settings.h"
#include "AudioEngine.h"
		// which will bring:
		// #include "SDPlayPCM.h"
		// #include "Settings.h"
#include "Interface.h"
		// which will bring:
		// #include "PlayState.h"
		// #include "Settings.h"
		// #include "AnalogInput.h"
#include "PlayState.h"

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

#define EEPROM_BANK_SAVE_ADDRESS 0

#define FLASHTIME 	10  	// How long do LEDs flash for?
#define SD_CARD_CHECK_DELAY 20

// //////////
// TIMERS
// //////////

elapsedMillis resetLedTimer = 0;
elapsedMillis ledFlashTimer = 0;

elapsedMillis meterDisplayDelayTimer; // Counter to hide MeterDisplay after bank change
elapsedMillis peakDisplayTimer; // COUNTER FOR PEAK METER FRAMERATE


boolean bankChangeMode = false;
File settingsFile;

Settings settings("SETTINGS.TXT");
LedControl ledControl;
FileScanner fileScanner;
AudioEngine audioEngine;
Interface interface;
PlayState playState;

int NO_FILES = 0;
uint8_t noFilesLedIndex = 0;

uint8_t rebootCounter = 0;

void setup() {

#ifdef DEBUG_STARTUP // Pauses module startup until serial connection is made
	while( !Serial );
	Serial.println("Starting");
#endif // DEBUG_STARTUP

	ledControl.init();
	ledControl.single(playState.bank);

	// SD CARD SETTINGS FOR AUDIO SHIELD
    SPI.setMOSI(11); // pin for MOSI, was 7 in Teensy 3.2 version
    SPI.setSCK(13); // pin for SCK, was 14 in Teensy 3.2 version

	boolean hasSD = openSDCard();
	if(!hasSD) {
		Serial.println("Rebooting");
		reBoot(0);
	}

	settings.init();

	File root = SD.open("/");
	fileScanner.scan(&root, settings);

	getSavedBankPosition();

	audioEngine.init(settings);

	int numFiles = 0;
	for(int i=0;i<=fileScanner.lastBankIndex;i++) {
		numFiles += fileScanner.numFilesInBank[i];
	}
	D(Serial.print("File Count ");Serial.println(numFiles););

	if(numFiles == 0) {
		NO_FILES = 1;
		D(Serial.println("No files"););
		ledFlashTimer = 0;
	} else if(fileScanner.numFilesInBank[playState.bank] == 0) {
		D(Serial.println("Empty bank"););
		while(fileScanner.numFilesInBank[playState.bank] == 0) {
			playState.bank++;
			if(playState.bank == fileScanner.lastBankIndex) {
				playState.bank = 0;
			}
		}
		D(Serial.print("Set bank to ");Serial.println(playState.bank););
	}

	interface.init(fileScanner.fileInfos[playState.bank][0].size, fileScanner.numFilesInBank[playState.bank], settings, &playState);

	D(Serial.println("--READY--"););
}

void getSavedBankPosition() {
	// CHECK FOR SAVED BANK POSITION
	int a = 0;
	a = EEPROM.read(EEPROM_BANK_SAVE_ADDRESS);
	if (a >= 0 && a <= fileScanner.lastBankIndex) {
		D(
			Serial.print("Using bank from EEPROM ");
			Serial.print(a);
			Serial.print(" . Active banks ");
			Serial.println(fileScanner.lastBankIndex);

		);
		playState.bank = a;
		playState.channelChanged = true;
	} else {
		EEPROM.write(EEPROM_BANK_SAVE_ADDRESS, 0);
	};
}

boolean openSDCard() {
	if (!(SD.begin(SS))) { // SS is the SPI chip select pin

		Serial.println("No SD.");
		while (!(SD.begin(SS))) { // SS is the SPI chip select pin
			ledControl.single(15);
			delay(SD_CARD_CHECK_DELAY);
			ledControl.single(rebootCounter % 4); // Modulo 4 on reboot counter. 0,1,2,3,0,1,2,3,0,1,2,3...
			delay(SD_CARD_CHECK_DELAY);
			rebootCounter++;
			Serial.print("Crash Countdown ");
			Serial.println(rebootCounter);
			if (rebootCounter > 4) {
				return false;
			}
		}
	}
	return true;
}

void loop() {

	#ifdef CHECK_CPU
	checkCPU();
//	audioEngine.measure();
	#endif

	if(NO_FILES) {
		// TODO : Flash the lights to show there are no files
		if(ledFlashTimer > 100) {
			ledControl.single(noFilesLedIndex);
			noFilesLedIndex++;
			noFilesLedIndex %= 4;
			ledFlashTimer = 0;
			rebootCounter ++;
			// Wait 3 seconds and reboot
			if(rebootCounter == 30) {
				reBoot(0);
			}
		}
		return;
	}

	updateInterfaceAndDisplay();

	audioEngine.update();

	if(audioEngine.error) {
		// Too many read errors, reboot
		Serial.println("Audio Engine errors. Reboot");
		delay(50); //give a short delay for the serial output to get there
		reBoot(0);
	}

	if (playState.channelChanged) {
		D(
		Serial.print("RW: Going to next channel : ");
		if(playState.channelChanged) Serial.print("RW: Channel Changed. ");
		Serial.println("");
		);

		playState.currentChannel = playState.nextChannel;

		AudioFileInfo* currentFileInfo = &fileScanner.fileInfos[playState.bank][playState.nextChannel];

		audioEngine.changeTo(currentFileInfo, interface.start);
		playState.channelChanged = false;

		resetLedTimer = 0; // This will trigger updateInterfaceAndDisplay(), which is in the main loop, to flash the reset LED

	}

}

void updateInterfaceAndDisplay() {

	checkInterface();

	if (bankChangeMode) {
		ledControl.showReset(1); // Reset LED is on continuously when in bank change mode..
		ledControl.multi(playState.bank);
	} else {
		ledControl.showReset(resetLedTimer < FLASHTIME); // Illuminate the reset LED until the timer runs out
		if (settings.showMeter) {
			peakMeter();
		}
	}
}

// INTERFACE //

void checkInterface() {

	uint16_t changes = interface.update();

	// BANK MODE HANDLING
	if((changes & BUTTON_LONG_PRESS) && !bankChangeMode) {
		D(Serial.println("Enter bank change mode"););
		bankChangeMode = true;
		nextBank();
//		ledFlashTimer = 0;
	} else if((changes & BUTTON_LONG_RELEASE) && bankChangeMode) {
		D(Serial.println("Exit bank change mode"););
		bankChangeMode = false;
	}

	if(changes & BUTTON_PULSE) {
		if(bankChangeMode) {
			D(Serial.println("BUTTON PULSE"););
			nextBank();
		} else {
			D(Serial.println("Button Pulse but not in bank mode"););
		}
	}

	boolean resetTriggered = changes & RESET_TRIGGERED;

	bool skipToStartPoint = false;
	bool speedChange = false;

	if(settings.pitchMode) {

		if(resetTriggered) {
			skipToStartPoint = true;
		}

		if((changes & (ROOT_NOTE_CHANGED | ROOT_POT_CHANGED | ROOT_CV_CHANGED) ) || resetTriggered) {
			speedChange = true;
		}

	} else {

		if((changes & CHANGE_START_NOW) || resetTriggered) {
			skipToStartPoint = true;
		}
	}

	if(resetTriggered) {
		if((changes & CHANNEL_CHANGED) || playState.nextChannel != playState.currentChannel) {
			playState.channelChanged = true;
		} else {
			resetLedTimer = 0; // This will trigger updateInterfaceAndDisplay(), which is in the main loop, to flash the reset LED
		}
	}

	if(speedChange) doSpeedChange();
	if(skipToStartPoint && !playState.channelChanged) {
		if(settings.pitchMode) {
			audioEngine.skipTo(0);
		} else {
			D(Serial.print("Skip to ");Serial.println(interface.start););
			audioEngine.skipTo(interface.start);
		}
	}
}

void doSpeedChange() {
	float speed = 1.0;
	speed = interface.rootNote - settings.rootNote;
	D(Serial.print("Root ");Serial.println(interface.rootNote););
	speed = pow(2,speed / 12); // Raise to power of 2

	audioEngine.setPlaybackSpeed(speed);
}

void nextBank() {

	if(fileScanner.lastBankIndex == 0) { // Only 1 bank. Don't do anything.
		D(Serial.println("Only 1 bank."););
		return;
	}
	playState.bank++;
	if (playState.bank > fileScanner.lastBankIndex) { // If we exceed the last bank, go back to the first bank.
		playState.bank = 0;
	}
	if(fileScanner.numFilesInBank[playState.bank] == 0) { // If the bank is empty, go to the next bank.
		D(Serial.print("No file in bank ");Serial.println(playState.bank););
		nextBank();
	}

	if (playState.nextChannel >= fileScanner.numFilesInBank[playState.bank]) 
		playState.nextChannel = fileScanner.numFilesInBank[playState.bank] - 1;

	interface.setChannelCount(fileScanner.numFilesInBank[playState.bank]);
	playState.channelChanged = true;

	D(
		Serial.print("RW: Next Bank ");
		Serial.println(playState.bank);
	);

	meterDisplayDelayTimer = 0; // Reset the peak meter display delay timer. Once settings.meterHide has elapsed, the peak meter will be displayed again. See peakMeter() below.
	EEPROM.write(EEPROM_BANK_SAVE_ADDRESS, playState.bank);
}

void peakMeter() {
	if( (peakDisplayTimer < 50) || (meterDisplayDelayTimer < settings.meterHide) ) return;

	float peakReading = audioEngine.getPeak();
	int monoPeak = round(peakReading * 4);
	monoPeak = round(pow(2, monoPeak)); // Raise to power of 2. monoPeak = 2^monoPeak. 
	ledControl.multi(monoPeak - 1);
	peakDisplayTimer = 0;
}

