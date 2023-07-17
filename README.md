## Raw Waves v2

Raw Waves v2 is a sample player that functions much like a live radio. Audio sample files may be placed on a MicroSD card, and arranged into "banks" (like channels) which you may select either manually through the panel knobs, or via CV. File formats supported include RAW and WAV files.

The module is functionally identical to the popular "Raw Waves" module, and its predecessor, "Radio Music" by Tom Whitwell of Music Thing Modular. These previous modules are no longer available, having been built upon the now-discontinued Teensy 3 microcontroller.

Raw Waves v2 has been completely redesigned to use the Teensy 4 platform. Because the module uses the Teensy 4 microcontroller, the "Raw Waves v2" will not be compatible with other alternative firmware made for Radio Music (e.g. 'Chord Organ').

Raw Waves v2 still supports all of the settings in the SETTINGS.TXT file that you came to know and love in the earlier models. If you haven't played with the settings, you really should!

#### Specifications
Audio output: 16-bit audio output, with hardware anti-alias filtering.
Dimensions: 4 hp, 21mm deep.
Power specifications: Requires +12v (at 100 mA) and -12v (at 10 mA). The module features several protections against accidental reverse power connection: First, a keyed 10-pin Eurorack standard connector, second, automatically resetting fuses, and thirdly, protection diodes.
CV Inputs are designed for 0-5v, with reasonable over/undervoltage protection built-in.


#### Compiling the code
My preference is to use Visual Studio Code:
1. Install the Microsoft "Arduino for Visual Studio Code" extension
2. Add the Teensy packages. You can accomplish this in the "Arduino for Visual Studio Code" extension settings by adding the URL https://www.pjrc.com/teensy/package_teensy_index.json to the "Arduino: Additional Urls, Additional URLs for 3rd party packages." setting.
3. The following file contents, placed into the repo under the filename .vscode\arduino.json helps with other details:
```
{
    "sketch": "Software\\RawWavesV2\\RawWavesV2.ino",
    "configuration": "usb=serial,speed=600,opt=o2std,keys=en-us",
    "board": "teensy:avr:teensy40",
    "output": "ArduinoOutput\\RawWavesV2",
    "port": "COM3"
}
```
4. In VS Code, activate the Command Palette (typically by pressing F1), and choose "Ardunio: Verify".
5. If all codes well, the compiled output should appear in the repo directory, under ArduinoOutput\RawWavesV2

#### License  
Raw Waves is Open Hardware. It is a derivate work of ["Radio Music", by Tom Whitwell](https://github.com/TomWhitwell/RadioMusic). 
All hardware and software design in this project is Creative Commons licensed by Jim Mulvey: [CC-BY-SA: Attribution / ShareAlike](https://creativecommons.org/licenses/by-sa/3.0/)
If you use any work in this project, you should credit Tom Whitwell and me, and you must republish any changes you make under the same license.
This license does permit commercial use of these designs, but consider getting in touch before selling anything.
This project includes work from the [Teensy](https://www.pjrc.com/teensy/) project, which is not covered by this license.