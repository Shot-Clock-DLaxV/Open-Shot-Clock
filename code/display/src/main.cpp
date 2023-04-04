
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

#include <Preferences.h>
#include <Adafruit_PWMServoDriver.h>

#include "config.h"
#include "LEDs.h"
#include "Horn.h"

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

bool RS485mode = false;

AsyncWebServer server(80);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
LEDs leds(pwm);
Horn horn(pwm);


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
bool clientFlag = false;
long int currentTime = 0;
long int previousTime = 0;

long band_select[5]={
  433000000,   //  not needed
  433000000,   //  Kanal 1
  433500000,   //  Kanal 2  
  434000000,   //  Kanal 3  
  434500000    //  Kanal 4
};

// Function Prototypes
void drawLoraInfo();
void drawRS485Info();
bool timeIsUp();

bool timeIsUp() {
  return currentTime == 0 && previousTime != 0;
}

void waitingHeltecDisplay(){
    Heltec.display->clear();
    Heltec.display->drawHorizontalLine(2, 50, 124);  
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->setFont(ArialMT_Plain_24);
    Heltec.display->drawString(64 , 1 , "waiting");
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(30, 52, "Channel " + String(channel));
    Heltec.display->drawString(95, 52, "RSSI");
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

void handlePacket(){
  
  String setTimeCommand = "T";
  String honkCommand = "H";
  if (packet.startsWith(setTimeCommand)){
    previousTime = currentTime;
    String currentTimeString = packet.substring(1,3);
    currentTime = currentTimeString.toInt();

    String brigthnessString = packet.substring(3);
    leds.setBrightnessLevel(brigthnessString.toInt());
    Serial.println(currentTime);              // Serial.println(receivedChars);      //and determining if it's what is expected
    leds.displayClock(currentTime);
      
    Heltec.display->clear();
    Heltec.display->drawHorizontalLine(2, 50, 124);  
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->setFont(DSEG14_Classic_Mini_Regular_40);
    Heltec.display->drawString(64 , 1 , currentTimeString);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(30, 52, "Channel " + String(channel));
    //Heltec.display->drawString(95, 52, rssi);
    Heltec.display->display();
  } else if (packet.startsWith(honkCommand)){
    horn.requestHonk();
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
  handlePacket();
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

void initWebserver() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am your Shot Clock :)");
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

}

void initChannelFromEEPROM(){
  preferences.begin("shot-clock", false);

  channel = preferences.getInt("channel", default_channel);

  band = band_select[channel];
   
  preferences.end();
}


void setup() {

  initChannelFromEEPROM();

    //RS-485
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  inputString.reserve(200);
  

  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, false /*PABOOST Enable*/, band /*long BAND*/);
     
  pwm.begin();
  pwm.setPWMFreq(200);  // This is the maximum recommended PWM frequency for LEDs

  leds.allSegmentsOff();

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
  
  initWebserver();

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");

  
  //delay(1000);  // not necessary?
  //LoRa.onReceive(cbk);  // aus Beispiel auskommentiert übernommen
  LoRa.receive();

  leds.showWaitingAnimation();
  waitingHeltecDisplay();
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

  AsyncElegantOTA.loop(); 

  if (timeIsUp()) {
    horn.requestHonk();
  }

  horn.handle();
  leds.handle();
}
