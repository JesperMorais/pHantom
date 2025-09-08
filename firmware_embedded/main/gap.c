#include "gap.h"
#include "services/gap/ble_svc_gap.h"
#include "esp_log.h"

const char* DEVICE_NAME = "Jesp_Device";
static const char* TAG = "GAP";

int gap_init(){
    int rc = 0;

    // INIT GAP service
    ble_svc_gap_init();

    //Set ble name?? TO fukicng waht?? VOID?
    rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    if (rc != 0){
        ESP_LOGE(TAG, "Failed to set device name %s, error code: %d ", DEVICE_NAME, rc);
        return rc;
    }

    int randomID = 4;
    /* Code example had BLE_GAP_APPEARANCE_GENERIC_TAG??
        We test with random int for now? Could it be some tag for the device?
    */
    rc = ble_svc_gap_device_appearance_set(randomID);
    if (rc != 0){
        ESP_LOGE(TAG, "Failed to set device appearance, error code: %d ", rc);
        return rc;
    }

    return rc;
}