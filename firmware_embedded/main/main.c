#include <stdio.h>
#include "esp_hidh_nimble.h"
#include "esp_check.h"
#include "nvs_flash.h"

char* TAG = "MAIN";

void app_main(void)
{
    int rc;
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
    }
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", ret);
    }

    // INIT Nimble stack
    ret = nimble_port_init();
    if (ret != ESP_OK){
        ESP_LOGE(TAG, "Failed to init Nimble stack error code %d", ret);
        return;
    }

    

}