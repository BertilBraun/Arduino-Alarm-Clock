#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2SNoDAC.h>

#include <vector>
#include <string>
#include <sstream>

#define motorPin    D0
#define buttonPin   D6
#define speakerPin  D5

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// TODO implement for WIFI
char* ssid = "";
char* pass = "";

AudioGeneratorMP3 *mp3 = nullptr;
AudioFileSourceICYStream *file = nullptr;
AudioFileSourceBuffer *buff = nullptr;
AudioOutputI2SNoDAC *out = nullptr;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2) - 1] = 0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string) {
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

unsigned long timerDelay = 60000;
unsigned long lastTime = timerDelay;
unsigned long disableAlarmDelay = 500;

unsigned long startHeld = 0;

bool alarmIsOn = false;
bool darkMode = false;
bool screenOff = false;

int snoozeTime = 5;
int snoozeCount = 0;
int snoozeH = -1;
int snoozeM = -1;
int alarmH = 0;
int alarmM = 0;

int timeFromServer = 0;
int numberToDisplay = 0;
bool updateScreen = true;

std::string lastSoundfilePath = "";

void connectToWifi() {
  Serial.print("Attempting to connect to Network named: ");
  Serial.print(ssid);

  WiFi.begin(ssid, pass);
  while ( WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("Connected to WiFi Network!");
}

void loadAudio(const std::string& url) {

  if (file) delete file;
  if (buff) delete buff;

  Serial.print("Starting to load Audio from Src: ");
  Serial.println(url.c_str());

  file = new AudioFileSourceICYStream(url.c_str());
  file->RegisterMetadataCB(MDCallback, (void*)"ICY");
  buff = new AudioFileSourceBuffer(file, 2048);
  buff->RegisterStatusCB(StatusCallback, (void*)"buffer");

  Serial.println("Done Loading!");
}

std::string httpGetRequest(const char* path) {
  HTTPClient http;
  http.begin(path);

  int httpCode = http.GET();
  if (httpCode <= 0)
  {
    Serial.println("Error on http request");
    return "";
  }

  return http.getString().c_str();
}

std::vector<std::string> split(const std::string& s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter))
  {
    tokens.push_back(token);
  }
  return tokens;
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void dim(bool dark) {

  uint8_t contrast = 255 * 1.171 - 43;
  uint8_t precharge = 241;
  uint8_t comdetect = 32;

  if (dark) {
    contrast = 0;
    precharge = 0;
    comdetect = 0;
  }

  display.ssd1306_command(SSD1306_SETPRECHARGE); //0xD9
  display.ssd1306_command(precharge); //0xF1 default, to lower the contrast, put 1-1F
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(contrast); // 0-255
  display.ssd1306_command(SSD1306_SETVCOMDETECT); //0xDB, (additionally needed to lower the contrast)
  display.ssd1306_command(comdetect);  //0x40 default, to lower the contrast, put 0
  display.ssd1306_command(SSD1306_DISPLAYALLON_RESUME);
  display.ssd1306_command(SSD1306_NORMALDISPLAY);
  display.ssd1306_command(SSD1306_DISPLAYON);
}

void draw() {
  // Only update Screen when nescessary
  if (!updateScreen)
    return;
  updateScreen = false;

  // display/lighting variation (to dimm the display, turn it on and off fast)
  dim(darkMode);
  display.clearDisplay();

  if (!screenOff) {
    display.setTextSize(4);
    display.setTextColor(WHITE);
    display.setCursor(5, 20);

    int h = numberToDisplay / 100;
    int m = numberToDisplay % 100;

    display.print(h);
    display.print(':');
    if ((int)(m / 10) == 0)
      display.print('0');
    display.print(m);

    display.setTextSize(1);
    display.setCursor(0, 0);
    if (alarmIsOn)
      display.print("Alarm: ON");
    else if (snoozeH != -1 || snoozeM != -1)
      display.print("Alarm: Pause");
    else
      display.print("Alarm: OFF");
  }

  display.display();
}

void startAlarm() {
  // TODO Mp3 doesn't work
  // TODO Starting Motor Kills the Arduino
  Serial.println("Alarm is now on!");
  alarmIsOn = true;
  updateScreen = true;
  digitalWrite(motorPin, HIGH);
  mp3->begin(buff, out);
}

void stopAlarm(bool forceStop = false) {
  if ((millis() - startHeld) < 3) return;
  Serial.println("Alarm turning off!");

  // disable Alarm
  if ((millis() - startHeld) > disableAlarmDelay || forceStop) {
    snoozeCount = 0;
    snoozeH = -1;
    snoozeM = -1;
  }
  // Not held long enought, go to snooze
  else if (alarmIsOn) {
    snoozeCount++;
    snoozeM = (alarmM + snoozeTime * snoozeCount) % 60;
    snoozeH = (alarmH + (int)((alarmM + snoozeTime * snoozeCount) / 60)) % 24;
  }

  digitalWrite(motorPin, LOW);
  alarmIsOn = false;
  updateScreen = true;

  if (mp3->isRunning())
    mp3->stop();
}

bool alarmRunningTooLong(int H, int M) {
  int timeTooLong = 10;

  if (!alarmIsOn)
    return false;

  if (snoozeH != -1 && snoozeM != -1)
    if (M >= (snoozeM + timeTooLong) % 60 &&
        H >= (snoozeH + (int)((snoozeM + timeTooLong) / 60)))
      return true;

  if (M >= (alarmM + timeTooLong) % 60 &&
      H >= (alarmH + (int)((alarmM + timeTooLong) / 60)))
    return true;

  return false;
}

void updatePerMin(unsigned long timeNow) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected");
    connectToWifi();
    return;
  }

  std::string timeString = httpGetRequest("http://alarm-clock.braun-oliver.de/getTime.php");
  std::string settingsString = httpGetRequest("http://alarm-clock.braun-oliver.de/settings.txt");

  if (timeString == "" || settingsString == "")
    return;

  std::vector<std::string> timeTokens = split(timeString, ' ');
  std::vector<std::string> settingTokens = split(settingsString, '\n');

  int h = atoi(timeTokens[0].c_str());
  int m = atoi(timeTokens[1].c_str());
  int s = atoi(timeTokens[2].c_str());

  timeFromServer = h * 100 + m;
  updateScreen = true;

  // Adjust lastTime based on s passed since full m
  lastTime = timeNow - ((s * 1000));

  Serial.printf("H %i, m %i, s %i\n", h, m, s);

  for (const std::string& token : settingTokens) {
    std::vector<std::string> nameValuePair = split(token, ':');

    if (nameValuePair.size() != 2) {
      Serial.printf("Could not evaluate %s\n", token.c_str());
      continue;
    }

    const std::string& name = nameValuePair[0];
    const std::string& value = nameValuePair[1];

    if (name == "Hour")
      alarmH = atoi(value.c_str());
    else if (name == "Minute")
      alarmM = atoi(value.c_str());
    else if (name == "SnoozeTime")
      snoozeTime = atoi(value.c_str());
    else if (name == "SoundPath") {
      if (lastSoundfilePath != value) {
        lastSoundfilePath = value;
        loadAudio(lastSoundfilePath);
      }
    }
    else if (name == "DarkMode")
      darkMode = (bool)atoi(value.c_str());
    else if (name == "ScreenOff")
      screenOff = (bool)atoi(value.c_str());
  }

  if ((alarmH == h && alarmM == m) || (snoozeH == h && snoozeM == m))
    startAlarm();

  // If alarm has been going for more than 10 min, shut off
  if (alarmRunningTooLong(h, m)) {
    Serial.println("Alarm has been running too long, stopping!");
    stopAlarm(true);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  digitalWrite(motorPin, LOW);

  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println("SSD1306 allocation failed");
    while (true);
  }

  delay(4000);

  connectToWifi();

  audioLogger = &Serial;
  out = new AudioOutputI2SNoDAC(speakerPin);
  mp3 = new AudioGeneratorMP3();
  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");

  Serial.println("Done Audio Setup!");
}

bool lastButtonState = LOW;
void loop() {
  draw();

  if (mp3->isRunning()) {
    if (!mp3->loop())
      mp3->stop();
  }

  unsigned long timeNow = millis();
  if ((timeNow - lastTime) > timerDelay)
    updatePerMin(timeNow);

  if (digitalRead(buttonPin) == HIGH && startHeld == 0)
    startHeld = millis();

  if (digitalRead(buttonPin) == LOW && startHeld != 0)
    stopAlarm();
  if (digitalRead(buttonPin) == LOW)
    startHeld = 0;
  // Overwrite number to display if button is pressed and alarm is not on, to see, at which time the alarm will go off
  if (digitalRead(buttonPin) == HIGH && !alarmIsOn)
    numberToDisplay = alarmH * 100 + alarmM;
  else
    numberToDisplay = timeFromServer;

  if (lastButtonState != digitalRead(buttonPin))
    updateScreen = true;

  lastButtonState = digitalRead(buttonPin);
}
