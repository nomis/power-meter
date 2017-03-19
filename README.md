# Arduino Power Meter Modbus Client
Acts as a Modbus master device to collect data from a slave Power Meter device.
Readings are taken every second and then output in YAML format.

## Hardware Interface
MAX485 with the following pin connections:

| MAX485 | Arduino Micro | Espressif ESP8266         |
| ------ | ------------- | ------------------------- |
| VCC    | +5V           | N/A¹                      |
| GND    | ⏚ (GND)       | GND                       |
| DI     | 0 (TX)        | 1 (U0TXD)²                |
| RO     | 1 (RX)        | 3 (U0RXD)²                |
| DE     | 4             | GPIO4²                    |
| R̅E̅     | 5             | GPIO5²                    |

¹ a separate +5V supply is required

² via a 3.3V to 5V logic level converter

### Arduino Micro
Output is on the USB serial console.

### Espressif ESP8266
Output is on the UART1 TX GPIO2 pin at 115200 8N1.

To configure the WiFi SSID and passphrase, connect GPIO14 to GND and the device will enter AP mode using the SSID `🔌 ########`.

# Supported Power Meters
* Rayleigh Instruments RI-D19-80-C: 230V 5/80A LCD Single Phase Energy modbus – 80A Direct With RS485 Output

# Sample Output
```yaml
meter: {model: "RI-D19-80-C",serialNumber: "############",reading: {voltage: 2471e-1,current: 3e-1,frequency: 500e-1,activePower: 81,reactivePower: 28,apparentPower: 90,powerFactor: 1000e-1,temperature: 31,activeEnergy: 88e-2}}
```
