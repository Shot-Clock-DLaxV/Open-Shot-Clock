
/*
  This is a simple example show the Heltec.LoRa recived data in OLED.

  The onboard OLED display is SSD1306 driver and I2C interface. In order to make the
  OLED correctly operation, you should output a high-low-high(1-0-1) signal by soft-
  ware to OLED's reset pin, the low-level signal at least 5ms.

  OLED pins to ESP32 GPIOs via this connecthin:
  OLED_SDA -- GPIO4
  OLED_SCL -- GPIO15
  OLED_RST -- GPIO16
  
  by Aaron.Lee from HelTec AutoMation, ChengDu, China
  成都惠利特自动化科技有限公司
  www.heltec.cn
  
  this project also realess in GitHub:
  https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/
#include <Arduino.h>

#include <heltec.h>
#include "images.h"
#include "channel.h"
#include "version.h"
#include "font.h"


#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include <Preferences.h>
#include <Adafruit_PWMServoDriver.h>

#include "config.h"
#include "LEDs.h"
#include "Horn.h"

#include <RadioLib.h>

#include <Regexp.h>

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

bool RS485mode = false;

AsyncWebServer server(80);

#if defined(WIFI_LoRa_32_V2)
  // Use the SX1276 Radio
  SX1276 radio = new Module(SS, DIO0, RST_LoRa, DIO0);
  Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
#endif

#if defined(WIFI_LoRa_32_V3)
  // Use the SX1262 Radio
  SX1262 radio = new Module(SS, DIO0, RST_LoRa, BUSY_LoRa);
  // Create a new TwoWire Object, because OLED uses the other one, that is not usable through pins
  TwoWire I2C = TwoWire(1);
  // Create PWM object using the new Wire object (i2c)
  Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, I2C);
#endif



LEDs leds(pwm);
Horn horn(pwm);

Preferences preferences;

String rssi = "RSSI --";
String packSize = "--";
String packet ;
String resetString = "restarting...reset your wifi connection";
String currentMode = "LoRa";
unsigned long ms;
unsigned long lms;
unsigned long diff;
bool clientFlag = false;
long int currentTime = 0;
long int previousTime = 0;

int channel;
int default_channel = 1;
uint8_t syncword_select[5]={
  0x12,   //  not needed
  0x12,   //  Kanal 1
  0x23,   //  Kanal 2  
  0x34,   //  Kanal 3  
  0x45    //  Kanal 4
};
uint8_t syncword = syncword_select[default_channel];
float frequency_select[5]={
  433.0F,   //  not needed
  433.0F,   //  Kanal 1
  433.5F,   //  Kanal 2  
  434.0F,   //  Kanal 3  
  434.5F   //  Kanal 4
};
float frequency = frequency_select[default_channel];

MatchState matchState;


// Function Prototypes
void drawLoraInfo();
void drawRS485Info();
bool timeIsUp();
void setupRadio();

unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}


bool timeIsUp() {
  return currentTime == 0 && previousTime != 0;
}

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setLoRaReceiveFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

void waitingHeltecDisplay(){
    Heltec.display->clear();
    Heltec.display->drawHorizontalLine(2, 50, 124);  
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->setFont(ArialMT_Plain_24);
    Heltec.display->drawString(64 , 1 , "waiting");
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(30, 52, "Channel " + String(channel));
    Heltec.display->drawString(95, 52, "none");
    Heltec.display->display();
}

void client_check(){
  ms = millis();
  diff = ms - lms;
  //lms = ms;
  if (diff >= 2100) {
    clientFlag = false;
    RS485mode = false;
    leds.showWaitingAnimation();
    }
}

bool isMessageValid(String msg) {
  // Msg Format 1:
  // C0461
  // C: Command [A-Z]
  // 04: 2 numbers - time
  // 6: 1 number - brightness
  // 1: channel

  // Msg Format 2:
  // H1
  // H: Command [H,B]
  // 1: channel

  // prepare matchstate
  int msgLength = msg.length();
  int bufferSize = msgLength + 1;
  char buffer[bufferSize];
  msg.toCharArray(buffer, bufferSize);
  matchState.Target(buffer);

  // check if channel is correct if not in cable mode
  if (!RS485mode && !msg.endsWith(String(channel))){
    return false;
  }

  // check if message matches valid patterns
  if ( msgLength == 2 ) {
    return matchState.Match("[HB][1-4]") == REGEXP_MATCHED;
  } else if ( msgLength == 5 ) {
    return matchState.Match("T[0-9][0-9][1-8][1-4]") == REGEXP_MATCHED;
  }
  return false;
}

void handlePacket(){
  if(!isMessageValid(packet)) {
    return;
  }
  String setTimeCommand = "T";
  String honkCommand = "H";
  Serial.println(packet);
  if (packet.startsWith(setTimeCommand)){
    previousTime = currentTime;
    String currentTimeString = packet.substring(1,3);
    currentTime = currentTimeString.toInt();

    String brigthnessString = packet.substring(3,4);
    leds.setBrightnessLevel(brigthnessString.toInt());
    leds.displayClock(currentTime);
      
    Heltec.display->clear();
    Heltec.display->drawHorizontalLine(2, 50, 124);  
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->setFont(DSEG14_Classic_Mini_Regular_40);
    Heltec.display->drawString(64 , 1 , currentTimeString);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(30, 52, "Channel " + String(channel));
    Heltec.display->drawString(90, 52, currentMode);
    //Heltec.display->drawString(95, 52, rssi);
    Heltec.display->display();
  } else if (packet.startsWith(honkCommand)){
    horn.requestHonk();
  }
}

void readLoraMessage() {
  int state = radio.readData(packet);
  if(state == RADIOLIB_ERR_NONE) {
    lms = millis();
    clientFlag = true;
    drawLoraInfo();
    handlePacket();
  } else {
    radio.reset();
    setupRadio();
  }
}

void set_channel(int ch){
  channel = ch;
  preferences.begin("shot-clock", false);
  preferences.putInt("channel", channel);
  Serial.println("Channel " + channel);
  preferences.end();
  delay(1000);
  ESP.restart();
}

String processor(const String& var){
  String links = "";
  if(var == "LINKPLACEHOLDER"){
    for(int c = 1; c <= 4; c++){
      links += "<a class=\"pp-button";
      links += channel == c ? " pp-is-active" : "";
      links += "\" href=\"/" + String(c) + "\">Channel " + String(c) + "</a>";
    }
  }   
  return links;
}

String versionProcessor(const String& var){
  String val = "";
  if(var == "PCB_VERSION_PLACEHOLDER"){
    val = String(DISPLAY_PCB_VERSION);
  }
  if(var == "BOARD_VERSION_PLACEHOLDER"){
    val = String(BOARD);
  }
  if(var == "BRANCH_PLACEHOLDER"){
    val = String(BRANCH);
  }
  if(var == "COMMIT_PLACEHOLDER"){
    val = String(COMMIT);
  }
  return val;
}

void RS485receive() {
   //while (RS485Serial.available()) {
   while (Serial2.available()) {

     //char inChar = (char)RS485Serial.read(); // Get the next byte
     char inChar = (char)Serial2.read(); // Get the next byte

     if (inChar == '\n') // If the incoming character is a newline break while loop
     {
       stringComplete = true;
       RS485mode = true;
       break;
     } else if (!isSpace(inChar)) {
      packet += inChar; // Add value to inputstring
     }

    if (packet.length() > 100) // If inputString is too long break while loop
    {
      Serial.println("ERROR");
      break;
    }
  }
}

void initWebserver() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am your Shot Clock :)");
  });

  server.on("/channel", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", channel_html, processor);
  });

  server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", version_html, versionProcessor);
  });

  server.on("/1", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", resetString);
    set_channel(1);
  });

  server.on("/2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", resetString);
    set_channel(2);
  });  

  server.on("/3", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", resetString);
    set_channel(3);    
  });  

  server.on("/4", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", resetString);
    set_channel(4);
  });

}

void initChannelFromEEPROM(){
  preferences.begin("shot-clock", false);

  channel = preferences.getInt("channel", default_channel);

  syncword = syncword_select[channel];
  frequency = frequency_select[channel];
   
  preferences.end();
}

void setupRadio() {
  // initialize SX12xx with default settings
  Serial.print(F("[SX12xx] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    // while (true);
  }

  radio.setSyncWord(syncword);
  radio.setFrequency(frequency);

  // set the function that will be called
  // when new packet is received
  radio.setPacketReceivedAction(setLoRaReceiveFlag);

  // start listening for LoRa packets
  Serial.print(F("[SX12xx] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    // while (true);
  }
}

void initI2C() {
  #ifdef WIFI_LoRa_32_V3
    I2C.setPins(SDA_LED, SCL_LED);
  #endif

  pwm.begin();
  pwm.setPWMFreq(200);  // This is the maximum recommended PWM frequency for LEDs
}

void setup() {

  initChannelFromEEPROM();

  //RS-485
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  inputString.reserve(200);
  
  long band=434000000;  // not used anymore, because radioLib handles LoRa
  Heltec.begin(true /*Display Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/, false /*PABOOST Enable*/, band /*long BAND*/);

  setupRadio();

  initI2C();

  leds.allSegmentsOff();

  Heltec.display->init();
  // Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  
  Heltec.display->clear();
  
  //Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
  //Heltec.display->drawString(0, 10, "LED Test abgeschlossen");
  //Heltec.display->drawString(0, 0, "Wait for incoming data...");
  Heltec.display->display();
  

  //ESP32 As access point
  WiFi.mode(WIFI_AP); //Access Point mode
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP(); //Get IP address
  Serial.print("HotSpt IP:");
  Serial.println(myIP);
  
  initWebserver();

  ElegantOTA.begin(&server);    // Start ElegantOTA
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  server.begin();
  Serial.println("HTTP server started");

  
  leds.showWaitingAnimation();
  waitingHeltecDisplay();
}

void drawLoraInfo() {
  currentMode = "LoRa";
  // Heltec.display->drawString(90, 52, "LoRa");
  // Heltec.display->display();
}

void drawRS485Info() {
  currentMode = "RS485";
  // Heltec.display->drawString(90, 52, "RS485");
  // Heltec.display->display();
}

void loop() {

   ElegantOTA.loop(); 

  if (RS485mode == false){
    if (receivedFlag) { 
      readLoraMessage();
      receivedFlag = false;
    }
  }
  //RS-485 Test
  RS485receive();

  // print the string when a newline arrives:
  if (stringComplete) {
    

    lms = millis();
    clientFlag = true;
    handlePacket();
    drawRS485Info();

    // clear the string:
    packet = "";
    stringComplete = false;
  }


  if (clientFlag == false){
    leds.showWaitingAnimation();
    waitingHeltecDisplay();
    }
  else{
    client_check();
    }

 

  if (timeIsUp()) {
    horn.requestHonk();
  }

  horn.handle();
  leds.handle();
}
