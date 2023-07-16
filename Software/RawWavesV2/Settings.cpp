#include <SD.h>
#include "Settings.h"

#include "RawWaves.h"

#ifdef DEBUG_SETTINGS
#define D(x) x
#else
#define D(x)
#endif

Settings::Settings(const char* filename) {
	_filename = filename;
}

void Settings::init() {
	if (!SD.exists(_filename)) {
		write();
	};
	read();
}

void Settings::read() {

	D(Serial.println("Reading settings.txt"););

	char character;
	String settingName;
	String settingValue;
	settingsFile = SD.open("settings.txt");

	uint8_t NAME = 1;
	uint8_t VALUE = 2;
	uint8_t state = NAME;

	if (settingsFile) {
		while (settingsFile.available()) {
			character = settingsFile.read();
			if(state == NAME) {
				if(character == '=') {
					state = VALUE;
				} else {
					settingName = settingName + character;
				}
			} else if(state == VALUE) {
				if(character == '\n') {
					applySetting(settingName, settingValue);
					settingName = "";
					settingValue = "";
					state = NAME;
				} else {
					settingValue = settingValue + character;
				}
			}
		}
		if(settingName.length() > 0 && settingValue.length() > 0) {
			applySetting(settingName, settingValue);
		}
		// close the file:
		settingsFile.close();
	} else {
		// if the file didn't open, print an error:
		Serial.println("error opening settings.txt");
	}
	// Do test settings here

//	crossfade = true;
//	crossfadeTime = 1000;
//	looping = false;
//	anyAudioFiles = true;
//	hardSwap = true;
//	chanPotImmediate = false;
//	chanCVImmediate = false;
//	quantizeNote = true;
//	startCVImmediate = true;
//	startPotImmediate = true;
//	quantiseRootPot = true;
//	quantiseRootCV = true;
//	pitchMode = true;
//	startCVDivider = 1;

#ifdef TEST_RADIO_MODE
	radioMode();
#endif

#ifdef TEST_DRUM_MODE
	drumMode();
#endif

	if(!loopMode) {
		if(pitchMode) {
			loopMode = LOOP_MODE_START_POINT;
		} else {
			loopMode = LOOP_MODE_RADIO;
		}
	}

	D(Serial.print("Loop mode ");Serial.println(loopMode););

	if(anyAudioFiles || pitchMode) {
		hardSwap = true;
	}
}

void Settings::drumMode() {
	crossfade=0;
	crossfadeTime=100;
	showMeter=1;
	meterHide=2000;
	chanPotImmediate=1;
	chanCVImmediate=1;
	startPotImmediate=1;
	startCVImmediate=1;
	startCVDivider=1;
	looping=1;
	sort=1;
	pitchMode=1;
	hardSwap = true;
	anyAudioFiles = true;
	loopMode = LOOP_MODE_START_POINT;
}

void Settings::radioMode() {
	crossfade=1;
	crossfadeTime=100;
	showMeter=1;
	meterHide=2000;
	chanPotImmediate=1;
	chanCVImmediate=1;
	startPotImmediate=0;
	startCVImmediate=0;
	startCVDivider=2;
	looping=1;
	sort=1;
	pitchMode=0;
	hardSwap = false;
	anyAudioFiles = false;
	loopMode = LOOP_MODE_RADIO;
}

// Shamelessly stolen from https://stackoverflow.com/questions/650162/why-cant-the-switch-statement-be-applied-to-strings
constexpr unsigned int hash(const char *s, int off = 0) {                        
    return !s[off] ? 5381 : (hash(s, off+1)*33) ^ s[off];                           
}    

/* Apply the value to the parameter by searching for the parameter name
 Using String.toInt(); for Integers
 toFloat(string); for Float
 toBoolean(string); for Boolean
 */
void Settings::applySetting(String settingName, String settingValue) {

	D(
	Serial.print(settingName);
	Serial.print(" -> ");
	Serial.println(settingValue);
	);
	switch (hash(settingName.toLowerCase().c_str())) {
		case hash("mute"):
		case hash("crossfade"): // we are "falling through" here. "mute" or "crossfade" will both be handled by the same code
			crossfade = toBoolean(settingValue);
			break;
		case hash("declick"):
		case hash("crossfadetime"): // falling through again
			crossfadeTime = settingValue.toInt();
			break;
		case hash("showmeter"):
			showMeter = toBoolean(settingValue);
			break;
		case hash("meterhide"):
			meterHide = settingValue.toInt();
			break;
		case hash("chanpotimmediate"):
			chanPotImmediate = toBoolean(settingValue);
			break;
		case hash("chancvimmediate"):
			chanCVImmediate = toBoolean(settingValue);
			break;
		case hash("startpotimmediate"):
			startPotImmediate = toBoolean(settingValue);
			break;
		case hash("startcvimmediate"):
			startCVImmediate = toBoolean(settingValue);
			break;
		case hash("startcvdivider"):
			startCVDivider = settingValue.toInt();
			break;
		case hash("looping"):
			looping = toBoolean(settingValue);
			break;
		case hash("sort"):
			sort = toBoolean(settingValue);
			break;
		case hash("anyaudiofiles"):
			anyAudioFiles = toBoolean(settingValue);
			break;
		case hash("pitchmode"):
			pitchMode = toBoolean(settingValue);
			break;
		case hash("hardswap"):
			hardSwap = toBoolean(settingValue);
			break;
		case hash("noterange"):
			noteRange = settingValue.toInt();
			if(noteRange < 1) noteRange = 1;
			if(noteRange > 72) noteRange = 72;	
			break;
		case hash("rootnote"):
			rootNote = settingValue.toInt();
			if(rootNote < lowNote) rootNote = lowNote;
			if(rootNote > 96) rootNote = 96;
			break;
		case hash("loopmode"):
			loopMode = settingValue.toInt();
			break;
		case hash("quantisenotecv"):
		case hash("quantizerootcv"): // falling through again
			quantiseRootCV = toBoolean(settingValue);
			break;
		case hash("quantisenotepot"):
		case hash("quantizerootpot"): // falling through again
			quantiseRootPot = toBoolean(settingValue);
			break;
		default:
			// do something else
			break;
	}
}

// converting string to Float
float Settings::toFloat(String settingValue) {
	char floatbuf[settingValue.length()];
	settingValue.toCharArray(floatbuf, sizeof(floatbuf));
	float f = atof(floatbuf);
	return f;
}

// Converting String to integer and then to boolean. 1=true, 0=false
boolean Settings::toBoolean(String settingValue) {
	return settingValue.toInt() ==1;
}

void Settings::write() {
	Serial.println("Settings file not found, writing new settings");

	// Delete the old One
	SD.remove("settings.txt");
	// Create new one
	settingsFile = SD.open("settings.txt", FILE_WRITE);
	settingsFile.print("crossfade=");
	settingsFile.println(crossfade);
	settingsFile.print("crossfadeTime=");
	settingsFile.println(crossfadeTime);
	settingsFile.print("showMeter=");
	settingsFile.println(showMeter);
	settingsFile.print("meterHide=");
	settingsFile.println(meterHide);
	settingsFile.print("chanPotImmediate=");
	settingsFile.println(chanPotImmediate);
	settingsFile.print("chanCVImmediate=");
	settingsFile.println(chanCVImmediate);
	settingsFile.print("startPotImmediate=");
	settingsFile.println(startPotImmediate);
	settingsFile.print("startCVImmediate=");
	settingsFile.println(startCVImmediate);
	settingsFile.print("startCVDivider=");
	settingsFile.println(startCVDivider);
	settingsFile.print("looping=");
	settingsFile.println(looping);
	settingsFile.print("sort=");
	settingsFile.println(sort);
	settingsFile.print("pitchMode=");
	settingsFile.println(pitchMode);
	// close the file:
	settingsFile.close();
}
