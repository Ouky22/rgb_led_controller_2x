#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"

const String ssid = "";
const String password = "";
IPAddress staticIp(192, 168, 1, 250);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
ESP8266WebServer server(80);

// pin definitions. must be PWM-capable pins!
const int redPin1 = D2;
const int greenPin1 = D1;
const int bluePin1 = D3;
const int redPin2 = D6;
const int greenPin2 = D7;
const int bluePin2 = D5;

// structure of commands: ccc.abc.def.
// ccc = command identifier (which type of command) or value for red
//## 0 <= ccc <= 255 -> change color
// ccc --> red value (0-255)
// abc --> green value (0-255)
// def --> blue value (0-255)
//## 300.abc.xxx. - off command
// turn off certain LED strip
// abc --> id of strip
//## 400.abc.xxx. - on command
// turn on certain LED strip
// abc --> id of strip
//## 500.abc.xxx. - brightness command
// abc --> brightness value (0-255)
//## 600.abc.xxx. - rgb show command
// abc --> speed (0-10)
//## 700.xxx.xxx. - current settings command
// receive all current settings (e.g. color values, brightness, strips, which strip is enabled, status of rgb show)
//## 800.xxx.xxx. - rgb wakeup command
// trigger new rgb wakeup

const int offCommandIdentifier = 300;
const int onCommandIdentifier = 400;
const int brightnessCommandIdentifier = 500;
const int rgbShowCommandIdentifier = 600;
const int settingsCommandIdentifier = 700;
const int alarmWakeupCommandIdentifier = 800;

const int strip1Id = 1;
const String strip1Name = "Sofa";
const int strip2Id = 2;
const String strip2Name = "Bed";

int currentBrightness = 100;
int rgbColors[3] = {255, 0, 0};
bool strip1IsOn = false;
bool strip2IsOn = false;

int rgbShowSpeed = 500; // a higher number means more delay --> less speed
int maxRgbShowSpeed = 20;
int minRgbShowSpeed = 1000;
bool rgbShowIsRunning = false;
int rgbShowCounter = 0;

int rgbWakeupCounter = 0;
int rgbWakeupIsRunning = false;

const char commandSeperator = '.';


void setup() {
  pinMode(redPin1, OUTPUT);
  pinMode(greenPin1, OUTPUT);
  pinMode(bluePin1, OUTPUT);
  pinMode(redPin2, OUTPUT);
  pinMode(greenPin2, OUTPUT);
  pinMode(bluePin2, OUTPUT);

  initWifi();
}

void loop() {
  server.handleClient();

  if (rgbShowIsRunning)
    setRgbShowColors();
  else if (rgbWakeupIsRunning)
    setWakeupBrightness();

  while(WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(10000);
  }
}

void executeCommand(int commandParts[3]) {
  int commandIdentifier = commandParts[0];
  
  if (commandIdentifier <= 255 && commandIdentifier >= 0)
    executeChangeColorCommand(commandParts[0], commandParts[1], commandParts[2]);
  else if (commandIdentifier == brightnessCommandIdentifier)
    executeChangeBrightnessCommand(commandParts[1]);
  else if (commandIdentifier == offCommandIdentifier)
    executeTurnLightOffCommand(commandParts[1]);
  else if (commandIdentifier == onCommandIdentifier)
    executeTurnLightOnCommand(commandParts[1]);
  else if (commandIdentifier == settingsCommandIdentifier)
    sendCurrentSettings();
  else if (commandIdentifier == alarmWakeupCommandIdentifier)
    executeStartWakeupCommand();
  else if (commandIdentifier == rgbShowCommandIdentifier)
    executeRgbShowCommand(commandParts[1]);
}

void executeChangeColorCommand(int red, int green, int blue) {
  rgbShowIsRunning = false;
  rgbColors[0] = constrain(red, 0, 255);
  rgbColors[1] = constrain(green, 0, 255);
  rgbColors[2] = constrain(blue, 0, 255);
  setColors();
  rgbWakeupIsRunning = false;
  
  String response = "colors set to: r:" + (String) rgbColors[0] + " g:" + (String) rgbColors[1] + " b:" + (String) rgbColors[2];
  server.send(200, "text/plain", response);
}

void executeChangeBrightnessCommand(int brightness) {
  currentBrightness = constrain(brightness, 0, 255);
  setColors();
  rgbWakeupIsRunning = false;
  
  String response = "brightness set to: " + (String) brightness;
  server.send(200, "text/plain", response);
}

void executeTurnLightOffCommand(int stripId) {
  if (stripId == 0)
    turnAllStripsOff();
   else
    turnStripOff(stripId);

  rgbShowIsRunning = false;
  rgbWakeupIsRunning = false;

  if (stripId == 0)
    server.send(200, "text/plain", "all strips turned off");
  else
    server.send(200, "text/plain", "strip " + (String) stripId + " turned off");  
}

void executeTurnLightOnCommand(int stripId) {
  if (stripId == 0)
    turnAllStripsOn();
  else
    turnStripOn(stripId);

  rgbShowIsRunning = false;
  rgbWakeupIsRunning = false;

  if (stripId == 0)
    server.send(200, "text/plain", "all strips turned on");
  else
    server.send(200, "text/plain", "strip " + (String) stripId + " turned on");  
}

void executeStartWakeupCommand() {
  rgbShowIsRunning = false;
  rgbWakeupIsRunning = true;
  rgbWakeupCounter = 0;
  currentBrightness = 20;
  turnAllStripsOn();
  
  String response = "wakeup started";
  server.send(200, "text/plain", response);
}

void executeRgbShowCommand(int receivedSpeedValue) {
  rgbWakeupIsRunning = false;
  rgbShowIsRunning = true;
  setRgbShowSpeed(receivedSpeedValue);
  rgbShowCounter = 0;

  String response = "rgb show started with speed: " + (String) rgbShowSpeed;
  server.send(200, "text/plain", response);  
}

void setRgbShowSpeed(int receivedSpeedValue) {
  int contrainedSpeedValue = constrain(receivedSpeedValue, 0, 10);
  rgbShowSpeed = map(contrainedSpeedValue, 0, 10, minRgbShowSpeed, maxRgbShowSpeed);
}

void sendCurrentSettings() {
  String response = "{\"color\":{"
    "\"red\":" + (String) rgbColors[0] + ","
    "\"green\":" + (String) rgbColors[1] + ","
    "\"blue\":" + (String) rgbColors[2] + "},"
    "\"brightness\":" + (String) currentBrightness + ","
    "\"strips\":["
    "{\"id\":" + (String) strip1Id + ",\"name\":\"" + strip1Name + "\",\"isOn\":" + (String) (strip1IsOn ? "true" : "false") + "},"
    "{\"id\":" + (String) strip2Id + ",\"name\":\"" + strip2Name + "\",\"isOn\":" + (String) (strip2IsOn ? "true" : "false") + "}"
    "],"
    "\"isRgbShowActive\":" + (String)(rgbShowIsRunning ? "true" : "false") + "}";
  server.send(200, "text/plain", response);
}

void handleCommandReceived() {
  String receivedValue = server.arg("value");

  // valid commands can only have a length between 6 and 12 (e.g.: 0.0.0. or 255.255.255.) 
  if (receivedValue.length() < 6 && receivedValue.length() > 12) {
    server.send(400, "text/plain", "invalid command");
    return;
  }

  int commandParts[3];
  bool invalidCommand = extractCommandParts(receivedValue, commandParts) < 0;
  if (invalidCommand) {
    server.send(400, "text/plain", "invalid command");
    return;      
  }
  executeCommand(commandParts);
}

// returned value is -1 if receivedValue is not a valid command, positive otherwise
int extractCommandParts(String receivedValue, int commandParts[3]) {
  String commandPart = "";
  int commandPartsCounter = 0;

  for (int i = 0; i < receivedValue.length(); i++) {
    if (commandPart.length() >= 4 || commandPartsCounter >= 3)
      return -1;
    
    if (receivedValue[i] == commandSeperator) {
      commandParts[commandPartsCounter] = commandPart.toInt();
      commandPartsCounter++;
      commandPart = "";
    } else
      commandPart += receivedValue[i];
  }
  
  if (commandPartsCounter != 3)
    return -1;
  return 1;
}

void setColors() {
  if (strip1IsOn) {
    analogWrite(redPin1, map(rgbColors[0], 0, 255, 0, currentBrightness));
    analogWrite(greenPin1, map(rgbColors[1], 0, 255, 0, currentBrightness));
    analogWrite(bluePin1, map(rgbColors[2], 0, 255, 0, currentBrightness));
  }
  if (strip2IsOn) {
    analogWrite(redPin2, map(rgbColors[0], 0, 255, 0, currentBrightness));
    analogWrite(greenPin2, map(rgbColors[1], 0, 255, 0, currentBrightness));
    analogWrite(bluePin2, map(rgbColors[2], 0, 255, 0, currentBrightness));
  }
}

void turnAllStripsOn() {
  turnStripOn(strip1Id);
  turnStripOn(strip2Id);
}

void turnStripOn(int stripId) {
  if (stripId == strip1Id) {
    strip1IsOn = true;
    setColors(); 
  }
  if (stripId == strip2Id) {
    strip2IsOn = true;
    setColors();
  }
}

void turnAllStripsOff() {
  turnStripOff(strip1Id);
  turnStripOff(strip2Id);
}

void turnStripOff(int stripId) {
  if (stripId == strip1Id) {
    strip1IsOn = false;
    analogWrite(redPin1, 0);
    analogWrite(greenPin1, 0);
    analogWrite(bluePin1, 0);
  }
  if (stripId == strip2Id) {
    strip2IsOn = false;
    analogWrite(redPin2, 0);
    analogWrite(greenPin2, 0);
    analogWrite(bluePin2, 0);
  }  
}

void setRgbShowColors() {
  if (rgbShowCounter == 0) {
    if (rgbColors[0] == 255 && rgbColors[1] < 255 && rgbColors[2] == 0) // red to yellow
      rgbColors[1] = rgbColors[1] + 1;
    else if (rgbColors[0] > 0 && rgbColors[1] == 255 && rgbColors[2] == 0) // yellow to green
      rgbColors[0] = rgbColors[0] - 1;
    else if (rgbColors[0] == 0 && rgbColors[1] == 255 && rgbColors[2] < 255) // green to turquoise
      rgbColors[2] = rgbColors[2] + 1;
    else if (rgbColors[0] == 0 && rgbColors[1] > 0 && rgbColors[2] == 255) // turquoise to blue
      rgbColors[1] = rgbColors[1] - 1;
    else if (rgbColors[0] < 255 && rgbColors[1] == 0 && rgbColors[2] == 255) // blue to purple
      rgbColors[0] = rgbColors[0] + 1;
    else if (rgbColors[0] == 255 && rgbColors[1] == 0 && rgbColors[2] > 0) // purple to red
      rgbColors[2] = rgbColors[2] - 1;
    
    setColors();
  }
  delay(1);
  rgbShowCounter = rgbShowCounter + 1;
  if (rgbShowCounter > rgbShowSpeed)
    rgbShowCounter = 0; 
}

void setWakeupBrightness() {
  if (rgbWakeupCounter == 0) {
    currentBrightness = constrain(currentBrightness + 5, 0, 255); 
    setColors();
  }

  delay(1);
  rgbWakeupCounter++;
  if (rgbWakeupCounter > 2500)
    rgbWakeupCounter = 0;
  
  if (currentBrightness >= 255)
    rgbWakeupIsRunning = false;
}

void initWifi() {
  WiFi.config(staticIp, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  server.on("/command/", handleCommandReceived);
  server.begin();
}
