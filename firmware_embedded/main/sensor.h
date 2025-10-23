#pragma once

#include <stdint.h>
#include "esp_adc/adc_oneshot.h"

/* DFRobot Gravity Analog pH Sensor Meter Kit V2 (SKU: SEN0161-V2)
 *
 * Hardware setup:
 * - Connect sensor signal (blue wire) to GPIO34
 * - Connect VCC (red wire) to 3.3V or 5V (check your sensor version)
 * - Connect GND (black wire) to GND
 *
 * Note: GPIO34 is input-only and supports analog reading
 */

/* pH sensor configuration */
#define PH_SENSOR_ADC_UNIT      ADC_UNIT_1
#define PH_SENSOR_ADC_CHANNEL   ADC_CHANNEL_6  /* GPIO34 on ESP32 */
#define PH_SENSOR_ATTEN         ADC_ATTEN_DB_12  /* 0-3.3V range */
#define PH_SENSOR_BITWIDTH      ADC_BITWIDTH_12  /* 12-bit resolution (0-4095) */

/* Update interval in milliseconds */
#define SENSOR_UPDATE_INTERVAL_MS  10000  /* 10 seconds (good for battery life) */

/* Initialize pH sensor ADC */
void sensor_init(void);

/* Start sensor reading task */
void sensor_start_task(void);

/* Read current pH value (blocking) */
float sensor_read_ph(void);
