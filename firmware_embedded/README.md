# BLE pH Sensor Beacon - Firmware

ESP32 firmware for a BLE beacon that broadcasts pH sensor data in advertising packets (connectionless).

## Hardware Requirements

### Components
- **ESP32 Development Board** (any ESP32 variant)
- **DFRobot Gravity Analog pH Sensor Meter Kit V2** (SKU: SEN0161-V2)
- pH buffer solutions for calibration (pH 4.00, 6.86/7.00, 9.18)

### Wiring

| pH Sensor Wire | ESP32 Pin | Notes |
|----------------|-----------|-------|
| Signal (Blue)  | GPIO34    | ADC1_CHANNEL_6 (analog input only) |
| VCC (Red)      | 3.3V or 5V | Check your sensor version! |
| GND (Black)    | GND       | Common ground |

**Important:** GPIO34 is input-only and perfect for ADC readings. Do not use output-only GPIOs!

## Software Setup

### Prerequisites
- ESP-IDF v5.5 or later
- Python 3.11+ with ESP-IDF tools

### Build and Flash

```bash
# Navigate to firmware directory
cd firmware_embedded

# Build the project
idf.py build

# Flash to ESP32 (replace COMX with your port)
idf.py -p COMX flash

# Monitor serial output
idf.py -p COMX monitor

# Or do all at once
idf.py -p COMX flash monitor
```

## BLE Broadcasting

### Advertising Data Format

The device broadcasts in **non-connectable mode** (beacon mode) every 100ms with the following data:

**Advertising Packet (26 bytes):**
```
Flags:              3 bytes  (General discoverable, BR/EDR not supported)
Device Name:       14 bytes  ("Jesp_Device")
Manufacturer Data:  9 bytes  (pH sensor data)
```

**Manufacturer Data Structure (9 bytes):**
```c
Byte 0-1:  Company ID (0xFFFF, little-endian) - Test ID
Byte 2:    Data Type (0x01 = pH sensor)
Byte 3-4:  pH Value (uint16, little-endian, pH × 100)
           Example: 745 = pH 7.45
Byte 5:    Battery Level (0-100%)
Byte 6-7:  Sequence Number (uint16, packet counter)
```

**Scan Response (optional):**
```
TX Power Level
Device Appearance (Generic Tag)
```

### Example Data

If pH = 7.45, Battery = 85%, Sequence = 123:
```
0xFF, 0xFF,        // Company ID
0x01,              // Data type
0xE9, 0x02,        // pH 7.45 → 745 → 0x02E9 (little-endian)
0x55,              // Battery 85%
0x7B, 0x00         // Sequence 123 → 0x007B (little-endian)
```

## pH Sensor Calibration

⚠️ **CRITICAL:** You must calibrate the sensor for accurate readings!

### Calibration Procedure

1. **Prepare Materials:**
   - pH 4.00 buffer solution
   - pH 6.86 or 7.00 buffer solution
   - pH 9.18 buffer solution (optional, for verification)
   - Distilled water for rinsing

2. **Calibration Steps:**

   a. **pH 7.0 Calibration (Neutral Point):**
   ```bash
   - Rinse probe with distilled water
   - Place in pH 7.0 buffer solution
   - Wait 1 minute for stabilization
   - Read voltage from serial monitor
   - Note: "ADC: XXXXmV -> pH: YY.YY"
   - Record the XXXXmV value as neutral_voltage_mv
   ```

   b. **pH 4.0 Calibration (Slope):**
   ```bash
   - Rinse probe with distilled water
   - Place in pH 4.0 buffer solution
   - Wait 1 minute for stabilization
   - Read voltage from serial monitor
   - Record the XXXXmV value as ph4_voltage_mv
   ```

   c. **Calculate Slope:**
   ```
   acid_slope = (neutral_voltage_mv - ph4_voltage_mv) / (7.0 - 4.0)

   Example:
   - pH 7.0 reads 2480mV
   - pH 4.0 reads 2657mV
   - Slope = (2480 - 2657) / 3.0 = -59.0 mV/pH ✓
   ```

3. **Update Firmware:**

   Edit `main/sensor.c` at line ~168:
   ```c
   const float neutral_voltage_mv = 2480.0f;  // Your pH 7.0 voltage
   const float acid_slope = -59.0f;           // Your calculated slope
   const float neutral_ph = 7.0f;             // Keep at 7.0
   ```

4. **Rebuild and Flash:**
   ```bash
   idf.py build flash
   ```

5. **Verify:**
   - Test with pH 9.18 buffer
   - Should read close to 9.18 (±0.2 pH is acceptable)

### Typical Values

**DFRobot Gravity V2 specifications:**
- Neutral voltage (pH 7.0): 2450-2550 mV
- Acid slope: -55 to -60 mV/pH (at 25°C)
- Temperature coefficient: ~0.003 pH/°C

**Without calibration:**
- You'll see pH 14.00 (voltage too low) or pH 0.00 (voltage too high)
- Readings will be inaccurate by 1-3 pH units

## Configuration Options

### Adjust Update Interval

Edit `main/sensor.h` line 23:
```c
#define SENSOR_UPDATE_INTERVAL_MS  10000  // 10 seconds (default)
```

Recommendations:
- **Fast (1000ms):** Real-time monitoring, high power consumption
- **Normal (5000ms):** Balance between responsiveness and battery life
- **Slow (10000ms+):** Maximum battery life, slower updates

### Change ADC Channel

If you need to use a different GPIO, edit `main/sensor.h`:
```c
#define PH_SENSOR_ADC_CHANNEL   ADC_CHANNEL_X  // See ESP32 pinout
```

Valid ADC1 channels on ESP32:
- ADC1_CH0 = GPIO36
- ADC1_CH3 = GPIO39
- ADC1_CH4 = GPIO32
- ADC1_CH5 = GPIO33
- ADC1_CH6 = GPIO34 ← Default
- ADC1_CH7 = GPIO35

**Important:** Use ADC1 channels only (not ADC2) because ADC2 conflicts with WiFi!

### Change Device Name

Edit `main/gap.c` line 10:
```c
const char* DEVICE_NAME = "Jesp_Device";
```

**Warning:** Keep name ≤ 13 characters to stay under 31-byte advertising limit!

### Change Company ID

Edit `main/gap.c` line 18:
```c
#define COMPANY_ID 0xFFFF  // Replace with your registered ID
```

Get a registered Company ID from Bluetooth SIG if deploying commercially.

## Serial Monitor Output

Expected log output:
```
I (367) MAIN: pH sensor initialized
I (437) GAP: device address: F0:F5:BD:0E:5F:5A
I (447) GAP: advertising started successfully (non-connectable mode)
I (2557) SENSOR: ADC: 2480mV -> pH: 7.12
I (2557) GAP: Updated sensor data: pH=7.12, Battery=100%, Seq=1
I (2557) SENSOR: Broadcasting pH=7.12, Battery=100%
```

## Receiving the Data

### Using nRF Connect (Mobile App)

1. Install **nRF Connect** (iOS/Android)
2. Scan for devices
3. Find "Jesp_Device"
4. Look for "Manufacturer data" field:
   ```
   0xFFFF: 01 E9 02 64 01 00
           │  │  │  │  └─ Sequence
           │  │  │  └─ Battery (100)
           │  └──┴─ pH (745 = 7.45)
           └─ Type (0x01)
   ```

### Using Python (Example)

```python
from bleak import BleakScanner

async def scan():
    devices = await BleakScanner.discover()
    for device in devices:
        if device.name == "Jesp_Device":
            mfg_data = device.metadata.get("manufacturer_data", {})
            if 0xFFFF in mfg_data:
                data = mfg_data[0xFFFF]
                data_type = data[0]
                ph_raw = int.from_bytes(data[1:3], 'little')
                battery = data[3]
                sequence = int.from_bytes(data[4:6], 'little')

                ph = ph_raw / 100.0
                print(f"pH: {ph:.2f}, Battery: {battery}%, Seq: {sequence}")
```

## Troubleshooting

### "ADC: 924mV -> pH: 14.00"
- **Cause:** No sensor connected or loose wiring
- **Fix:** Check connections, ensure sensor is powered

### "Error code: 4" (BLE_HS_EMSGSIZE)
- **Cause:** Advertising data > 31 bytes
- **Fix:** Shorten device name or remove data fields

### Unstable pH Readings
- **Cause:** Uncalibrated sensor or temperature drift
- **Fix:** Perform calibration procedure, store probe properly

### pH Always Shows 0.00 or 14.00
- **Cause:** Wrong calibration constants
- **Fix:** Re-calibrate with buffer solutions

### BLE Not Advertising
- **Cause:** NimBLE stack not synced
- **Fix:** Check serial logs for "BLE stack synced" message

## Project Structure

```
firmware_embedded/
├── main/
│   ├── main.c           # Application entry, NimBLE init
│   ├── gap.c/gap.h      # BLE GAP advertising logic
│   ├── sensor.c/sensor.h # pH sensor ADC reading
│   ├── common.h         # Shared includes and defines
│   └── CMakeLists.txt   # Component configuration
├── CMakeLists.txt       # Project configuration
├── sdkconfig            # ESP-IDF configuration
└── README.md            # This file
```

## Battery Optimization Tips

1. **Increase update interval:** 30-60 seconds
2. **Reduce advertising rate:** Change `itvl_min/max` to 500-1000ms
3. **Enable deep sleep:** Sleep between readings
4. **Disable logging:** Set log level to ERROR only
5. **Lower TX power:** Reduce BLE transmit power

Example deep sleep implementation:
```c
#include "esp_sleep.h"

// In sensor_task() loop:
esp_sleep_enable_timer_wakeup(30 * 1000000); // 30 seconds
esp_deep_sleep_start();
```

## License

[Add your license here]

## Support

For issues or questions, please open an issue on GitHub.

## Credits

- ESP-IDF by Espressif Systems
- NimBLE stack by Apache
- DFRobot Gravity pH Sensor documentation
