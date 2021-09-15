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
#include "heltec.h" 
#include "images.h"
#include "channel.h"
#include "font.h"

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

const char* ssid = "ShotClockBlue1";
//const char* ssid = "ShotClockBlue2";
//const char* ssid = "ShotClockRed1";
//const char* ssid = "ShotClockRed2";
const char* password = "12345678";

AsyncWebServer server(80);


#include <Adafruit_PWMServoDriver.h>

int B = 200; //Brightness between 0 and 4096

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

#include <Preferences.h>

Preferences preferences;

int channel;
int default_channel = 1;
long band;
String rssi = "RSSI --";
String packSize = "--";
String packet ;
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

// ANZEIGE //

void segment(int seg, bool on){
  
  if(on) {
    //pwm.setPWM(seg, 0, B); // an 10%
    pwm.setPWM(seg+8, 4096, 0); // an 100%
  }
  else {
    pwm.setPWM(seg+8, 0, 4096); // aus
  }
}

void segment10(int seg, bool on){

  if(on) {      
    //pwm.setPWM(seg + 7, 0, B); // an 10%
    pwm.setPWM(seg, 4096, 0); // an 100%
  }
  else {
    pwm.setPWM(seg, 0, 4096); // aus
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


void horn(){
  if (Clock == 0 && pClock != 0){
      pwm.setPWM(7, 4096, 0); // Horn an
    }
  else if (Clock == 0 && pClock == 0){
      pwm.setPWM(7, 0, 4096); // Horn aus
    }
  else if (Clock != 0){
      pwm.setPWM(7, 0, 4096); // Horn aus
    }
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
    pwm.setPWM(waiting1er[waitingNR], 4096, 0); // an 100%
    pwm.setPWM(waiting10er[waitingNR], 4096, 0); // an 100%

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
    waitingNR = 1;
    all_Segments_off();
    pwm.setPWM(waiting1er[waitingNR], 4096, 0); // an 100%
    pwm.setPWM(waiting10er[waitingNR], 4096, 0); // an 100%

    }
}

void showNewData(){
  
  String ClockCommand = "T";
  if (packet.startsWith(ClockCommand)){
    packet.remove(0, 1);
    pClock = Clock;
    Clock = packet.toInt();
    if (Clock < 10) {
      ClockStr = "0" + String(Clock);
    }
    else{
      ClockStr = String(Clock);
  }
  Serial.println(Clock);              // Serial.println(receivedChars);      //and determining if it's what is expected
  displayClock(Clock);
  horn();
    
  Heltec.display->clear();
  Heltec.display->drawHorizontalLine(2, 50, 124);  
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->setFont(DSEG14_Classic_Mini_Regular_40);
  Heltec.display->drawString(64 , 1 , ClockStr);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(30, 52, "Channel " + String(channel));
  Heltec.display->drawString(95, 52, rssi);
  Heltec.display->display();
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



void setup() {

  preferences.begin("shot-clock", false);

  channel = preferences.getInt("channel", default_channel);

  band = band_select[channel];
   
  preferences.end();
  
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, band /*long BAND*/);
     
  pwm.begin();
  pwm.setPWMFreq(200);  // This is the maximum recommended PWM frequency for LEDs

  all_Segments_off();
  
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  
  Heltec.display->clear();
  
  //Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
  //Heltec.display->drawString(0, 10, "LED Test abgeschlossen");
  //Heltec.display->drawString(0, 0, "Wait for incoming data...");
  Heltec.display->display();
  
  LoRa.setTxPower(20,RF_PACONFIG_PASELECT_PABOOST);
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
    request->send(200, "text/html", channel_html);
  });

  server.on("/1", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "restarting...");
    set_channel(1);
  });

  server.on("/2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "restarting...");
    set_channel(2);
  });  

  server.on("/3", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "restarting...");
    set_channel(3);    
  });  

  server.on("/4", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "restarting...");
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
  pwm.setPWM(waiting1er[waitingNR], 4096, 0); // an 100%
  pwm.setPWM(waiting10er[waitingNR], 4096, 0); // an 100%
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

void loop() {

  int packetSize = LoRa.parsePacket();
  yield();                                    // to mitigate random occuring hang issue when recieving data
  if (packetSize) { cbk(packetSize);  }
  
  if (clientFlag == false){
    waiting();
    }
  else{
    client_check();
    }
  AsyncElegantOTA.loop(); 
}
