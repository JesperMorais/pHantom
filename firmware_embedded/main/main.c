#include <stdio.h>
#include "esp_check.h"
#include "nvs_flash.h"
#include "gap.h"
#include "nimble/nimble_port.h"
#include "host/ble_hs.h"
#include "host/ble_store.h"

static const char* TAG = "MAIN";

static void nimble_host_config_init(){
    /*Set host cb*/
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Store host configs*/
    ble_store_config_init();
}

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

    rc = gap_init();
    if (rc != 0){
        ESP_LOGE(TAG, "Failed to init GAP service, error code: %d ", rc);
        return;
    }

    nimble_host_config_init();

}