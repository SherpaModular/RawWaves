#include "WavHeaderReader.h"

#include <Audio.h>
#include <SD.h>

#include "RawWaves.h"

#ifdef DEBUG_WAV
#define D(x) x
#else
#define D(x)
#endif

/* 
The WavHeaderReader class is used to read the header of a wav file.
The header is the first 44 bytes of the file.
It contains information about the audio file such as sample rate, bit depth, number of channels and the length of the audio data.
The header is used to set up the audio system to play the file.
The header is also used to determine the length of the file so that the file can be played from start to finish.
*/

boolean WavHeaderReader::read(File* file, AudioFileInfo& info) {

	waveFile = file;

	if (waveFile->available()) {
		D(
			Serial.print("Bytes available ");
			Serial.println(waveFile->available());
		);
		uint32_t chunkSize = 0;

		// WAV header, part 1:	RIFF
		// 	Description: RIFF file description header
		//  Usual contents: "The ASCII text string "RIFF"
		//  Size: 4 bytes
		if (!waveFile->seek(4)) {
			D(Serial.println("Seek past RIFF failed"); );
			return false;
		} else {
			D(Serial.print("Cur Pos "); Serial.println(waveFile->position()););
		}

		// WAV header, part 2: <file length>
		// 	Description: Size of file
		//  Usual contents: Size of the overall file LESS 8 bytes (less part 1 "RIFF", above, and this part 2 <file length>)
		//  Size: 4 bytes
		uint32_t fileSize = readLong();
		D(Serial.print("File size "); Serial.println(fileSize););

		// WAV header, part 3:	WAVE
		// 	Description: The WAV description header
		//  Usual contents: "The ASCII text string "WAVE"
		//  Size: 4 bytes

		// 'WAVE' as little endian uint32 is 1163280727
		// If chunk ID isnt 'WAVE' stop here.
		if (readLong() != 1163280727) {
			D(Serial.println("Chunk ID not WAVE"); );
			return false;
		}

		// WAV header, part 4: "fmt "
		// 	Description: Format chunk marker
		//  Usual contents: "The ASCII text string "fmt " (note the trailing space)
		//  Size: 4 bytes
		uint32_t nextID = readLong();
		// Keep skipping chunks until we hit 'fmt '
		while (nextID != 544501094) {
			chunkSize = readLong();
			D(Serial.print("Skipping "); Serial.println(chunkSize););
			waveFile->seek(waveFile->position() + chunkSize);
			nextID = readLong();
		}

		if (!waveFile->available()) {
			D(Serial.println("Skipped whole file"); );
			return false;
		}

		D(Serial.print("Found fmt chunk at "); Serial.println(waveFile->position()););

		// WAV header, part 5: <format chunk length>
		// 	Description: A number of formatting values are about to be presented in the file. Before presenting these values,
		//				This 4-byte number will give the total number of bytes that these values will occupy.
		//				These values are going to be: 
		//					* The audio format type (usually 2 bytes)
		// 					* The number of channels (2 bytes)
		// 					* The sample rate (4 bytes)
		// 					* The byte rate, bytes/sec (4 bytes)
		// 					* The block alignment (2 bytes)
		// 					* The number of bits per sample (2 bytes)
		//  Usual contents: Usually the value 16, as you can see from the above list of values.
		//  Size: 4 bytes
		chunkSize = readLong();
		D(Serial.print("Format Chunk Size "); Serial.println(chunkSize););

		// WAV header, part 6: <Audio format type>
		// 	Description: Type of WAV format
		//  Usual contents: (0x01) = PCM
		//  Size: 2 bytes
		uint16_t format = readShort();

		// WAV header, part 7: <Number of channels>
		// 	Description: Number of audio channels
		//  Usual contents: (0x01) = mono, (0x02) = stereo
		//  Size: 2 bytes
		info.setChannels(readShort());
		D(Serial.print("Is Stereo : "); Serial.println(info.format & STEREO););

		// WAV header, part 8: <Sample rate>
		// 	Description: Sample rate in Hz
		//  Usual contents: 8000, 44100, etc.
		//  Size: 4 bytes
		if(!info.setSampleRate(readLong())) {
			return false;
		}

		// WAV header, part 9: <Byte rate>
		// 	Description: Average bytes per second
		//  Usual contents: Sample rate * number of channels * bits per sample / 8
		//  Size: 4 bytes
		uint32_t byteRate = readLong();

		// WAV header, part 10: <Block alignment>
		// 	Description: Block size of data
		//  Usual contents: Number of channels * bits per sample / 8
		//  Size: 2 bytes
		uint16_t blockAlign = readShort();

		// WAV header, part 11: <Bits per sample>
		// 	Description: Number of bits per sample
		//  Usual contents: 8 bits = 8, 16 bits = 16, etc.
		//  Size: 2 bytes
		uint16_t bitsPerSample = readShort();
		if (bitsPerSample % 8 != 0) {
			D(Serial.print("Unsupported bit depth "); Serial.println(bitsPerSample););
			return false;
		}
		info.setBitsPerSample(bitsPerSample);

		if (chunkSize > 16) {
			D(Serial.print("Format chunk is extended "); Serial.println(chunkSize););
			waveFile->seek(waveFile->position() + chunkSize - 16);
		}

		// WAV header, part 12: "data"
		// 	Description: Data chunk marker
		//  Usual contents: The ASCII text string "data"
		//  Size: 4 bytes
		// 'data' as little endian uint32 is 1635017060
		while (readLong() != 1635017060) {
			chunkSize = readLong();
			waveFile->seek(waveFile->position() + chunkSize);
		}

		// WAV header, part 13: <data chunk length>
		// 	Description: Size of audio data to follow, in bytes
		//  Usual contents: (size of file) - 44
		//  Size: 4 bytes
		chunkSize = readLong();
		D(Serial.print("WAV data length "); Serial.println(chunkSize););
		info.size = chunkSize;
		info.dataOffset = waveFile->position();
	} else {
		D(Serial.println("File not available"); );
	}
	return true;
}

uint16_t WavHeaderReader::readShort() {
	uint16_t val = waveFile->read();
	val = waveFile->read() << 8 | val;
	return val;
}

// WAV files are little endian
uint32_t WavHeaderReader::readLong() {

	int32_t val = waveFile->read();
	if (val == -1) {
		D(Serial.println("Long read error. 1"); );
	}
	for (byte i = 8; i < 32; i += 8) {
		val = waveFile->read() << i | val;
		if (val == -1) {
			D(Serial.print("Long read error "); Serial.println(i););
		}
	}
	return val;
}
