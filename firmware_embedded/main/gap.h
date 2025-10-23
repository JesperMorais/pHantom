#pragma once

#include <stdint.h>

#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_URI_PREFIX_HTTPS 0x17
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

/* GAP initialization */
int gap_init();

/* Initialize advertising (called after BLE stack sync) */
void adv_init(void);

/* Update pH sensor value in advertising data */
void gap_update_ph_value(float ph_value, uint8_t battery_pct);

/* Restart advertising with updated data */
void gap_restart_advertising(void);