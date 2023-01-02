
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
#include "font.h"


#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

//RS-485
#define RXD2 13
#define TXD2 12

#define Vext 21

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

bool RS485mode = false;

//const char* ssid = "ShotClockBlue1";
//const char* ssid = "ShotClockBlue2";
// const char* ssid = "OSC_Blue";
const char* ssid = "ShotClockRed1";
// const char* ssid = "ShotClockRed2";
// const char* ssid = "OSC_Hannover_Set1_No1";
// const char* ssid = "OSC_Hannover_Set1_No2";
// const char* password = "12345678";
const char* password = "EM2022LAX";

AsyncWebServer server(80);


#include <Adafruit_PWMServoDriver.h>

//initial Brightness Level 8 (max)

int B_Level = 8;

uint16_t ticks[9][2]={
  {0, 4096},           // 0 (aus)
  {0, 32},       // 1
  {0, 64},       // 2 (25%)
  {0, 64*2},       // 3
  {0, 64*2*2},       // 4 (50%)
  {0, 64*2*2*2},       // 5
  {0, 64*2*2*2*2},       // 6 (75%)
  {0, 64*2*2*2*2*2},       // 7
  {4096, 0}                    // 8 (an, 100%)
};

//int B = 200; //Brightness between 0 and 4096, not active 

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

#include <Preferences.h>

Preferences preferences;

int channel;
int default_channel = 1;
long band;
String rssi = "RSSI --";
String packSize = "--";
String packet ;
String resetString = "restarting...reset your wifi connection";
unsigned long ms;
unsigned long lms;
unsigned long diff;
unsigned long w_ms;
unsigned long w_lms;
unsigned long w_diff;
unsigned long interval = 150;
bool clientFlag = false;
long int Clock = 88;
long int pClock = 88;
String ClockStr = "88";
int waitingNR = 1;

bool isHonking = false;
unsigned long honkStartTime = 0;
const int HONK_DURATION = 1000;
bool isHonkRequest = false;


// 1 Array mit 10 Daten Zeilen mit jeweils 7 Datensätzen Typ Bool (0 bis 9)
bool segs[10][7]={
 {true,true,true,true,true,true,false},               //0
 {true,false,false,false,false,true, false},          //1
 {true, true, false, true, true, false, true},        //2
 {true, true, false, false, true, true, true},        //3
 {true, false, true, false, false, true, true},       //4
 {false, true, true, false, true, true, true},        //5
 {false, true, true, true, true, true, true},         //6
 {true, true, false, false, false, true, false},      //7
 {true, true, true, true, true, true, true},          //8
 {true, true, true, false, true, true, true}          //9
};

long band_select[5]={
  433000000,   //  not needed
  433000000,   //  Kanal 1
  433500000,   //  Kanal 2  
  434000000,   //  Kanal 3  
  434500000    //  Kanal 4
};

int waiting1er[5]={
  0,
  8,
  13,
  12,
  4
};

int waiting10er[5]={
  0,
  3,
  2,
  1,
  9
};

String waitingOLED[5]={
  "0",
  "+",
  "X",
  "+",
  "X"
};

// Function Prototypes
void drawLoraInfo();
void drawRS485Info();
void handleHorn();
bool honkIsRequested();
bool isHonkTimeOver();
void startHonking();
void stopHonking();
bool timeIsUp();

// ANZEIGE //

void segment(int seg, bool on){
  
  if(on) {
    /*pwm.setPWM(seg, 0, B); // an 10%
    pwm.setPWM(seg+8, 4096, 0); // an 100%*/
    pwm.setPWM(seg+8, ticks[B_Level][0], ticks[B_Level][1]); // nach Brightness Level
  }
  else {
    //pwm.setPWM(seg+8, 0, 4096); // aus
    pwm.setPWM(seg+8, ticks[0][0], ticks[0][1]); // aus
  }
}

void segment10(int seg, bool on){

  if(on) {      
    /*pwm.setPWM(seg + 7, 0, B); // an 10%
    pwm.setPWM(seg, 4096, 0); // an 100%*/
    pwm.setPWM(seg, ticks[B_Level][0], ticks[B_Level][1]); // nach Brightness Level
  }
  else {
    //pwm.setPWM(seg, 0, 4096); // aus
    pwm.setPWM(seg, ticks[0][0], ticks[0][1]); // aus
  }
}

void displayClock(byte c)
{
  for(int i=0; i < 7; i++){
    segment(i, segs[c % 10][i]); //Einer-Stelle
  } 
  if(c > 9){
    for(int i=0; i < 7; i++){
      segment10(i, segs[c / 10][i]); //Zehner-Stelle
    }
  }
  else { 
      for(int i=0; i < 7; i++){
        pwm.setPWM(i, 0, 4096); // aus
      }
  }
}

void handleHorn() {
  if(honkIsRequested() && !isHonking) {
    startHonking();
  } else if (isHonking && isHonkTimeOver()) {
    stopHonking();
  }
}

bool honkIsRequested(){
  if (timeIsUp() || isHonkRequest ){
    isHonkRequest = false;
    return true;
  }
  return false;
}

bool timeIsUp() {
  return Clock == 0 && pClock != 0;
}

bool isHonkTimeOver() {
  if (millis() - honkStartTime > HONK_DURATION) {
    return true;
  }
  return false;
}

void startHonking() {
  pwm.setPWM(7, 4096, 0); // Horn an
  isHonking = true;
  honkStartTime = millis();
}

void stopHonking() {
  pwm.setPWM(7, 0, 4096); // Horn aus
  isHonking = false;
}

void all_Segments_off(){
  
  for(int i=0; i < 15; i++){

      pwm.setPWM(i, 0, 4096); // aus
      //pwm.setPWM(i+7, 0, 4096); // aus
  }
}

void segmentTest(){

  
  Heltec.display->clear();

  //Heltec.display->drawString(0, 10, "Segment Test");

  Heltec.display->display();

  delay(1000);

  for(int i=0; i < 7; i++){

      //pwm.setPWM(i+8, 0, B); // an 10%
      pwm.setPWM(i+8, 4096, 0); // an
      //pwm.setPWM(i, 0, B); // an 10%
      pwm.setPWM(i, 4096, 0); // an

      
      Heltec.display->clear();
      //Heltec.display->drawString(0, 10, String(i+1));
      Heltec.display->display();
      delay(1000);
  }
  for(int i=0; i < 16; i++){

      pwm.setPWM(i, 0, 4096); // aus
  }
}

void logo(){
  Heltec.display->clear();
  Heltec.display->drawXbm(0,5,logo_width,logo_height,logo_bits);
  Heltec.display->display();
}

void waiting(){
  w_ms = millis();
  w_diff = w_ms - w_lms;
  if (w_diff >= interval) {
    all_Segments_off();
    pwm.setPWM(waiting1er[waitingNR], ticks[B_Level][0], ticks[B_Level][1]); // an 100%
    pwm.setPWM(waiting10er[waitingNR], ticks[B_Level][0], ticks[B_Level][1]); // an 100%

    Heltec.display->clear();
    Heltec.display->drawHorizontalLine(2, 50, 124);  
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->setFont(DSEG14_Classic_Mini_Regular_40);
    Heltec.display->drawString(64 , 1 , waitingOLED[waitingNR]);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(30, 52, "Channel " + String(channel));
    Heltec.display->drawString(95, 52, "RSSI");
    Heltec.display->display();
    
    w_lms = w_ms;
    waitingNR++;
    if (waitingNR > 4){
      waitingNR = 1;
    }
  }  
}

void client_check(){
  ms = millis();
  diff = ms - lms;
  //lms = ms;
  if (diff >= 2100) {
    clientFlag = false;
    RS485mode = false;
    waitingNR = 1;
    all_Segments_off();
    pwm.setPWM(waiting1er[waitingNR], ticks[B_Level][0], ticks[B_Level][1]); // an 100%
    pwm.setPWM(waiting10er[waitingNR], ticks[B_Level][0], ticks[B_Level][1]); // an 100%

    }
}

void showNewData(){
  
  String ClockCommand_T = "T";
  String ClockCommand_Honk = "H";
  String ClockCommand_B = "B";
  if (packet.startsWith(ClockCommand_T)){
    //packet.remove(0, 1);
    pClock = Clock;
    String StClock = packet.substring(1,3);
    //Clock = packet.toInt();
    Clock = StClock.toInt();
    ClockStr = StClock;
    /*if (Clock < 10) {
      ClockStr = "0" + String(Clock);
    }
    else{
      ClockStr = String(Clock);
    }*/
    String StBrighness = packet.substring(3);
    B_Level = StBrighness.toInt();
  Serial.println(Clock);              // Serial.println(receivedChars);      //and determining if it's what is expected
  displayClock(Clock);
    
  Heltec.display->clear();
  Heltec.display->drawHorizontalLine(2, 50, 124);  
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->setFont(DSEG14_Classic_Mini_Regular_40);
  Heltec.display->drawString(64 , 1 , ClockStr);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(30, 52, "Channel " + String(channel));
  //Heltec.display->drawString(95, 52, rssi);
  Heltec.display->display();
  } else if (packet.startsWith(ClockCommand_Honk)){
    startHonking();
  }
}

void cbk(int packetSize) {
  packet ="";
  packSize = String(packetSize,DEC);
  for (int i = 0; i < packetSize; i++){ 
    packet += (char) LoRa.read(); }
  rssi = "RSSI " + String(LoRa.packetRssi(), DEC) ;
  lms = millis();
  clientFlag = true;
  drawLoraInfo();
  showNewData();
  LoRa.receive(); // Test, ob Aufhängen vermieden wird durch "Erinnern" der Lora Funktion
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
     }

     packet += inChar; // Add value to inputstring

    if (packet.length() > 100) // If inputString is too long break while loop
    {
      Serial.println("ERROR");
      break;
    }
  }
}


void setup() {

  //pinMode(Vext,OUTPUT);
  
  //digitalWrite(Vext, HIGH);

  preferences.begin("shot-clock", false);

  channel = preferences.getInt("channel", default_channel);

  band = band_select[channel];
   
  preferences.end();

    //RS-485
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  inputString.reserve(200);
  

  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, false /*PABOOST Enable*/, band /*long BAND*/);
     
  pwm.begin();
  pwm.setPWMFreq(200);  // This is the maximum recommended PWM frequency for LEDs

  all_Segments_off();

  //delay(50);

  //digitalWrite(Vext, LOW);
  
  Heltec.display->init();
  // Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  
  Heltec.display->clear();
  
  //Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
  //Heltec.display->drawString(0, 10, "LED Test abgeschlossen");
  //Heltec.display->drawString(0, 0, "Wait for incoming data...");
  Heltec.display->display();
  
  LoRa.setTxPower(20,RF_PACONFIG_PASELECT_RFO);
  LoRa.setSpreadingFactor(7);

  //ESP32 As access point
  WiFi.mode(WIFI_AP); //Access Point mode
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP(); //Get IP address
  Serial.print("HotSpt IP:");
  Serial.println(myIP);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am Shot-Clock Blue 1");
  });

  server.on("/channel", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", channel_html, processor);
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


  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");

  
  //delay(1000);  // not necessary?
  //LoRa.onReceive(cbk);  // aus Beispiel auskommentiert übernommen
  LoRa.receive();

  lms = millis();
  w_lms = millis();
  pwm.setPWM(waiting1er[waitingNR], ticks[B_Level][0], ticks[B_Level][1]); // an 100%
  pwm.setPWM(waiting10er[waitingNR], ticks[B_Level][0], ticks[B_Level][1]); // an 100%
  waitingNR++;

  Heltec.display->clear();
  Heltec.display->drawHorizontalLine(2, 50, 124);  
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->setFont(DSEG14_Classic_Mini_Regular_40);
  Heltec.display->drawString(64 , 1 , waitingOLED[waitingNR]);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(30, 52, "Channel " + String(channel));
  Heltec.display->drawString(95, 52, "RSSI");
  Heltec.display->display();
}

void drawLoraInfo() {
  Heltec.display->drawString(90, 52, "LoRa");
  Heltec.display->display();
  Serial.println("LoRa");
}

void drawRS485Info() {
  Heltec.display->drawString(90, 52, "RS485");
  Heltec.display->display();
  Serial.println("RS485");
}

void loop() {

  if (RS485mode == false){
  int packetSize = LoRa.parsePacket();
  yield();                                    // to mitigate random occuring hang issue when recieving data
  if (packetSize) { cbk(packetSize);  }
  }
  //RS-485 Test
  RS485receive();

  // print the string when a newline arrives:
  if (stringComplete) {
    

    lms = millis();
    clientFlag = true;
    showNewData();
    handleHorn();
    drawRS485Info();

    // clear the string:
    packet = "";
    stringComplete = false;
  }


  if (clientFlag == false){
    waiting();
    }
  else{
    client_check();
    }
  AsyncElegantOTA.loop(); 
}



/*
void serialEvent() {
  while (Serial2.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    packet += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
*/