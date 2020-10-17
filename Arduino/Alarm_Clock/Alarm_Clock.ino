#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

char* ssid = "";
char* pass = "";

unsigned long timerDelay = 58000;
// unsigned long timerDelay = 1000;
unsigned long lastTime = timerDelay;

bool alarmIsOn = false;
int motorPin = D2;
int buttonPin = D1;

void connectToWifi() {
  Serial.print("Attempting to connect to Network named: ");
  Serial.print(ssid);

  while ( WiFi.status() != WL_CONNECTED) {

    WiFi.begin(ssid, pass);
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("Connected to WiFi Network!");
}

String httpGetRequest(const char* path) {
  HTTPClient http;
  http.begin(path);

  int httpCode = http.GET();
  if (httpCode <= 0)
  {
    Serial.println("Error on http request");
    return "";
  }

  return http.getString();
}

String getValue(String data, char separator, int index)
{
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

void setup() {
  pinMode(motorPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  analogWrite(motorPin, 0);

  Serial.begin(115200);
  while (! Serial);

  delay(4000);

  connectToWifi();
}

void loop() {
  if (digitalRead(buttonPin) == HIGH) { // Stop the alarm
    analogWrite(motorPin, 0);
    alarmIsOn = false;
  }

  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi Disconnected");
      connectToWifi();
      return;
    }

    lastTime = millis();

    String timeString = httpGetRequest("http://alarm-clock.braun-oliver.de/getTime.php");
    String settingsString = httpGetRequest("http://alarm-clock.braun-oliver.de/settings.php");

    if (timeString == "" || settingsString == "")
      return;

    String H = getValue(timeString, ' ', 0);
    String M = getValue(timeString, ' ', 1);
    String S = getValue(timeString, ' ', 2);

    int h = atoi(H.c_str());
    int m = atoi(M.c_str());
    int s = atoi(S.c_str());

    String AH = getValue(settingsString, ' ', 0);
    String AM = getValue(settingsString, ' ', 1);

    int ah = atoi(AH.c_str());
    int am = atoi(AM.c_str());

    if (ah == h && am == m) {
      Serial.println("Alarm is now on!");
      alarmIsOn = true;
      analogWrite(motorPin, 255);
    }

    Serial.print(" H ");
    Serial.print(h);
    Serial.print(" M ");
    Serial.print(m);
    Serial.print(" S ");
    Serial.print(s);
    Serial.print(" AH ");
    Serial.print(ah);
    Serial.print(" AM ");
    Serial.print(am);
    Serial.println("");
  }
  delay(20);
}
