#include "board_definitions.h"

// WiFi config

const char* ssid = "ShotClockDisplay";
// password must have at least 8 characters
const char* password = "12345678";


// PCB config

#define DISPLAY_PCB_VERSION V0_2_HACK

//Pin config

//RS-485
#if defined(WIFI_LoRa_32_V2)
    #define RXD2 13
    #define TXD2 14
#endif

#if defined(WIFI_LoRa_32_V3)
    #define RXD2 6
    #define TXD2 5
#endif

//I2C
#if DISPLAY_PCB_VERSION >= V0_2_HACK
    #ifdef WIFI_LoRa_32_V3
        static const uint8_t SDA_LED = 38;
        static const uint8_t SCL_LED = 39;
    #elif defined(WIFI_LoRa_32_V2)
        static const uint8_t SDA_LED = 33;
        static const uint8_t SCL_LED = 32;
    #endif
#endif