

/*
  This is a simple example show the Heltec.LoRa sended data in OLED.

  The onboard OLED display is SSD1306 driver and I2C interface. In order to make the
  OLED correctly operation, you should output a high-low-high(1-0-1) signal by soft-
  ware to OLED's reset pin, the low-level signal at least 5ms.

  OLED pins to ESP32 GPIOs via this connecthin:
  OLED_SDA -- GPIO4
  OLED_SCL -- GPIO15
  OLED_RST -- GPIO16

  by Aaron.Lee from HelTec AutoMation, ChengDu, China
  成都惠利特自动化科技有限公司
  https://heltec.org

  this project also realess in GitHub:
  https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/
#include <Arduino.h>

#include <heltec.h>
#include "images.h"
#include "channel.h"
#include "font.h"

#include <JC_Button.h>

#include <WiFi.h>
#include <WiFiClient.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include <SPIFFS.h>

#include "pinconfig.h"

long band_select[5] = {
    433000000, //  not needed
    433000000, //  Kanal 1
    433500000, //  Kanal 2
    434000000, //  Kanal 3
    434500000  //  Kanal 4
};

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Enter your SSID and PASSWORD
// const char* ssid = "ControllerBlue";
// const char* ssid = "ControllerRed";
// const char* ssid = "Controller3Button";
const char *ssid = "Controller6Button";
 const char* password = "12345678";
//const char *password = "EM2022LAX";

#include <Preferences.h>

Preferences preferences;

int channel;
int default_channel = 1;
long band;

String rssi = "RSSI --";
String packSize = "--";
String packet;

String resetString = "restarting...reset your wifi connection";

const unsigned long
    LONG_PRESS(500),          // we define a "long press" to be 1000 milliseconds.
    BLINK_INTERVAL_LONG(950), // the LED just Blinks for 50ms when a Second counted full down
    BLINK_INTERVAL_SHORT(50);

unsigned long FLEX_INTERVAL(1000);

bool ledState = false;  // current LED status
bool playState = false; // count on/off, starts off
bool startState = true;
bool intervalState = true;
bool smartControl = false;
int ClockStart = 30;
int Clock = ClockStart; // Start Zahl
String ClockStr = "30";
int B_Level = 8;
String ClockMsg;

long lastSendTime = 0;
const long LORA_SEND_INTERVAL_MS = 200;

unsigned long ms;              // current time from millis()
unsigned long msLastStop;      // last time Button Pause
unsigned long msLastPlay;      // last time Button Play
unsigned long msLastCount;     // last time count down
unsigned long msLastStopCount; // last time count/send in stop mode
unsigned long abweichung = 0;  // zählt die gesamte Abweichung
/*
const byte                      // connect a button switch from this pin to ground
    BUTTON_PIN_T(32),           // Button for Play/Pause - one Button
    BUTTON_PIN_P_P(17),           // Button for Play/Pause // previously 2
    BUTTON_PIN_R_P(2),         // Button for Reset-Paused // previously 13
    BUTTON_PIN_R_S(13),         // Button for Reset-started // previously 17
    LED_PIN(25);                // heltec specific pin 25
*/
const byte              // connect a button switch from this pin to ground
    BUTTON_PIN_T(22),   // Button for One Button
    BUTTON_PIN_P_P(33), // Button for Play/Pause
    BUTTON_PIN_R_P(2),  // Button for Reset
    BUTTON_PIN_R_S(23), // Button for Cancel
    BUTTON_PIN_H(32),   // Button for honking
    BUTTON_PIN_B(17),   // Button for nothing
    LED_PIN(25);        // heltec specific pin 25

Button myBtn_T(BUTTON_PIN_T), // define the button
    myBtn_P_P(BUTTON_PIN_P_P),
    myBtn_R_P(BUTTON_PIN_R_P),
    myBtn_H(BUTTON_PIN_H),
    myBtn_B(BUTTON_PIN_B),
    myBtn_R_S(BUTTON_PIN_R_S);

// function prototypes
void sendToClock(String);

void setStart()
{
  msLastCount = ms;
  msLastStop = ms;
  msLastPlay = ms;
}

void Set_Pause_Display()
{
  Heltec.display->fillRect(12, 16, 3, 16);
  Heltec.display->fillRect(18, 16, 3, 16);
}

void Set_Data_Display()
{
  Heltec.display->drawHorizontalLine(2, 50, 124);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->setFont(DSEG14_Classic_Mini_Regular_40);
  Heltec.display->drawString(64, 1, ClockStr);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(64, 52, "Channel " + String(channel));
}

void notifyClients(String message)
{
  ws.textAll(message);
}

void lorasend(String Msg)
{
  if (smartControl == false)
  {
    Heltec.display->clear();
    Set_Data_Display();

    if (!playState)
    {
      Set_Pause_Display();
    }
    Heltec.display->display();
  }

  unsigned long ms2 = millis();

  sendToClock(Msg);
  lastSendTime = ms2;

  // unsigned long ms3 = millis();
  // Serial.print("Dauer Senden: ");
  // Serial.print(ms3 - ms2);
  // Serial.print(" -- ");
}

void updateTime(){
  String Command_T = "T";
  if (Clock < 10)
  {
    ClockStr = "0" + String(Clock);
  }
  else
  {
    ClockStr = String(Clock);
  }
  ClockMsg = Command_T + ClockStr + B_Level;
  lorasend(ClockMsg);
  notifyClients(String(Clock));
  ws.cleanupClients();
  // Serial.println(ClockMsg);
}

void sendToClock(String Msg)
{
  // send packet
  LoRa.setTxPower(20, PA_OUTPUT_RFO_PIN); // test im loop, ob veränderung? was ist das Ergebnis??? bis jetzt nichts aufgefallen
  LoRa.beginPacket();

  /*
   * LoRa.setTxPower(txPower,RFOUT_pin);
   * txPower -- 0 ~ 20
   * RFOUT_pin could be RF_PACONFIG_PASELECT_PABOOST or RF_PACONFIG_PASELECT_RFO
   *   - RF_PACONFIG_PASELECT_PABOOST -- LoRa single output via PABOOST, maximum output 20dBm
   *   - RF_PACONFIG_PASELECT_RFO     -- LoRa single output via RFO_HF / RFO_LF, maximum output 14dBm
   */

  LoRa.print(Msg);
  LoRa.endPacket();

  // RS-485 Test
  Serial2.println(Msg);

  // Serial.print("Msg sent: ");
  // Serial.println(Msg);
}

void Count()
{
  unsigned long t = (msLastStop - msLastCount) + (ms - msLastPlay);

  if (Clock > 0)
  {

    if (t >= BLINK_INTERVAL_SHORT && ledState == false)
    {
      ledState = true;
      // digitalWrite(LED_PIN, ledState);
      // Serial.println("LED aus");
    }

    if (t >= FLEX_INTERVAL)
    {

      // abweichung = (t - FLEX_INTERVAL);
      // Serial.print("Abweichung: ");
      // Serial.print(abweichung);
      // Serial.println("ms");

      // FLEX_INTERVAL = FLEX_INTERVAL + t;
      // Serial.print("Ausgleichendes Interval: ");
      // Serial.print(FLEX_INTERVAL);
      // Serial.println("ms");

      if (intervalState == false)
      {
        FLEX_INTERVAL += LONG_PRESS;
        intervalState = true;
      }

      Clock--;

      if (Clock < ClockStart)
      {
        FLEX_INTERVAL = 1000;
      }

      updateTime();

      ledState = false;
      // digitalWrite(LED_PIN, ledState);
      // Serial.println("LED an");

      setStart();
    }
  }
  else
  {
    playState = !playState;
    Serial.println("DÖÖÖÖÖÖÖÖÖHHD!!!");
  }
}

void stopCount()
{
  if (ledState == true)
  {
    ledState = false;
    // digitalWrite(LED_PIN, ledState);
    msLastStopCount = ms;
  }
  if (ms - msLastStopCount >= 1000)
  {
    updateTime();
    msLastStopCount = ms;
  }
}

void resetClock(bool play)
{
  Clock = ClockStart;
  Serial.println("RESET");

  updateTime();

  setStart();
  playState = play;
  if (play == true)
  {
    notifyClients("true");
  }
  else
  {
    notifyClients("false");
  }
}

void resetClockByHWButton(bool play)
{
  resetClock(play);
  // FLEX_INTERVAL -= LONG_PRESS;
  intervalState = false;
}

void playPause()
{
  if (startState == true)
  {
    setStart();
    startState = false;
  }

  playState = !playState; // ON > OFF oder OFF > ON // fängt OFF an
  if (playState == false)
  {
    notifyClients("false");
    msLastStop = ms; // wenn auf Pause gewechselt, dann Zeit Letzter PAuse Speichern

    if (smartControl == false)
    {
      Set_Pause_Display();
      Heltec.display->display();
    }
  }
  else
  {
    notifyClients("true");
    msLastPlay = ms; // wenn auf Play gewechselt, dann Zeit Letztes Play Speichern

    if (smartControl == false)
    {
      Heltec.display->clear();
      Set_Data_Display();
      Heltec.display->display();
    }
  }
}

void sendStartTime(int T)
{
  String SW = "SW";
  String SWM = SW + T;
  notifyClients(SWM);
}

void setTime(int T)
{

  smartControl = true;
  Heltec.display->displayOff();

  ClockStart = T;
  if (ClockStart < 1)
  {
    ClockStart = 1;
  }
  if (ClockStart > 99)
  {
    ClockStart = 99;
  }
  resetClock(false);
  startState = true;
  sendStartTime(ClockStart);

  /*if (smartControl == false){
    Heltec.display->clear();
    Set_Pause_Display();
    Set_Data_Display();
    Heltec.display->display();
  }*/
}

void set_channel(int ch)
{
  channel = ch;
  preferences.begin("shot-clock", false);
  preferences.putInt("channel", channel);
  Serial.println("Channel " + channel);
  preferences.end();
  delay(1000);
  ESP.restart();
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    if (strcmp((char *)data, "reset") == 0)
    {
      smartControl = true;
      Heltec.display->displayOff();
      resetClock(true);
    }
    if (strcmp((char *)data, "playpause") == 0)
    {
      smartControl = true;
      Heltec.display->displayOff();
      playPause();
    }
    if (strcmp((char *)data, "setTime30") == 0)
    {
      setTime(30);
    }
    if (strcmp((char *)data, "setTimePlus10") == 0)
    {
      setTime(ClockStart + 10);
    }
    if (strcmp((char *)data, "setTimeMinus10") == 0)
    {
      setTime(ClockStart - 10);
    }
    if (strcmp((char *)data, "setTimePlus5") == 0)
    {
      setTime(ClockStart + 5);
    }
    if (strcmp((char *)data, "setTimeMinus5") == 0)
    {
      setTime(ClockStart - 5);
    }
    if (strcmp((char *)data, "setTimePlus1") == 0)
    {
      setTime(ClockStart + 1);
    }
    if (strcmp((char *)data, "setTimeMinus1") == 0)
    {
      setTime(ClockStart - 1);
    }
    if (strcmp((char *)data, "SW") == 0)
    {
      smartControl = true;
      Heltec.display->displayOff();
      sendStartTime(ClockStart);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void startHonking()
{
  String command_H = "H";
  sendToClock(command_H);
}

void sendBCommand()
{
  String command_B = "B";
  sendToClock(command_B);
}

void sendTimeEveryXms() {
  long now = millis();
  if (now > lastSendTime + LORA_SEND_INTERVAL_MS) {
    sendToClock(ClockMsg);
    lastSendTime = now;
  }
}

String processor(const String &var)
{
  String links = "";
  if (var == "LINKPLACEHOLDER")
  {
    for (int c = 1; c <= 4; c++)
    {
      links += "<a class=\"pp-button";
      links += channel == c ? " pp-is-active" : "";
      links += "\" href=\"/" + String(c) + "\">Channel " + String(c) + "</a>";
    }
  }
  return links;
}

String settingsProcessor(const String &var)
{
  if (var == "SELECTED_BRIGHTNESS_LEVEL1" && B_Level == 1)
  {
    return "selected";
  }
  else if (var == "SELECTED_BRIGHTNESS_LEVEL2" && B_Level == 2)
  {
    return "selected";
  }
  else if (var == "SELECTED_BRIGHTNESS_LEVEL3" && B_Level == 3)
  {
    return "selected";
  }
  else if (var == "SELECTED_BRIGHTNESS_LEVEL4" && B_Level == 4)
  {
    return "selected";
  }
  else if (var == "SELECTED_BRIGHTNESS_LEVEL5" && B_Level == 5)
  {
    return "selected";
  }
  else if (var == "SELECTED_BRIGHTNESS_LEVEL6" && B_Level == 6)
  {
    return "selected";
  }
  else if (var == "SELECTED_BRIGHTNESS_LEVEL7" && B_Level == 7)
  {
    return "selected";
  }
  else if (var == "SELECTED_BRIGHTNESS_LEVEL8" && B_Level == 8)
  {
    return "selected";
  }
  return String();
}

void loadChannelFromEEPROM()
{
  preferences.begin("shot-clock", false);
  channel = preferences.getInt("channel", default_channel);
  band = band_select[channel];
  preferences.end();
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void initWebserver()
{
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", String(), false); });

  server.on("/controller/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  request->send(SPIFFS, "/controller.html", String(), false);
  smartControl = true;
  Heltec.display->displayOff(); });

  server.on("/settings/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/settings.html", String(), false, settingsProcessor); });

  server.on("/brightness", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (request->hasParam("b")){
      B_Level = request->getParam("b")->value().toInt();
      request->send_P(200, "text/html", "brightness changed");
    }
    else{
      request->send(400, "text/plain", "missing parameters");
    } });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/style.css", "text/css"); });

  server.on("/javascript.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/javascript.js", "application/javascript"); });

  server.on("/piep.mp3", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/piep.mp3"); });

  server.on("/longpiep.mp3", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/longpiep.mp3"); });

  server.on("/digital-7-mono.ttf", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/digital-7-mono.ttf"); });

  server.on("/digital-7-mono.woff", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/digital-7-mono.woff"); });

  server.on("/digital-7-mono.woff2", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/digital-7-mono.woff2"); });

  server.on("/channel", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", channel_html, processor); });

  /*    server.on("/channel/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/channel.html", String(), false);
  });*/

  server.on("/1", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    request->send(200, "text/plain", resetString);
    set_channel(1); });

  server.on("/2", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    request->send(200, "text/plain", resetString);
    set_channel(2); });

  server.on("/3", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    request->send(200, "text/plain", resetString);
    set_channel(3); });

  server.on("/4", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    request->send(200, "text/plain", resetString);
    set_channel(4); });
}

void initButtons() {
  myBtn_T.begin(); // initialize the button object
  myBtn_P_P.begin();
  myBtn_R_P.begin();
  myBtn_R_S.begin();
  myBtn_H.begin();
  myBtn_B.begin();
  pinMode(LED_PIN, OUTPUT); // set the LED pin as an output
}

//===============================================================
// Setup
//===============================================================

void setup()
{

  loadChannelFromEEPROM();

  // RS-485
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, false /*PABOOST Enable*/, band /*long BAND*/);

  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  Heltec.display->init();
  // Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);

  // ESP32 As access point
  WiFi.mode(WIFI_AP); // Access Point mode
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP(); // Get IP address
  Serial.print("HotSpt IP:");
  Serial.println(myIP);

  initWebSocket();

  initWebserver();

  AsyncElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();                 // Start server

  LoRa.setTxPower(20, PA_OUTPUT_RFO_PIN);
  LoRa.setSpreadingFactor(7);

  Heltec.display->clear();
  Set_Pause_Display();
  Set_Data_Display();
  Heltec.display->display();

  initButtons();

  // digitalWrite(LED_PIN, ledState);  // LED an, da zu Beginn Pause
  ms = millis();
  msLastStop = ms;
  msLastStopCount = ms;
}

// the list of possible states for the state machine. This state machine has a fixed
// sequence of states, i.e. ONOFF --> TO_RESET --> Reset --> TO_RESET --> ONOFF
// note that while the user perceives two "modes", i.e. ON/OFF mode and rapid blink mode,
// two extra states are needed in the state machine to transition between these modes.
enum states_t
{
  INITIAL,
  TOONOFF,
  ONOFF,
  RESET
};

void loop()
{
  sendTimeEveryXms();

  static states_t STATE; // current state machine state
  ms = millis();         // record the current time
  myBtn_T.read();        // read the button
  myBtn_P_P.read();      // read the button
  myBtn_R_P.read();      // read the button
  myBtn_R_S.read();      // read the button
  myBtn_H.read();        // read the button
  myBtn_B.read();        // read the button

  // hier OTA_Loop

  switch (STATE)
  {
    // this state watches for short and long presses, switches ON/OFF for
    // short presses, and moves to the RESET state for long presses.

  case INITIAL:
    if (myBtn_T.wasPressed())
    {
      // Serial.println("Initial done");   //Initial State wird benötigt, weil anscheinend
      // beim Start einmal ein Button Release ausgelöst wird.
      smartControl = false;
      Heltec.display->displayOn();

      playPause();
      STATE = TOONOFF;
    }
    if (myBtn_P_P.wasPressed())
    {
      // Serial.println("Initial done");   //Initial State wird benötigt, weil anscheinend
      // beim Start einmal ein Button Release ausgelöst wird.
      smartControl = false;
      Heltec.display->displayOn();

      playPause();
      STATE = TOONOFF;
    }
    else
    {
      if (playState == false)
      {
        stopCount(); // aktuell: schaltet LED an als User Feedback für Pause
      }
      else
      {
        Count();
      }
      AsyncElegantOTA.loop();
    }
    break;

  case TOONOFF:
    if (myBtn_T.wasReleased())
    {
      STATE = ONOFF;
    }
    if (myBtn_P_P.wasReleased())
    {
      STATE = ONOFF;
    }
    else
    {
      if (playState == false)
      {
        stopCount(); // aktuell: schaltet LED an als User Feedback für Pause
      }
      else
      {
        Count();
      }
      AsyncElegantOTA.loop();
    }
    break;

  case ONOFF:
    if (myBtn_T.wasReleased())
    {
      smartControl = false;
      Heltec.display->displayOn();
      playPause();
    }
    else if (myBtn_P_P.wasPressed())
    {
      smartControl = false;
      Heltec.display->displayOn();
      playPause();
    }

    else if (myBtn_T.pressedFor(LONG_PRESS))
    {
      smartControl = false;
      Heltec.display->displayOn();
      resetClockByHWButton(true);
      FLEX_INTERVAL -= LONG_PRESS;
      STATE = RESET;
    }

    else if (myBtn_R_P.wasPressed())
    {
      resetClockByHWButton(true);
    }

    else if (myBtn_R_S.wasPressed())
    {
      resetClockByHWButton(false);
    }
    else if (myBtn_H.wasReleased())
    {
      startHonking();
    }
    else if (myBtn_B.wasReleased())
    {
      sendBCommand();
    }

    else
    {
      if (playState == false)
      {
        stopCount(); // aktuell: schaltet LED an als User Feedback für Pause
      }
      else
      {
        Count();
      }
      AsyncElegantOTA.loop();
    }
    break;

  case RESET:

    if (myBtn_T.wasReleased())
    {
      STATE = ONOFF;
    }
    else if (myBtn_T.pressedFor(2500))
    {
      smartControl = false;
      Heltec.display->displayOn();
      resetClockByHWButton(false);
      STATE = TOONOFF;
    }
    else
    {
      if (playState == false)
      {
        stopCount(); // aktuell: schaltet LED an als User Feedback für Pause
      }
      else
      {
        Count();
      }
      AsyncElegantOTA.loop();
    }
    break;
  }
}
