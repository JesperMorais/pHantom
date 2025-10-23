#include "common.h"
#include "gap.h"
#include "sensor.h"

static const char* TAG = "MAIN";

void ble_store_config_init(void);

static void on_stack_reset(int reason){
    ESP_LOGE("MAIN", "on stack reset: Resetting state; reason=%d", reason);
}

static void on_stack_sync(void){
    ESP_LOGI(TAG, "BLE stack synced - initializing advertising");
    /* Initialize advertising when BLE stack is ready */
    adv_init();
}

static void nimble_host_config_init(){
    /*Set host cb*/
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Store host configs*/
    ble_store_config_init();
}

static void nimble_host_task(void* param) {
    ESP_LOGI("NIMBLE", "Nimble host started!");

    nimble_port_run();

    vTaskDelete(NULL);
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

    /* Initialize pH sensor ADC */
    sensor_init();
    ESP_LOGI(TAG, "pH sensor initialized");

    /* Start NimBLE host task */
    xTaskCreate(nimble_host_task, "Nimble Host", 4*1024, NULL, 5, NULL);

    /* Start sensor reading and broadcasting task */
    sensor_start_task();
    ESP_LOGI(TAG, "Sensor task started - broadcasting pH data every %dms", SENSOR_UPDATE_INTERVAL_MS);

    return;
}