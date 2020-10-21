#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"


class Sound {
public:
	void begin(int pin) {
		Serial.println("Setting up amplifier and MP3 player...");
		pinMode(pin, OUTPUT);
		digitalWrite(pin, HIGH);  // enable access to AMP
		out = new AudioOutputI2S(0, true);
		out->SetOutputModeMono(true);
		out->SetGain(0.5);
		mp3 = new AudioGeneratorMP3();
		Serial.println("Done");
	}
	
    void tick() {
		if (playing && mp3->isRunning())
			if (!mp3->loop())
				play(file);
	}
	
	void stop() {
		playing = false;
		mp3->stop();
	}
	
    void play(const char *file) {
		if (mp3->isRunning()) {
			mp3->stop();
		}
		if (currentFileID3 != NULL) {
			delete currentFile;
		}
		currentFile = new AudioFileSourceSPIFFS(file);
		if (currentFileID3 != NULL) {
			delete currentFileID3;
		}
		currentFileID3 = new AudioFileSourceID3(currentFile);
		if (mp3->begin(currentFileID3, out)) {
			Serial.print("Beginning ");
			Serial.println(file);
		}
		else {
			Serial.print("Failed to begin ");
			Serial.println(file);
		}
		this->file = file;
    }
	
  private:
    const char* file;
	
    AudioGeneratorMP3 *mp3;
    AudioOutputI2S *out;
    AudioFileSourceSPIFFS* currentFile;
    AudioFileSourceID3* currentFileID3;
};