#include <SPI.h>
#include <LoRa.h>
#include <avr/wdt.h>

#define csPin 10
#define resetPin 9
#define irqPin 2

String outgoing;                 // outgoing message
byte msgCount = 0;               // count of outgoing messages
byte LOCAL_ADDRESS = 0xBB;       // address of this device -  0xBC (188)  or 0XBB (187)
byte DESTINATION_ADDRESS = 0xFF; // destination to send to

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

void sendMessage(String message)
{

  // Arduino Processors can only handle upto 7 Digits in 32bit RAM (4 x 8 bytes) Memory
  // float / double doesnt make any difference
  // sending more than 5 decimals requires you to send as string if you need more accuracy

  // Orginal Reference for receiver side on arduino
  // Float values consits of 4 Bytes always this is standard refer to this list
  // https://docs.python.org/3/library/struct.html#format-characters
  // https://www.programmingelectronics.com/dtostrf/
  // const uint8_t numFloatBytes {sizeof(float)};
  // char tempBuffer[numFloatBytes];
  // float floatValue;
  // for (uint8_t i = 0; i < numFloatBytes; i++) {
  //   tempBuffer[i] = LoRa.read();
  // }
  // memcpy((void *)&floatValue, tempBuffer, numFloatBytes);
  // Serial.println(floatValue,6);

  float lat = 19.1047461; // Accuracy is only 5 decimals send as text in message
  float lon = 72.8509614; // Accuracy is only 5 decimals send as text in message
  float temperature = -28.678;

  // char Latd[1] = {};
  // dtostrf(lat, 9, 7, Latd);
  // Serial.println(Latd);

  LoRa.beginPacket();              // start packet
  LoRa.write(255);                 // spaceer
  LoRa.write(DESTINATION_ADDRESS); // add destination address
  LoRa.write(LOCAL_ADDRESS);       // add sender address
  LoRa.write((uint8_t *)(&lat), sizeof(lat));
  LoRa.write((uint8_t *)(&lon), sizeof(lon));
  LoRa.write((uint8_t *)(&temperature), sizeof(temperature));
  LoRa.write(message.length()); // add payload length
  LoRa.print(message);          // add payload
  LoRa.endPacket();             // finish packet and send it
  Serial.println("[LORA] Sent Data" + message);
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
