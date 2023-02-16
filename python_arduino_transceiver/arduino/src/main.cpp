#include <SPI.h>
#include <LoRa.h>
#include <avr/wdt.h>
#include <Base64.h> // https://github.com/adamvr/arduino-base64
#include <AESLib.h> // https://github.com/DavyLandman/AESLib

#define csPin 10
#define resetPin 9
#define irqPin 2

#define ENCRYPT_BIT_SIZE 16

byte LOCAL_ADDRESS = 0xBB;       // address of this device -  0xBC (188)  or 0XBB (187)
byte DESTINATION_ADDRESS = 0xFF; // destination to send to

uint8_t key[] = "1234567890123456";

// // Initialise and configure AES Encryption settings
// AESLib aesLib;
// char cleartext[256];
// char ciphertext[512];

// // AES Encryption key
// byte aes_key[] = {0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};
// byte aes_iv[N_BLOCK] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

long lastSendTime = 0;
int interval = 2000;
const unsigned long CHECK_TIME = 5000;
unsigned long lastMsgTime = 0;

boolean runEvery(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

void initLora()
{
  LoRa.setPins(csPin, resetPin, irqPin);
  if (!LoRa.begin(434E6))
  { // initialize ratio at 434 MHz
    Serial.println("[LORA] Init failed. Check your connections.");
    while (1)
      ; // if failed, do nothing
  }
  // LoRa.setSyncWord(0xF3);
  // LoRa.dumpRegisters(Serial);
  // LoRa.enableInvertIQ();
  // LoRa.setSPI(SPI);
  // LoRa.setSPIFrequency(4e6);
  // LoRa.setSpreadingFactor(10);
  // LoRa.setSignalBandwidth(42.5E3);
  // LoRa.crc();
  // LoRa.setSpreadingFactor(12); //7 For Low Power
  // LoRa.setSignalBandwidth(250000); //500000 For Low Power
  // LoRa.setPreambleLength(8);
  // LoRa.setCodingRate4(8); //5 For Low Power
  // LoRa.enableCrc();
  Serial.println("[LORA] Initialized");
}

void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ;
  // wdt_enable(WDTO_2S);
  // wdt_enable(WDTO_1S);
  initLora();
}

void checkTimeout()
{
  unsigned long currentTime = millis();

  if (currentTime - lastMsgTime >= CHECK_TIME)
  {
    lastMsgTime = currentTime;
    Serial.println("No data received - SPI RESTART");

    LoRa.end();
    LoRa.setPins(csPin, resetPin, irqPin);

    if (!LoRa.begin(434E6))
    {
      Serial.println("Resting LoRa failed!");
    }
    else
    {
      Serial.println("Resting LoRa ok");
    }
  }
}

String encrypt(char *msg)
{
  char input[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  memcpy(input, msg, strlen(msg));
  aes128_enc_single(key, input);
  int inputLen = sizeof(input);
  int encodedLen = base64_enc_len(inputLen);
  char encoded[encodedLen];
  base64_encode((char *)encoded, (char *)input, inputLen);
  return (char *)encoded;
}

int bitLen(int inputLen)
{
  float float_len = inputLen / float(ENCRYPT_BIT_SIZE);
  int int_len = inputLen / ENCRYPT_BIT_SIZE;
  int result = 0;
  if (float_len > float(int_len))
  {
    result = int_len + 1;
  }
  else
  {
    result = int_len;
  }
  return result;
}

void sendMessage(String message)
{

  // Arduino Processors can only handle upto 7 Digits in 32bit RAM (4 x 8 bytes) Memory
  // float / double doesnt make any difference
  // sending more than 5 decimals requires you to send as string if you need more accuracy

  // Orginal Reference for receiver side on arduino
  // Float values consits of 4 Bytes always this is standard refer to this list
  // https://docs.python.org/3/library/struct.html#format-characters
  // https://www.programmingelectronics.com/dtostrf/
  // https://github.com/vtomanov/Gps64
  // const uint8_t numFloatBytes {sizeof(float)};
  // char tempBuffer[numFloatBytes];
  // float floatValue;
  // for (uint8_t i = 0; i < numFloatBytes; i++) {
  //   tempBuffer[i] = LoRa.read();
  // }
  // memcpy((void *)&floatValue, tempBuffer, numFloatBytes);
  // Serial.println(floatValue,6);

  // int value = (voltage * 100.0) + 0.5;
  // // Extract each digit with the 'modulo' operator (%)

  // char hundredsDigit = '0' + ((value / 100) % 10);
  // char tensDigit =  '0' + ((value / 10) % 10);
  // char onesDigit =  '0' + (value % 10);

  // https://www.thethingsnetwork.org/forum/t/how-to-send-float-value-and-convert-it-into-bytes/9646/9
  //  mydata[0] = LatitudeBinary >> 16;
  //  mydata[1] = LatitudeBinary >> 8;
  //  mydata[2] = LatitudeBinary;

  // char Latd[1] = {};
  // dtostrf(lat, 9, 7, Latd);
  // Serial.println(Latd);

  char input[] = "PM25=35|PM10=80|TMP=24.56|HUM=80|PRE=1008.90|CO2=2478|AQI=180|LGT=25000";
  int inputLen = sizeof(input);
  int inputBitLen = bitLen(inputLen);

  float lat = 19.1047461; // Accuracy is only 5 decimals send as text in message if you want more decimal accuracy
  float lon = 72.8509614; // Accuracy is only 5 decimals send as text in message if you want more decimal accuracy
  float temperature = -28.678;

  LoRa.beginPacket();              // start packet
  LoRa.write(255);                 // spaceer
  LoRa.write(DESTINATION_ADDRESS); // add destination address
  LoRa.write(LOCAL_ADDRESS);       // add sender address
  LoRa.write((uint8_t *)(&lat), sizeof(lat));
  LoRa.write((uint8_t *)(&lon), sizeof(lon));
  LoRa.write((uint8_t *)(&temperature), sizeof(temperature));

  LoRa.write(inputBitLen);
  char buffer[ENCRYPT_BIT_SIZE];
  memset(buffer, 0, ENCRYPT_BIT_SIZE);
  uint8_t a = 0;

  // Protocol : COUNT LOOP | MESSAGELENGTH | MESSAGE | MESSAGELENGTH | MESSAGE | ...

  for (uint8_t i = 0; i < inputLen; i++)
  {
    if (i > 0 && (i % 16 == 0))
    {
      Serial.println(buffer);
      String encoded = encrypt(buffer);
      LoRa.write(encoded.length());
      LoRa.print(encoded);

      memset(buffer, 0, ENCRYPT_BIT_SIZE);
      a = 0;
    }

    if ((inputLen - i) < 16 && (inputLen - i) == 1)
    {
      Serial.println(buffer);
      String encoded = encrypt(buffer);
      LoRa.write(encoded.length());
      LoRa.print(encoded);

      memset(buffer, 0, ENCRYPT_BIT_SIZE);
      a = 0;
    }
    buffer[a++] = input[i];
  }
  LoRa.endPacket(); // finish packet and send it
}

void onReceive(int packetSize)
{
  if (packetSize == 0)
    return;

  if (packetSize)
  {
    lastMsgTime = millis();
  }

  Serial.println("\n\n");
  LoRa.read(); // Requires first byte to be a string and so we discard it

  uint8_t receiver_address = LoRa.read();
  uint8_t sender_address = LoRa.read();

  uint8_t data_1 = LoRa.read();
  uint8_t data_2 = LoRa.read();
  uint8_t data_3 = LoRa.read();

  if (receiver_address != LOCAL_ADDRESS && receiver_address != sender_address)
  {
    Serial.println("[LORA] This message is not for me.");
    return;
  }

  if (sender_address == DESTINATION_ADDRESS)
  {

    byte messageLen = LoRa.read();
    String message = "";
    while (LoRa.available())
    {
      message += (char)LoRa.read();
    }

    if (messageLen != message.length())
    {
      Serial.println("[LORA] Message length does not match length");
      return;
    }

    Serial.println("\n\n");
    Serial.println("Sent to: 0x" + String(receiver_address, HEX));
    Serial.println("Received from: 0x" + String(sender_address, HEX));
    Serial.println("Servo Angle: " + String(data_1));
    Serial.println("Motor A Direction: " + String(data_2));
    Serial.println("PWM: " + String(data_3));
    Serial.println("RSSI: " + String(LoRa.packetRssi()));
    Serial.println("SNR: " + String(LoRa.packetSnr()));
    Serial.println("Message length: " + String(messageLen));
    Serial.println("Message: " + String(message));
  }
  sendMessage("HeLoRa World!");
}

void loop()
{
  // LoRa.idle();
  // LoRa.sleep();
  // wdt_reset();
  // if (runEvery(3000))
  // {
  //   Serial.println("Doing every: 3 seconds");
  // }
  onReceive(LoRa.parsePacket());
  // checkTimeout();
}
