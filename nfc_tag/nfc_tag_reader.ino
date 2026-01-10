/*
 * NFC Tag Reader Test
 *
 * Reads NFC tags using PN532 and validates against a list of authorized UIDs.
 *
 * Hardware: Arduino + PN532 NFC Reader (I2C mode)
 * Library: Adafruit PN532
 */

#include <Wire.h>
#include <Adafruit_PN532.h>

// I2C connection (SDA/SCL pins vary by board)
#define PN532_IRQ   2
#define PN532_RESET 3

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// Authorized tag UIDs (add your valid tags here)
// Format: {length, byte1, byte2, byte3, byte4, byte5, byte6, byte7}
const uint8_t VALID_TAGS[][8] = {
  {4, 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00},  // Example 4-byte UID
  {7, 0x04, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC},  // Example 7-byte UID
};
const uint8_t NUM_VALID_TAGS = sizeof(VALID_TAGS) / sizeof(VALID_TAGS[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("=== NFC Tag Reader Test ===");
  Serial.println();

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("ERROR: PN532 not found. Check wiring!");
    while (1) delay(10);
  }

  Serial.print("Found PN53x, firmware v");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print(".");
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Configure to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for NFC tag...");
  Serial.println();
}

void loop() {
  uint8_t uid[7];       // Buffer for UID (max 7 bytes)
  uint8_t uidLength;    // Length of the UID

  // Wait for tag (blocking with 0 timeout, or use a timeout in ms)
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.println("--- Tag Detected ---");

    // Print UID
    Serial.print("UID: ");
    printHex(uid, uidLength);
    Serial.print(" (");
    Serial.print(uidLength);
    Serial.println(" bytes)");

    // Validate tag
    if (isValidTag(uid, uidLength)) {
      Serial.println("Status: VALID TAG");
      onValidTag(uid, uidLength);
    } else {
      Serial.println("Status: INVALID TAG");
      onInvalidTag(uid, uidLength);
    }

    Serial.println();
    delay(1000);  // Debounce
  }
}

bool isValidTag(uint8_t *uid, uint8_t uidLength) {
  for (uint8_t i = 0; i < NUM_VALID_TAGS; i++) {
    uint8_t tagLen = VALID_TAGS[i][0];

    if (tagLen != uidLength) continue;

    bool match = true;
    for (uint8_t j = 0; j < tagLen; j++) {
      if (uid[j] != VALID_TAGS[i][j + 1]) {
        match = false;
        break;
      }
    }

    if (match) return true;
  }
  return false;
}

void onValidTag(uint8_t *uid, uint8_t uidLength) {
  // Add your action for valid tags here
  // e.g., unlock door, turn on LED, etc.
  Serial.println(">> Access granted");
}

void onInvalidTag(uint8_t *uid, uint8_t uidLength) {
  // Add your action for invalid tags here
  // e.g., sound buzzer, flash red LED, etc.
  Serial.println(">> Access denied");
}

void printHex(uint8_t *data, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    if (data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
    if (i < length - 1) Serial.print(":");
  }
}
