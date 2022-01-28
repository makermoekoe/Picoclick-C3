#include "esp_log.h"

#include "espnow_basic_config.h"

static const char *TAG = "My_Master";

// Your task to handle received my_data_t
void my_data_receive(const uint8_t *sender_mac_addr, const my_data_t *data){
    ESP_LOGI(TAG, "Data from "MACSTR": Random Value - %u, Button - %s",
                MAC2STR(sender_mac_addr), 
                data->random_value, 
                data->button_pushed ? "Pushed" : "Released");
}