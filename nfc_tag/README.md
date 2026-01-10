# NFC Tag Reader Test

Read NFC tags and validate them against a list of authorized UIDs using a PN532 reader.

## Hardware

| Part | Description |
|------|-------------|
| Arduino | Main microcontroller |
| PN532 NFC Module | NFC/RFID reader (I2C mode) |

## Wiring (I2C Mode)

| PN532 Pin | Arduino Uno |
|-----------|-------------|
| VCC       | 5V          | 
| GND       | GND         | 
| SDA       | A4          | 
| SCL       | A5          | 
| IRQ       | D2          | 
| RSTO      | D3          | 

> **Note:** Set the PN532 DIP switches to I2C mode (typically: SEL0=ON, SEL1=OFF)

## Dependencies

Install via Arduino Library Manager:
- **Adafruit PN532** by Adafruit

## Usage

1. Upload `nfc_tag_reader.ino` to your Arduino
2. Open Serial Monitor at 115200 baud
3. Scan an NFC tag
4. The serial output will show:
   - Tag UID
   - VALID or INVALID status

## Adding Valid Tags

Edit the `VALID_TAGS` array in the code:

```cpp
const uint8_t VALID_TAGS[][8] = {
  {4, 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00},  // 4-byte UID
  {7, 0x04, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC},  // 7-byte UID
};
```

The first byte is the UID length, followed by the UID bytes.

To find a tag's UID, scan it first — the UID will be printed to serial.
