#include "board_definitions.h"

/**************************************/ 
/********* user config ****************/
/**************************************/

// WiFi config
// Enter your SSID and PASSWORD
const char *ssid = "Controller";
// password must have at least 8 characters
const char* password = "12345678";

// Screen
// uncomment to flip screen
// #define FLIPSCREEN

// PCB config 
// see board_definitions.h for available configs

#define CONTROLLER_PCB_VERSION V2_1


/**************************************/ 
/********* auto config ****************/
/**************************************/

#if CONTROLLER_PCB_VERSION == NO_BOARD
    #ifdef WIFI_LoRa_32_V2
        static const uint8_t PIN_P_P = 17;
        static const uint8_t PIN_R_P = 2;
        static const uint8_t PIN_R_S = 13;
        static const uint8_t PIN_T = 22;
        static const uint8_t PIN_H = 32;
        static const uint8_t PIN_B = 100;
        static const uint8_t PIN_LED = 25;
    #elif defined(WIFI_LoRa_32_V3)
        static const uint8_t PIN_P_P = 6;
        static const uint8_t PIN_R_P = 5;
        static const uint8_t PIN_R_S = 4;
        static const uint8_t PIN_T = 7;
        static const uint8_t PIN_H = 2;
        static const uint8_t PIN_B = 3;
        static const uint8_t PIN_LED = 35;
    #endif
#elif CONTROLLER_PCB_VERSION == V0_1
    #ifdef WIFI_LoRa_32_V2
        static const uint8_t PIN_P_P = 33;
        static const uint8_t PIN_R_P = 2;
        static const uint8_t PIN_R_S = 23;
        static const uint8_t PIN_T = 22;
        static const uint8_t PIN_H = 32;
        static const uint8_t PIN_B = 17;
        static const uint8_t PIN_LED = 25;
    #elif defined(WIFI_LoRa_32_V3)
        static const uint8_t PIN_P_P = 38;
        static const uint8_t PIN_R_P = 26;
        static const uint8_t PIN_R_S = 34;
        static const uint8_t PIN_T = 39;
        static const uint8_t PIN_H = 45;
        static const uint8_t PIN_B = 46;
        static const uint8_t PIN_LED = 35;
    #endif
#elif CONTROLLER_PCB_VERSION == V2_1
    #ifdef WIFI_LoRa_32_V2
        static const uint8_t PIN_P_P = 39;
        static const uint8_t PIN_R_P = 32;
        static const uint8_t PIN_R_S = 33;
        static const uint8_t PIN_T = 23;
        static const uint8_t PIN_H = 2;
        static const uint8_t PIN_B = 17;
        static const uint8_t PIN_LED = 25;
    #elif defined(WIFI_LoRa_32_V3)
        static const uint8_t PIN_P_P = 42;  //J1
        static const uint8_t PIN_R_P = 39;  //J2
        static const uint8_t PIN_R_S = 38;  //J3
        static const uint8_t PIN_T = 34;    //J4
        static const uint8_t PIN_H = 26;    //J5
        static const uint8_t PIN_B = 20;    //J6
        static const uint8_t PIN_LED = 35;
#endif
#endif

//RS-485
#if defined(WIFI_LoRa_32_V2)
    #define RXD2 13
    #define TXD2 12
#endif

#if defined(WIFI_LoRa_32_V3)
    #define RXD2 6
    #define TXD2 5
#endif