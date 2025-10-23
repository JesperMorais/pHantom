#include "sensor.h"
#include "gap.h"
#include "common.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char* TAG = "SENSOR";

/* ADC handles */
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t adc_cali_handle = NULL;
static bool do_calibration = false;

/* Initialize ADC calibration */
static bool sensor_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "Calibration scheme: Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = PH_SENSOR_BITWIDTH,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "Calibration scheme: Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = PH_SENSOR_BITWIDTH,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    if (calibrated) {
        adc_cali_handle = handle;
    } else {
        ESP_LOGW(TAG, "No calibration scheme available, using raw values");
    }

    return calibrated;
}

/* Initialize ADC for pH sensor */
void sensor_init(void) {
    esp_err_t ret;

    /* Configure ADC oneshot unit */
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = PH_SENSOR_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
        return;
    }

    /* Configure ADC channel */
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = PH_SENSOR_BITWIDTH,
        .atten = PH_SENSOR_ATTEN,
    };
    ret = adc_oneshot_config_channel(adc_handle, PH_SENSOR_ADC_CHANNEL, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(ret));
        return;
    }

    /* Initialize calibration */
    do_calibration = sensor_adc_calibration_init(PH_SENSOR_ADC_UNIT, PH_SENSOR_ADC_CHANNEL, PH_SENSOR_ATTEN);

    ESP_LOGI(TAG, "pH sensor ADC initialized on channel %d (calibration: %s)",
             PH_SENSOR_ADC_CHANNEL, do_calibration ? "enabled" : "disabled");
    ESP_LOGI(TAG, "ADC config: Unit=%d, Channel=%d, Atten=%d, Bitwidth=%d",
             PH_SENSOR_ADC_UNIT, PH_SENSOR_ADC_CHANNEL, PH_SENSOR_ATTEN, PH_SENSOR_BITWIDTH);
}

/* Read pH value from ADC
 *
 * DFRobot Gravity Analog pH Sensor Meter Kit V2 (SKU: SEN0161-V2)
 *
 * Hardware connection for ESP32-C6:
 * - Sensor Signal (Blue) → ESP32-C6 GPIO3 (ADC1_CH3)
 * - Sensor VCC (Red)     → ESP32-C6 5V
 * - Sensor GND (Black)   → ESP32-C6 GND
 *
 * Calibration procedure (IMPORTANT!):
 * 1. Prepare pH buffer solutions: pH 4.00, pH 6.86 (or 7.00), pH 9.18
 * 2. At room temperature (25°C), rinse probe with distilled water
 * 3. Place in pH 6.86/7.00 buffer, wait 1 minute, note voltage (should be ~2.5V)
 * 4. Place in pH 4.00 buffer, wait 1 minute, note voltage
 * 5. Calculate slope: (voltage_pH7 - voltage_pH4) / (7.0 - 4.0) = mV per pH unit
 * 6. Update ph_offset and mv_per_ph constants below
 *
 * Default calibration values (adjust after real calibration):
 * - pH 7.0 (neutral) ≈ 2500mV (ph_offset)
 * - Slope ≈ -59.16mV per pH unit at 25°C (Nernst equation, negative because voltage decreases as pH increases)
 */
float sensor_read_ph(void) {
    if (adc_handle == NULL) {
        ESP_LOGE(TAG, "ADC not initialized");
        return 7.0f;  /* Return neutral pH as fallback */
    }

    /* Read multiple samples for averaging */
    const int num_samples = 10;
    uint32_t voltage_sum = 0;
    esp_err_t ret;

    int adc_raw_first = 0;
    int adc_raw_last = 0;

    for (int i = 0; i < num_samples; i++) {
        int adc_raw = 0;
        ret = adc_oneshot_read(adc_handle, PH_SENSOR_ADC_CHANNEL, &adc_raw);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(ret));
            continue;
        }

        if (i == 0) adc_raw_first = adc_raw;
        if (i == num_samples - 1) adc_raw_last = adc_raw;

        int voltage_mv = 0;
        if (do_calibration) {
            /* Convert raw value to voltage using calibration */
            ret = adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &voltage_mv);
            if (ret == ESP_OK) {
                voltage_sum += voltage_mv;
            }
        } else {
            /* Fallback: rough estimation without calibration
             * For ESP32 with 12-bit ADC and 3.3V reference:
             * voltage_mv ≈ (adc_raw / 4095) * 3300 */
            voltage_mv = (adc_raw * 3300) / 4095;
            voltage_sum += voltage_mv;
        }

        vTaskDelay(pdMS_TO_TICKS(10));  /* 10ms between samples */
    }

    ESP_LOGI(TAG, "ADC raw readings: first=%d, last=%d (range: 0-4095)", adc_raw_first, adc_raw_last);

    uint32_t avg_voltage_mv = voltage_sum / num_samples;

    /* Convert voltage to pH value
     *
     * ⚠️  TEMPORARY CALIBRATION based on pH 7.0 buffer reading
     *
     * Current calibration (measured):
     * - pH 7.0 buffer reads: 2010mV
     * - pH 4.0 buffer reads: 2010mV (SAME - sensor needs activation!)
     *
     * Using theoretical slope until sensor is properly activated.
     *
     * TO ACTIVATE SENSOR:
     * 1. Leave electrode in pH 7.0 buffer for 8-24 hours
     * 2. Test again with fresh pH 4.0 and pH 7.0 buffers
     * 3. Update calibration values here after proper activation
     *
     * After proper calibration:
     * - neutral_voltage_mv: Your measured pH 7.0 voltage
     * - acid_slope: (voltage_pH7 - voltage_pH4) / (7.0 - 4.0)
     */
    const float neutral_voltage_mv = 2010.0f;  // Measured: pH 7.0 → 2010mV
    const float acid_slope = -59.16f;          // Theoretical (update after activation!)
    const float neutral_ph = 7.0f;             // Reference pH

    /* Calculate pH from voltage using linear equation:
     * pH = neutral_ph + (measured_voltage - neutral_voltage) / acid_slope
     */
    float ph_value = neutral_ph + ((float)avg_voltage_mv - neutral_voltage_mv) / acid_slope;

    /* Clamp to valid pH range */
    if (ph_value < 0.0f) ph_value = 0.0f;
    if (ph_value > 14.0f) ph_value = 14.0f;

    ESP_LOGI(TAG, "ADC: %lumV -> pH: %.2f", avg_voltage_mv, ph_value);

    return ph_value;
}

/* Sensor reading task */
static void sensor_task(void *pvParameters) {
    ESP_LOGI(TAG, "Sensor task started");

    /* Wait for BLE stack to be ready */
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        /* Read pH sensor */
        float ph_value = sensor_read_ph();

        /* For battery level, you could read from a battery monitoring IC
         * or calculate based on supply voltage. For now, using placeholder */
        uint8_t battery_pct = 100;  /* TODO: Implement real battery monitoring */

        /* Update advertising data with new sensor reading */
        gap_update_ph_value(ph_value, battery_pct);
        gap_restart_advertising();

        ESP_LOGI(TAG, "Broadcasting pH=%.2f, Battery=%d%%", ph_value, battery_pct);

        /* Wait for next update interval */
        vTaskDelay(pdMS_TO_TICKS(SENSOR_UPDATE_INTERVAL_MS));
    }
}

/* Start sensor reading task */
void sensor_start_task(void) {
    xTaskCreate(
        sensor_task,
        "sensor_task",
        4096,  /* Stack size */
        NULL,
        5,     /* Priority */
        NULL
    );

    ESP_LOGI(TAG, "Sensor task created (update interval: %dms)", SENSOR_UPDATE_INTERVAL_MS);
}
