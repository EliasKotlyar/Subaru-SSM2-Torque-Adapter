// See : https://github.com/matprophet/subduino/blob/master/subduino/subduino.ino

#include <SoftwareSerial.h>
#include "SSMCAN.h"
#include "ELM327_Emulator.h"
#if 0
//#define DPRINT(...)    Serial.print(__VA_ARGS__)
//#define DPRINTLN(...)  Serial.println(__VA_ARGS__)
#else
//#define DPRINT(...)
//#define DPRINTLN(...)
#endif

#define DPRINT(...)
#define DPRINTLN(...)
#define SERIAL_DIAG_SPEED     9600
#define SSM_REQUEST_INTERVAL  10
SoftwareSerial gSerialPort = SoftwareSerial(PIN_PD2, PIN_PB3); // Rx, Tx
bool gIsClearToSend = true;
unsigned long gTimeLast = 0;
unsigned long gTimeCurrent = 0;
enum SDRequestState : byte {
  SDRequestStateInit,
  SDRequestStateSet1,
};


ELM327Emu emu;
byte gSDRequestAddressSet1Size = 9;
unsigned long gSDRequestAddressSet1SSMAddresses[9] = {
  0x0F, // Engine Speed 2 (RPM) low byte
  0x0E, // Engine Speed 1 (RPM) high byte
  0x0D, // Manifold Absolute Pressure
  0x08, // Coolant Temperature
  0x10, // Vehicle Speed
  0x12, // Intake Air Temperature
  0x15, // Throttle Opening Angle
  0x1C, // Battery Voltage
  0x46, // Air/Fuel Sensor #1
};
SDRequestState gSDRequestState = SDRequestStateInit;


void setupObd() {

  emu.setup();

}
void setup()
{

  pinMode(PIN_PA6, OUTPUT);
  DPRINTLN("Connecting to Serial...");
  //Serial.begin(SERIAL_DIAG_SPEED); //for diagnostics
  //while (!Serial) {
  //  delay(50);
  //}

  DPRINTLN("Initializing SSM serial port...");
  gSerialPort.begin(SSM_BUS_SPEED); //SSM uses 4800 8N1 baud rate
  do {
    delay(50);
  } while (!gSerialPort);
  DPRINTLN("SSM Serial Line Established.");



  gTimeLast = millis();
  setupObd();

}
void loop()
{
  emu.loop();
  return;

  gTimeCurrent = millis();
  if ((gTimeCurrent - gTimeLast) > SSM_REQUEST_INTERVAL) {
    gSerialPort.flush();

    sendSSMCommandForState(gSDRequestState); // re-send the current request
    gTimeLast = gTimeCurrent;
  }

  if (readSSMResponse()) {
    gTimeLast = gTimeCurrent;
    gIsClearToSend = true;
  }




  if (gIsClearToSend) {
    sendSSMCommandForState(gSDRequestState);
    gIsClearToSend = false;
  }

}



void sendSSMCommandForState(SDRequestState inState) {
  switch (inState) {
    case SDRequestStateInit:
      DPRINTLN("Requesting SSM init...");
      sendSSMPacket(packetForSSMInit(), gSerialPort);
      break;


    case SDRequestStateSet1:
      DPRINTLN("Requesting SSM set 1...");
      sendSSMPacket(packetForAddressRead(gSDRequestAddressSet1SSMAddresses, gSDRequestAddressSet1Size), gSerialPort);
      break;

  }
}


bool readSSMResponse() {
  if (!gSerialPort.available()) {
    return false;
  }

  SSMPacket *packet = readPacketFromSSMBus(gSerialPort);
  if (!packet) {
    DPRINT("ERR: Could not create SSM response packet for request state: ");
    DPRINTLN(gSDRequestState);
    return false;
  }

  switch (gSDRequestState) {
    case SDRequestStateInit:

      gSDRequestState = SDRequestStateSet1;

      break;


    case SDRequestStateSet1:
      //logPacket(packet);

      emu.setPedalValue(packet->data[7]);
      //Serial.println(packet->data[7]);

      break;

  }

  freePacket(packet);
  return true;
}
