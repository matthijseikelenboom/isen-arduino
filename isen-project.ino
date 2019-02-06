//  SerialIn_SerialOut_HM-10_01
//
//  Uses hardware serial to talk to the host computer and AltSoftSerial for communication with the bluetooth module
//
//  What ever is entered in the serial monitor is sent to the connected device
//  Anything received from the connected device is copied to the serial monitor
//  Does not send line endings to the HM-10
//
//  Pins
//  BT VCC to Arduino 5V out. 
//  BT GND to GND
//  Arduino D8 (SS RX) - BT TX no need voltage divider 
//  Arduino D9 (SS TX) - BT RX through a voltage divider (5v to 3.3v)
// 
#include <AltSoftSerial.h>
#include <TheThingsNetwork.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

const char *appEui = "70B3D57ED0015281";
const char *appKey = "EDDDFDC57C20C3EE7E28D0A6274A530C";

#define loraSerial Serial1

// Replace REPLACE_ME with TTN_FP_EU868 or TTN_FP_US915
#define freqPlan TTN_FP_EU868
    
TheThingsNetwork ttn(loraSerial, Serial, freqPlan);

AltSoftSerial BTserial; 
// https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(8, 11);

String result = "";
String previousResult = "";

void setup()  {
  Serial.begin(9600);
  loraSerial.begin(57600);
  while (!Serial && millis() < 10000);
  Serial.print("Sketch:   ");   Serial.println(__FILE__);
  Serial.print("Uploaded: ");   Serial.println(__DATE__);
  Serial.println(" ");

  Serial.println("TTN -- STATUS");
  ttn.showStatus();

  Serial.println("TTN -- JOIN");
  ttn.join(appEui, appKey);

  BTserial.begin(9600);
  Serial.println("BTserial started at 9600");
  delay(1000);
  BTserial.print("AT+IMME1" );
  delay(1000);
  BTserial.print("AT+ROLE1" );
  delay(1000);
  BTserial.print("AT+DISC?");
  delay(1000);
  ss.begin(4800);
}

void loop() {
  if (BTserial.available()) {
    result = BTserial.readStringUntil('\r\n');
    result.trim();
    Serial.println(result);
    if (result == "OK+DISCE") {
      BTserial.print("AT+DISC?");
    } else if (result == "OK+NAME:RDL51822") {
      char toSend[13];
      if (previousResult.length() == 40) {
        previousResult.substring(16, 28).toCharArray(toSend, 13);
      } else if (previousResult.length() == 32) {
        previousResult.substring(8, 20).toCharArray(toSend, 13);
      }
      byte payload[7] = {0};
      payload[0] = 1;
      hexCharacterStringToBytes(payload, toSend);
      ttn.sendBytes(payload, sizeof(payload));
    }
    previousResult = result;
  }
  if (ss.available() > 0) {
    if (gps.encode(ss.read())) {
      uint8_t coords[7]; // 6*8 bits = 48
      int32_t lat = gps.location.lat() * 10000;
      int32_t lng = gps.location.lng() * 10000;
  
      coords[0] = 2;
      
      // Pad 2 int32_t to 6 8uint_t, skipping the last byte (x >> 24)
      coords[1] = lat;
      coords[2] = lat >> 8;
      coords[3] = lat >> 16;
      
      coords[4] = lng;
      coords[5] = lng >> 8;
      coords[6] = lng >> 16;
      ttn.sendBytes(coords, sizeof(coords));
    }
  }
}

void hexCharacterStringToBytes(byte *byteArray, const char *hexString) {
  bool oddLength = strlen(hexString) & 1;

  byte currentByte = 0;
  byte byteIndex = 1;

  for (byte charIndex = 0; charIndex < strlen(hexString); charIndex++) {
    bool oddCharIndex = charIndex & 1;

    if (oddLength) {
      // If the length is odd
      if (oddCharIndex) {
        // odd characters go in high nibble
        currentByte = nibble(hexString[charIndex]) << 4;
      } else {
        // Even characters go into low nibble
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    } else {
      // If the length is even
      if (!oddCharIndex) {
        // Odd characters go into the high nibble
        currentByte = nibble(hexString[charIndex]) << 4;
      } else {
        // Odd characters go into low nibble
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
  }
}

byte nibble(char c){
  if (c >= '0' && c <= '9') {
    return c - '0';
  }

  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }

  return 0;  // Not a valid hexadecimal character
}
