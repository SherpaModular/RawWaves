#ifndef AudioSystemHelpers_h
#define AudioSystemHelpers_h

#include <Audio.h>

// REBOOT CODES
#define RESTART_ADDR       0xE000ED0C
#define READ_RESTART()     (*(volatile uint32_t *)RESTART_ADDR)
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

void reBoot(int delayTime) {
	Serial.println("Rebooting");
//	Serial.end();
	if (delayTime > 0)
		delay(delayTime);
	WRITE_RESTART(0x5FA0004);
}

elapsedMillis cpuCheckTimer;

void checkCPU() {
    if (cpuCheckTimer > 3000) {
    	int maxCPU = AudioProcessorUsageMax();

    	Serial.print("MaxCPU : ");
    	Serial.print(maxCPU);

    	cpuCheckTimer = 0;
    	AudioProcessorUsageMaxReset();
    }
}

#endif
