#pragma once

#include <stdint.h>
#include "esp_adc/adc_oneshot.h"

/* DFRobot Gravity Analog pH Sensor Meter Kit V2 (SKU: SEN0161-V2)
 *
 * Hardware setup for ESP32-C6-WROOM-1:
 * - Connect sensor signal (blue wire) to GPIO3
 * - Connect VCC (red wire) to 5V
 * - Connect GND (black wire) to GND
 *
 * Pin mapping ESP32-C6:
 * - GPIO3 = ADC1_CH3 (analog input)
 */

/* pH sensor configuration */
#define PH_SENSOR_ADC_UNIT      ADC_UNIT_1
#define PH_SENSOR_ADC_CHANNEL   ADC_CHANNEL_3  /* GPIO3 on ESP32-C6 */
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
