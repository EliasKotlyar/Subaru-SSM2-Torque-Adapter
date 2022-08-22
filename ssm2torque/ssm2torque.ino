 // See : https://github.com/matprophet/subduino/blob/master/subduino/subduino.ino
void setup()
{
  DPRINTLN("Connecting to Serial...");
  Serial.begin(SERIAL_DIAG_SPEED); //for diagnostics
  while (!Serial) {
    delay(50);
  }

  DPRINTLN("Initializing SSM serial port...");
  gSerialPort.begin(SSM_BUS_SPEED); //SSM uses 4800 8N1 baud rate
  do{
    delay(50);
  } while (!gSerialPort);
  DPRINTLN("SSM Serial Line Established.");

  DPRINTLN("Initializing CAN bus...");
  CAN.begin(CAN_BPS_500K);
  DPRINTLN("CAN Bus Initialized.");

  gTimeLast = millis();
}
void loop()
{
  gTimeCurrent = millis();
  if ((gTimeCurrent - gTimeLast) > SSM_REQUEST_INTERVAL) {
    gSerialPort.flush();
    DPRINTLN("-----flush-----");
    sendSSMCommandForState(gSDRequestState); // re-send the current request
    gTimeLast = gTimeCurrent;
  }

  if (readSSMResponse()) {
    gTimeLast = gTimeCurrent;
    gIsClearToSend = true;
  }

  readAnalogPinsIntoSDMemory();
  sendCANPacketsFromSDMemory();

  if (gIsClearToSend) {
    sendSSMCommandForState(gSDRequestState);
    gIsClearToSend = false;
  }
}
