#include "gap.h"
#include "services/gap/ble_svc_gap.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/util/util.h"

const char* DEVICE_NAME = "Jesp_Device";
static const char* TAG = "GAP";

/* BLE address variables */
static uint8_t own_addr_type;
static uint8_t addr_val[6] = {0};

/* Manufacturer-specific data structure for pH sensor */
#define COMPANY_ID 0xFFFF  /* Test company ID - replace with your own */

struct sensor_data_payload {
    uint16_t company_id;    /* Company identifier (little-endian) */
    uint8_t  data_type;     /* 0x01 = pH sensor data */
    uint16_t ph_value;      /* pH * 100 (e.g., 7.45 -> 745) */
    uint8_t  battery_level; /* Battery percentage 0-100 */
    uint16_t sequence;      /* Packet sequence number */
} __attribute__((packed));

/* Global sensor data */
static struct sensor_data_payload sensor_data = {
    .company_id = COMPANY_ID,
    .data_type = 0x01,
    .ph_value = 700,        /* Default: pH 7.00 */
    .battery_level = 100,   /* Default: 100% */
    .sequence = 0
};

/* Helper function to format BLE address as string */
static void format_addr(char *addr_str, const uint8_t *addr) {
    sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X",
            addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}

/* Function to update pH sensor value in advertising data */
void gap_update_ph_value(float ph_value, uint8_t battery_pct) {
    sensor_data.ph_value = (uint16_t)(ph_value * 100.0f);
    sensor_data.battery_level = battery_pct;
    sensor_data.sequence++;

    ESP_LOGI(TAG, "Updated sensor data: pH=%.2f, Battery=%d%%, Seq=%d",
             ph_value, battery_pct, sensor_data.sequence);
}

static void start_advertising(void) {
    /* Local variables */
    int rc = 0;
    const char *name;
    struct ble_hs_adv_fields adv_fields = {0};
    struct ble_hs_adv_fields rsp_fields = {0};
    struct ble_gap_adv_params adv_params = {0};

    /* Set flags (required) */
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Set device name (shortened to fit in 31 bytes) */
    name = ble_svc_gap_device_name();
    adv_fields.name = (uint8_t *)name;
    adv_fields.name_len = strlen(name);
    adv_fields.name_is_complete = 1;

    /* Set manufacturer-specific data with pH sensor reading */
    adv_fields.mfg_data = (uint8_t *)&sensor_data;
    adv_fields.mfg_data_len = sizeof(sensor_data);

    /* NOTE: Removed tx_pwr_lvl and appearance to stay under 31 byte limit
     * Current size: Flags(3) + Name(14) + MfgData(9) = 26 bytes */

    /* Set advertising data */
    rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set advertising data, error code: %d", rc);
        return;
    }

    /* Use scan response for additional data if needed */
    rsp_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    rsp_fields.tx_pwr_lvl_is_present = 1;
    rsp_fields.appearance = BLE_GAP_APPEARANCE_GENERIC_TAG;
    rsp_fields.appearance_is_present = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGW(TAG, "failed to set scan response data, error code: %d", rc);
        /* Non-critical, continue anyway */
    }

    /* Configure advertising parameters for connectionless broadcasting */
    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;  /* Non-connectable */
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;   /* General discoverable */
    adv_params.itvl_min = 160;  /* 100ms (units of 0.625ms) */
    adv_params.itvl_max = 160;  /* 100ms - fixed interval */

    /* Start advertising */
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to start advertising, error code: %d", rc);
        return;
    }

    ESP_LOGI(TAG, "advertising started successfully (non-connectable mode)");
}

/* Restart advertising with updated sensor data */
void gap_restart_advertising(void) {
    int rc;

    /* Stop current advertising */
    rc = ble_gap_adv_stop();
    if (rc != 0 && rc != BLE_HS_EALREADY) {
        ESP_LOGE(TAG, "failed to stop advertising, error code: %d", rc);
    }

    /* Restart advertising with new data */
    start_advertising();
}

void adv_init(void) {
    /* Local variables */
    int rc = 0;
    char addr_str[18] = {0};

    /* Make sure we have proper BT identity address set */
    rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "device does not have any available bt address!");
        return;
    }

    /* Figure out BT address to use while advertising */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to infer address type, error code: %d", rc);
        return;
    }

    /* Copy device address to addr_val */
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to copy device address, error code: %d", rc);
        return;
    }
    format_addr(addr_str, addr_val);
    ESP_LOGI(TAG, "device address: %s", addr_str);

    /* Start advertising. */
    start_advertising();
}

int gap_init(){
    int rc = 0;

    // INIT GAP service
    ble_svc_gap_init();

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