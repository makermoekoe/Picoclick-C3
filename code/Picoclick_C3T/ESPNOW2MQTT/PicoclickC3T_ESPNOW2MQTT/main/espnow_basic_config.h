#ifndef ESPNOW_BASIC_CONFIG_H
#define ESPNOW_BASIC_CONFIG_H

#include <inttypes.h>
#include <stdbool.h>

// Define the structure of your data
typedef struct __attribute__((packed)) {
    int id;
    int value;
    int battery_level;
    int single_tap_duration; // zero to disable
} my_data_t;

// Destination MAC address
// The default address is the broadcast address, which will work out of the box, but the slave will assume every tx succeeds.
// Setting to the master's address will allow the slave to determine if sending succeeded or failed.
//   note: with default config, the master's WiFi driver will log this for you. eg. I (721) wifi:mode : sta (12:34:56:78:9a:bc)
// #define MY_RECEIVER_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
#define MY_RECEIVER_MAC {0x7C, 0xDF, 0xA1, 0x55, 0xB6, 0xB2} //7C:DF:A1:55:B6:B2

#define MY_ESPNOW_PMK "pmk1234567890123"
#define MY_ESPNOW_CHANNEL 1

// #define MY_ESPNOW_ENABLE_LONG_RANGE 1

// #define MY_SLAVE_DEEP_SLEEP_TIME_MS 10000

#endif // ESPNOW_BASIC_CONFIG_H