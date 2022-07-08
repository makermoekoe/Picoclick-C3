#include "esp_system.h"
#include "esp_log.h"

#include "espnow_basic_config.h"

static const char *TAG = "My_Slave";

// Your function to populate a my_data_t to send
void my_data_populate(my_data_t *data)
{
    ESP_LOGI(TAG, "Populating my_data_t");
    data->random_value = esp_random();
    data->button_pushed = esp_random();
}