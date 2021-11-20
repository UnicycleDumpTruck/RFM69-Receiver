#include <Arduino.h>
#include <Adafruit_SleepyDog.h>
#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

#define RF69_FREQ 915.0

#define MY_ADDRESS 1

// Feather M0 w/Radio
#define RFM69_CS 8
#define RFM69_IRQ 3
#define RFM69_RST 4

// Feather M0 w/Radio Featherwing
//#define RFM69_CS      10   // "B"
//#define RFM69_RST     11   // "A"
//#define RFM69_IRQ     6    // "D"
//#define RFM69_IRQN    digitalPinToInterrupt(RFM69_IRQ )

//#define RELAY 		  12

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_IRQ);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);

int16_t packetnum = 0; // packet counter, we increment per xmission

uint32_t localCounter = 1;

struct EvenDataPacket
{
  uint32_t counter;
  float batteryVoltage;
  uint8_t cubeID;
  uint8_t side;
} eventData;

// Dont put this on the stack:
uint8_t eventBuffer[sizeof(eventData)];
uint8_t from;
uint8_t len = sizeof(eventData);

// buffers for receiving and sending data
char packetBuffer[255];                         //buffer to hold incoming packet,
const char AckBuffer[] = "acknowledged";        // a string to send back
const char DenyBuffer[] = "Error, no release."; // a string to send back
const char pigDrop[] = "pigDrop";               // command string to compare rx to
const char cEquals[] = "c=";
const char tEquals[] = " t=";
const char sEquals[] = " s=";
const char gEquals[] = " g=";
const char bEquals[] = " b=";
const char fEquals[] = " f=";

char ReplyBuffer[] = "acknowledged"; // a string to send back

void selectRadio()
{
  digitalWrite(RFM69_CS, LOW);
  delay(100);
}

void deselectRadio()
{
  digitalWrite(RFM69_CS, HIGH);
  delay(100);
}
//RF communication
void sendEventData()
{
  rf69.send((uint8_t *)&eventData, sizeof(eventData));
  rf69.waitPacketSent();
}

void sendDispenseEvent()
{
  //eventData.side = 1;
  eventData.counter = localCounter;
  localCounter++;
  eventData.cubeID = 0;
  eventData.side = 61;
  Serial.print(F("About to send transmission number: "));
  Serial.println(eventData.counter);
  sendEventData();
  //eventData.side = 0;
}

void setupRadio()
{
  //--Radio Setup--//
  selectRadio();
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(50);
  digitalWrite(RFM69_RST, LOW);
  delay(50);

  if (!rf69_manager.init())
  {
    Serial.println(F("RFM69 radio init failed"));
    while (1)
      ;
  }
  Serial.println(F("RFM69 radio init OK!"));
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ))
  {
    Serial.println(F("setFrequency failed"));
  }

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true); // range from 14-20 for power, 2nd arg must be true for 69HCW

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                   0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);

  eventData.cubeID = 0;
  eventData.side = 0;
  eventData.batteryVoltage = 0;
  eventData.counter = 0;
}

void receiveFromCube()
{
  selectRadio();
  if (rf69.recv((uint8_t *)&eventData, &len) && len == sizeof(eventData))
  {
    char buf[16];

    Serial.print(F("Received: "));
    Serial.print(F("c="));
    Serial.print(itoa(eventData.cubeID, buf, 10));
    Serial.print(F(" t="));
    Serial.print(itoa(eventData.counter, buf, 10));
    Serial.print(F(" s="));
    Serial.print(itoa(eventData.side, buf, 10));
    Serial.print(F(" g="));
    Serial.print(rf69.lastRssi(), DEC);
    Serial.print(F(" b="));
    Serial.print(eventData.batteryVoltage);
    Serial.println(F(" "));
  }
  deselectRadio();
}

void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ;

  Serial.println(F("Feather RFM69 RX Test!"));
  Serial.println();

  setupRadio();

  int countdownMS = Watchdog.enable(4000);
  Serial.print(F("Enabled the watchdog with max countdown of "));
  Serial.print(countdownMS, DEC);
  Serial.println(F(" milliseconds!"));
  Serial.println();

} // END SETUP //

void loop()
{
  receiveFromCube();

  Watchdog.reset();
} // END LOOP //
