#include <Z21.h>

extern WiFiUDP Udp;

// ---------- Observer
int Z21::numObservers = 0;
Z21Observer* Z21::observers[MaxObservers];

const bool XOR = true;
const bool NOXOR = false;

static char aux[100];

long Z21::lastReceived = -Z21_HEARTBEAT;

String Z21::ipAddress;

#define yetUnknownNumeric -1

String Z21::hwVersion = yetUnknown;

BoolState Z21::trackPowerState = BoolState::Undefined;
BoolState Z21::progState = BoolState::Undefined;
BoolState Z21::emergencyStopState = BoolState::Undefined;
BoolState Z21::shortCircuitState = BoolState::Undefined;

String Z21::xbusVersion = yetUnknown;
String Z21::csID = yetUnknown;
String Z21::serialNbr = yetUnknown;
String Z21::fwVersion = yetUnknown;
String Z21::currMain = yetUnknown;
String Z21::currProg = yetUnknown;
String Z21::temp = yetUnknown;
bool Z21::lowVoltage = yetUnknownNumeric;
bool Z21::highTemp = yetUnknownNumeric;


boolean Z21::BCF_BASIC;
boolean Z21::BCF_RBUSFB;
boolean Z21::BCF_RC;
boolean Z21::BCF_SYS;
boolean Z21::BCF_ALLLOCO;
boolean Z21::BCF_ALLRC;
boolean Z21::BCF_CANFB;
boolean Z21::BCF_LNMSG;
boolean Z21::BCF_LNLOCO;
boolean Z21::BCF_LNSwitch;
boolean Z21::BCF_LNFB;
boolean Z21::flagsValid = false;

// Zumindest mit der DR5000 kommt es zu einem Problem:
// Obwohl eine LOCO_INFO übermittelt wurde, die vom Client dieser Lib ausgelöst wurde
// (LAN_X_GET_LOCO_INFO), wird die Lok in der Antwort (LAN_X_LOCO_INFO) als fremdübernommen
// gekennzeichnet. Außerdem wird nach einer gewissen Zeit (oder Zahl von Meldungen?) die letzte (!)
// LOCO_INFO als fremdgesteuert gekennzeichnet. Mit einem Flag wird das gekennzeichnet, um dies
// erkennen zu können

int Z21::lastControlledAddress = 0;

int Z21::addrOffs = 0;

long lastMessageSentReceived = 0;
long diffLastSentReceived;

void sendCommand(byte[], int, bool);
void insertChecksum(byte[], int);

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

// // ----------------------------------------------------------------------------------------------------
// //

// void Z21::setXBustrace(boolean on) {
//   traceOn = on;
// }

// ----------------------------------------------------------------------------------------------------
//

void Z21::setIPAddress(String ipAddr) {
  ipAddress = ipAddr;
}

String Z21::getIPAddress() {
  return ipAddress;
}

// // ====================================================================================================
// // SEND commands to Z21
// // ====================================================================================================

// ----------------------------------------------------------------------
//
//

uint16_t Z21::heartbeat() {
  trace(toZ21, diffLastSentReceived, "Lebenszeichen", "");
  LAN_X_GET_STATUS();
  delay(50);
  LAN_SYSTEMSTATE_GETDATA();

  if (xbusVersion == yetUnknown) {
    delay(50);
    LAN_X_GET_VERSION();
  }
  if (fwVersion == yetUnknown) {
    delay(50);
    LAN_X_GET_FIRMWARE_VERSION();
  }
  if (hwVersion == yetUnknown) {
    delay(50);
    LAN_GET_HWINFO();
  }
  if (serialNbr == yetUnknown) {
    delay(50);
    LAN_GET_SERIAL_NUMBER();
  }
  if (!flagsValid) {
    delay(50);
    LAN_GET_BROADCASTFLAGS();
  }

  // Nach Z21_HEARTBEAT erneut aufrufen:
  // "Es wird erwartet, dass ein Client 1x pro Minute kommuniziert, da er sonst
  // aus der Liste der aktiven Geräte gestrichen wird"
  return Z21_HEARTBEAT; // funktioniert nur, wenn z.B. durch eine Timerlibrary wie ezTime gerufen, ansonsten muss
  // der zyklische Aufruf selbst programmiert werden!
}

// ----------------------------------------------------------------------
// Orientiert an dem, was die WLAN-Maus anfangs sendet

void Z21::init() {
  LAN_SET_BROADCASTFLAGS(3, 0, 1, 0); // 3: on/off/prog/loco/access, 1: loco info auch für alle geänderte Loks

}

// ----------------------------------------------------------------------
//

void Z21::LAN_SYSTEMSTATE_GETDATA() {
  uint8_t bytes[] = { 0x04, 0x00, 0x85, 0x00 };
  sendCommand(bytes, 4, NOXOR);
  trace(toZ21, diffLastSentReceived, "LAN_SYSTEMSTATE_GETDATA", "");
}

// ----------------------------------------------------------------------
//

void Z21::LAN_GET_SERIAL_NUMBER() {
  uint8_t bytes[] = { 0x04, 0x00, 0x10, 0x00 };
  sendCommand(bytes, 4, NOXOR);
  trace(toZ21, diffLastSentReceived, "LAN_GET_SERIAL_NUMBER", "");
}

// ----------------------------------------------------------------------
//

void Z21::LAN_GET_HWINFO() {
  uint8_t bytes[] = { 0x04, 0x00, 0x1a, 0x00 };
  sendCommand(bytes, 4, NOXOR);
  trace(toZ21, diffLastSentReceived, "LAN_GET_HWINFO", "");
}

// ----------------------------------------------------------------------
//

void Z21::LAN_X_GET_FIRMWARE_VERSION() {
  uint8_t bytes[] = { 0x07, 0x00, 0x40, 0x00, 0xf1, 0x0a, 0x00 };
  sendCommand(bytes, bytes[0], XOR);
  trace(toZ21, diffLastSentReceived, "LAN_X_GET_FIRMWARE_VERSION", "");
}

// ----------------------------------------------------------------------
//

void Z21::LAN_X_GET_VERSION() {
  uint8_t bytes[] = { 0x07, 0x00, 0x40, 0x00, 0x21, 0x21, 0x00 };
  sendCommand(bytes, 7, NOXOR);
  trace(toZ21, diffLastSentReceived, "LAN_X_GET_VERSION", "");
}

// ----------------------------------------------------------------------
//

void Z21::LAN_X_GET_STATUS() {
  uint8_t bytes[] = { 0x07, 0x00, 0x40, 0x00, 0x21, 0x24, 0x00 };
  sendCommand(bytes, bytes[0], NOXOR);
  trace(toZ21, diffLastSentReceived, "LAN_X_GET_STATUS", "");
}

// ----------------------------------------------------------------------
//

void Z21::LAN_GET_BROADCASTFLAGS() {
  uint8_t bytes[] = { 0x04, 0x00, 0x51, 0x00 };
  sendCommand(bytes, bytes[0], NOXOR);
  trace(toZ21, diffLastSentReceived, "LAN_GET_BROADCASTFLAGS", "");
}

// ----------------------------------------------------------------------
//

void Z21::LAN_SET_BROADCASTFLAGS(char f1, char f2, char f3, char f4) {
  uint8_t bytes[] = { 0x08, 0x00, 0x50, 0x00, f1, f2, f3, f4 };
  sendCommand(bytes, bytes[0], NOXOR);
  sprintf(aux, "%0x %0x %0x %0x", f1, f2, f3, f4);
  trace(toZ21, diffLastSentReceived, "LAN_SET_BROADCASTFLAGS", aux);
}


// ----------------------------------------------------------------------------------------------------
//

void Z21::LAN_X_SET_TRACK_POWER(boolean on) {
  uint8_t bytes[] = { 0x07, 0x00, 0x40, 0x00, 0x21, 0x80, 0xa1 };
  if (on) {
    bytes[5] = 0x81; bytes[6] = 0xa0;
  }
  sendCommand(bytes, 7, NOXOR);
  trace(toZ21, diffLastSentReceived, String("LAN_X_SET_TRACK_POWER_") + (on ? "ON" : "OFF"), "");
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::LAN_X_GET_TURNOUT_INFO(int addr) {
  byte bytes[] = { 0x08, 0x00, 0x40, 0x00, 0x43, (byte)((addr - 1) / 256), (byte)((addr - 1) % 256), 0};
  sendCommand(bytes, bytes[0], XOR);
  sprintf(aux, "Adresse=%d", addr);
  trace(toZ21, diffLastSentReceived, "LAN_X_GET_TURNOUT_INFO", String(aux));
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::LAN_X_SET_TURNOUT(int addr, bool plus) {
  byte data = (byte)0b10101000; // 10Q0A00P Q=Queue, A=Aktivieren oder deaktivieren, P=Ausgang 0 oder 1 = Schalten
  data = (byte)(data + (plus ? 1 : 0));
  byte bytes[] = { 0x09, 0x00, 0x40, 0x00, 0x53, (byte)((addr - 1) / 256), (byte)((addr - 1) % 256), data, 0};
  sendCommand(bytes, 9, XOR);
  sprintf(aux, "Adresse=%d Lage=%d", addr, plus ? 1 : 0);
  trace(toZ21, diffLastSentReceived, "LAN_X_SET_TURNOUT", String(aux));
}

// ----------------------------------------------------------------------------------------------------
// speed ist hier User Speed 0..126 ohne Nothalt 1

void Z21::LAN_X_SET_LOCO_DRIVE(int addr, int numFst, Direction dir, int speed) {
  locoDrive(addr, numFst, dir, userToDccSpeed(speed));
  sprintf(aux, "Adresse=%d Richtung=%s Fahrstufe=%d/%d", addr, dir == Direction::Forward ? "V" : "R", speed, numFst);
  trace(toZ21, diffLastSentReceived, "LAN_X_SET_LOCO_DRIVE", String(aux));
}

void Z21::locoStop(int addr, int numFst, Direction dir) {
  locoDrive(addr, numFst, dir, 1);
  sprintf(aux, "Adresse=%d Richtung=%s Nothalt", addr, dir == Direction::Forward ? "V" : "R");
  trace(toZ21, diffLastSentReceived, "LAN_X_SET_LOCO_DRIVE", String(aux));
}

// ----------------------------------------------------------------------------------------------------
// Speed ist hier DCC-Speed 0..127 mit Nothalt 1 bei 128  Fst
//                          0..27 bei 28 Fst
//                          0..13 bei 14 Fst
// Auch erst hier Umsetzung Gerd-Offset

void Z21::locoDrive(int addr, int numFst, Direction dir, int speed) {
  lastControlledAddress = addr;
  byte fstCoding;
  byte speedCoding;

  switch(numFst) {
    case 14:
      fstCoding= 0x10;
      speedCoding = (byte)((dir == Direction::Forward ? 128 : 0) + speed); // wie bei 128, nur dass der Wertevorrat von speed kleiner ist
      break;
    case 28: { // Block nötig, sonst error: jump to case label, wegen "int halfStep"
        fstCoding= 0x12;
        // Weiterhin werden nur die letzten 4 Bit verwendet (Bit0 bis Bit3), so dass das Bit4 missbraucht wird (bei geraden Stufen)
          int halfStep = speed/2 + 1; // da /2 abrundet, erhalten 2,3 und 4,5 und 6,7 etc. den gleichen Wert. Damit  nicht
                                      // gleiche Werte rauskommen, wird abwechselnd in Bit4 geschrieben (0b1000)
          speedCoding = (byte)((dir == Direction::Forward ? 128 : 0) + halfStep);
        if (speed%2 == 0) {
          speedCoding |= 0b1000;
        } 
      }
      break;
    case 128:
    default:
      fstCoding= 0x13;
      speedCoding = (byte)((dir == Direction::Forward ? 128 : 0) + speed);
  }
  byte bytes[] = { 0x0a, 0x00, 0x40, 0x00, (byte)0xe4, fstCoding, (byte)(((addr + addrOffs) / 256) | (addr > 128 ? 0xC0 : 0)), (byte)((addr + addrOffs) % 256), speedCoding, 0};
  sendCommand(bytes, 10, XOR);
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::LAN_X_SET_LOCO_FUNCTION(int addr, int func, bool plus) {
  lastControlledAddress = addr;

  byte data = (byte)((plus ? 1 << 6 : 0) + func);
  byte bytes[] = { 0x0a, 0x00, 0x40, 0x00, (byte)0xe4, (byte)0xf8, (byte)(((addr + addrOffs) / 256) | (addr > 128 ? 0xC0 : 0)), (byte)((addr + addrOffs) % 256), data, 0};
  sendCommand(bytes, bytes[0], XOR);
  sprintf(aux, "Adresse=%d Funktion=%d Zustand=%d", addr, func, plus);
  trace(toZ21, diffLastSentReceived, "LAN_X_SET_LOCO_FUNCTION", String(aux));
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::LAN_X_GET_LOCO_INFO(int addr) {
  byte bytes[] = { 0x09, 0x00, 0x40, 0x00, (byte)0xe3, (byte)0xf0, (byte)(((addr + addrOffs) / 256) | (addr > 128 ? 0xC0 : 0)), (byte)((addr + addrOffs) % 256), 0};
  sendCommand(bytes, bytes[0], XOR);
  lastControlledAddress = addr;
  trace(toZ21, diffLastSentReceived, "LAN_X_GET_LOCO_INFO", "Adresse=" + String(addr));
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::LAN_X_CV_POM_WRITE_BYTE(int addr, int cv, byte value) {

  cv--;
  byte bytes[] = { 0x0c, 0x00, 0x40, 0x00, (byte)0xe6, 0x30, (byte)((addr + addrOffs) / 256), (byte)((addr + addrOffs) % 256), (byte)((cv / 256) | 0xec), ((byte)(cv % 256)), value, 0};
  sendCommand(bytes, bytes[0], XOR);
  trace(toZ21, diffLastSentReceived, "LAN_X_CV_POM_WRITE_BYTE", "Adresse=" + String(addr) + ", CV=" + String(cv + 1) + ", Wert=" + String(value));
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::LAN_X_CV_WRITE(int cv, byte value) {

  cv--;
  byte bytes[] = {0x0a, 0x00, 0x040, 0x00, 0x24, 0x12, (byte)(cv / 256), (byte)(cv % 256), value, 0x00};
  sendCommand(bytes, bytes[0], XOR);
  trace(toZ21, diffLastSentReceived, "LAN_X_CV_WRITE", "CV=" + String(cv + 1) + ", Wert=" + String(value));

}

// ----------------------------------------------------------------------------------------------------
//

void Z21::LAN_X_CV_READ(int cv) {

  cv--;
  byte bytes[] = {0x09, 0x00, 0x040, 0x00, 0x23, 0x11, (byte)(cv / 256), (byte)(cv % 256), 0x00};
  sendCommand(bytes, bytes[0], XOR);
  trace(toZ21, diffLastSentReceived, "LAN_X_CV_READ", "CV=" + String(cv + 1));

}


// ----------------------------------------------------------------------------------------------------
// <addXOR> == true -> Checksumme in letztes Byte einfügen

void Z21::sendCommand(byte bytes[], int len, bool addXOR) {

  if (lastMessageSentReceived == 0) {
    diffLastSentReceived = 0;
  } else {
    diffLastSentReceived = millis() - lastMessageSentReceived;
  }
  lastMessageSentReceived = millis();

  if (addXOR) {
    insertChecksum(bytes, len);
  }

#ifdef DEBUG_O
  Serial.printf("Sending %d byte(s) to %s: \n  ", len, ipAddress.c_str());
  for (int i = 0; i < len; i++) {
    Serial.printf("      % 02x ", bytes[i]);
  }
  Serial.print("\n  ");
  for (int i = 0; i < len; i++) {
    Serial.printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(bytes[i])); Serial.print(" ");
  }
  Serial.println();
#endif

  if (WiFi.status() == WL_CONNECTED) {
    Udp.beginPacket(ipAddress.c_str(), Z21_PORT);
    Udp.write(bytes, len);
    Udp.endPacket();
  } else {
    Serial.println("WiFi nicht verbunden, kann (noch nicht) auf UDP schreiben");
  }
}

// ----------------------------------------------------------------------
// Checksumme berechnen
// In letztes Byte des Arrays schreiben an Stelle <length>-1

void Z21::insertChecksum(byte bytes[], int len) {
  bytes[len - 1] = bytes[0];
  for (int i = 1; i < len - 1; i++) {
    bytes[len - 1] ^= bytes[i];
  }
}

// ====================================================================================================
// RECEIVE notifications/results from Z21
// ====================================================================================================

#define BufSize 100
char buf[BufSize];

// ----------------------------------------------------------------------
//

void Z21::receive() {

  static char buf[100];

  // if (Udp.available() > 0) {
  if (Udp.parsePacket() > 0) {

    lastReceived = millis();

    String receivedHex = "";

    if (lastMessageSentReceived == 0) {
      diffLastSentReceived = 0;
    } else {
      diffLastSentReceived = millis() - lastMessageSentReceived;
    }

    lastMessageSentReceived = millis();

    int len = Udp.read(buf, BufSize);
    int remainingLen = len;

    buf[len] = 0;

    receivedHex = "";
    for (int i = 0; i < len; i++) {
      sprintf(aux, "%02x ", buf[i]);
      receivedHex += aux;
    }

    if (buf[0] != len) Serial.printf(">>>>> mehrere Pakete >>>> %s\n", receivedHex.c_str());

    // Schleife, weil mehrere Pakete in Nachricht sein können
    do {

      // LAN_X_BC_TRACK_POWER_OFF ?
      if (buf[0] == 0x07 && buf[4] == 0x61 && buf[5] == 0x00 && buf[6] == 0x61) {
        trace(fromZ21, diffLastSentReceived, "LAN_X_BC_TRACK_POWER_OFF", "");
        setTrackPowerState(BoolState::Off);

        // LAN_X_BC_TRACK_POWER_ON ?
      } else if (buf[0] == 0x07 && buf[4] == 0x61 && buf[5] == 0x01 && buf[6] == 0x60) {
        trace(fromZ21, diffLastSentReceived, "LAN_X_BC_TRACK_POWER_ON", "");
        setTrackPowerState(BoolState::On);
        // setProgStateOn(false);

        // LAN_X_STATUS_CHANGED ?
      } else if (buf[0] == 0x08 && buf[2] == 0x40 && buf[4] == 0x62 && buf[5] == 0x22) {

        setEmergencyStopState((buf[6] & 0x01) > 0 ? BoolState::On : BoolState::Off); // Bit ein
        setTrackPowerState((buf[6] & 0x02) > 0 ? BoolState::Off : BoolState::On);   // Bit ein = off
        setShortCircuitState((buf[6] & 0x04) > 0 ? BoolState::On : BoolState::Off);  // Bit ein = on
        setProgState((buf[6] & 0x20) > 0 ? BoolState::On : BoolState::Off);     // Bit ein = onse

        sprintf(aux, "Gleissp=%s, Nothalt=%s, Kurzschl=%s, ProgModus=%s",
                getTrackPowerState() == BoolState::On ? "Ein" : "Aus",
                getEmergencyStopState() == BoolState::On ? "Ja" : "Nein",
                getShortCircuitState() == BoolState::On ? "Ja" : "Nein",
                getProgState() == BoolState::On ? "Ein" : "Aus");

        trace(fromZ21, diffLastSentReceived, "LAN_X_STATUS_CHANGED", aux);

        // Antwort auf LAN_X_GET_VERSION
      } else if (buf[4] == 0x63) {
        sprintf(aux, "%x.%x", buf[6] >> 4, (buf[6] & 0b00001111));
        xbusVersion = aux;
        sprintf(aux, "%x", buf[7]);
        csID = String(aux) == "12" ? "Z21" : "unbekannt: " + String(aux);
        trace(fromZ21, diffLastSentReceived, "LAN_VERSION", String("X-Bus-Version=") + xbusVersion + ", ID Zentrale=" + csID);

        // Antwort auf LAN_GET_SERIAL_NUMBER
      } else if (buf[2] == 0x10 && buf[3] == 0x00) {
        serialNbr = String(buf[5] * 256 + buf[4]);
        trace(fromZ21, diffLastSentReceived, "LAN_GET_SERIAL_NUMBER", String("Seriennummer=") + serialNbr);

        // Antwort auf LAN_X_GET_FIRMWARE_VERSION
      } else if (buf[0] == 0x09 && buf[2] == 0x40 && buf[4] == 0xf3 && buf[5] == 0x0a) {
        fwVersion = String(buf[6] + 0) + "." + String(buf[7] + 0);
        trace(fromZ21, diffLastSentReceived, "LAN_X_GET_FIRMWARE_VERSION", String("Fw-Version=") + fwVersion);

        // Antwort auf LAN_SYSTEMSTATE_GETDATA oder wenn LAN_SET_BROADCASTFLAGS Flag 0x00000100 gesetzt hat
      } else if (buf[2] == 0x84 && buf[3] == 0x00) {
        currMain = String(buf[5] * 256 + buf[4]) + "mA";
        currProg = String(buf[5] * 256 + buf[6]) + "mA";
        temp = String(buf[5] * 256 + buf[10]) + "&deg;C";
        lowVoltage = (buf[17] & 0x02) > 0;
        highTemp = (buf[17] & 0x01) > 0;

        setEmergencyStopState((buf[16] & 0x01) > 0 ? BoolState::On : BoolState::Off); // Bit ein
        setTrackPowerState((buf[16] & 0x02) > 0 ? BoolState::Off : BoolState::On);   // Bit ein = off
        setShortCircuitState((buf[16] & 0x04) > 0 ? BoolState::On : BoolState::Off);  // Bit ein = on
        setProgState((buf[16] & 0x20) > 0 ? BoolState::On : BoolState::Off);     // Bit ein = on

        trace(fromZ21, diffLastSentReceived, "LAN_SYSTEMSTATE_DATACHANGED",
              "Strom Hauptgl=" + currMain +
              ", Strom Proggleis=" + currProg +
              ", Temperatur=" + temp +
              ", Temp zu hoch=" + (highTemp ? "Ja" : "Nein") +
              ", Spannung zu niedrig=" + (lowVoltage ? "Ja" : "Nein") +
              ", Gleissp=" + (getTrackPowerState() == BoolState::On ? "Ein" : "Aus") +
              ", Nothalt=" + (getEmergencyStopState() == BoolState::On ? "Ja" : "Nein") +
              ", Kurzschl=" + (getShortCircuitState() == BoolState::On? "Ja" : "Nein") +
              ", ProgrModus=" + (getProgState() == BoolState::On ? "Ein" : "Aus"));

        // LAN_BROADCASTFLAGS? Kommando ist allerdings (so) nicht benannt
      } else if (buf[0] == 0x08 && buf[2] == 0x51) {

        flagsValid = true;

        BCF_BASIC = (buf[4] & 0x01) > 0;
        BCF_RBUSFB = (buf[4] & 0x02) > 0;
        BCF_RC = (buf[4] & 0x04) > 0;

        BCF_SYS = (buf[5] & 0x01) > 0;

        BCF_ALLLOCO = (buf[6] & 0x01) > 0;
        BCF_ALLRC = (buf[6] & 0x04) > 0;
        BCF_CANFB = (buf[6] & 0x08) > 0;

        BCF_LNMSG = (buf[7] & 0x01) > 0;
        BCF_LNLOCO = (buf[7] & 0x02) > 0;
        BCF_LNSwitch = (buf[7] & 0x04) > 0;
        BCF_LNFB = (buf[7] & 0x08) > 0;

        trace(fromZ21, diffLastSentReceived, "LAN_BROADCASTFLAGS",
              String(BCF_BASIC ? "Fahren/Schalten=LAN_X_BC_TRACK_POWER_OFF/POWER_ON/PROGRAMMING_MODE/TRACK_SHORT_CIRCUIT/STOPPED/LOCO_INFO(subscribed)/TURNOUT_INFO" : "") +
              String(BCF_RBUSFB ? " RBUS" : "") +
              String(BCF_RC ? " LAN_RAILCOM_DATACHANGED(subscribed)" : "") +
              String(BCF_SYS ? " LAN_SYSTEMSTATE_DATACHANGED)" : "") +
              String(BCF_ALLLOCO ? " LAN_X_LOCO_INFO(all)" : "") +
              String(BCF_ALLRC ? " LAN_RAILCOM_DATACHANGED(all)" : "") +
              String(BCF_CANFB ? " LAN_CAN_DETECTOR" : "") +
              String(BCF_LNMSG ? "  Loconet messages, no loco / switch info" : "") +
              String(BCF_LNLOCO ? " Loconet OPC_LOCO_SPD / DIRF / SND / F912 / EXP_CMD" : "") +
              String(BCF_LNSwitch ? " Loconet OPC_SW_REQ / REP / ACK / STATE" : "") +
              String(BCF_LNFB ? "Loconet feedback LAN_LOCONET_DETECTOR" : "")
             );

        // Antwort auf LAN_GET_HWINFO
      } else if (buf[0] == 0x0c && buf[2] == 0x1a) {

        switch (buf[4]) {
          case 0x00: hwVersion = "Z21 schwarz (alt)"; break;
          case 0x01: hwVersion = "Z21 schwarz oder DR5000"; break;
          case 0x02: hwVersion = "Smartrail"; break;
          case 0x03: hwVersion = "Z21 weisz"; break;
          case 0x04: hwVersion = "Z21 Start"; break;
          default: hwVersion = String(buf[4]) + "?";
        }

        trace(fromZ21, diffLastSentReceived, "LAN_GET_HWINFO", String("HW-Version=") + hwVersion);

        //   } else if (buf[0] == 0x80 && buf[2] == 0x0a) {
        //     if (traceOn) {
        //       z21State.trace(FROM_Z21, diffLastSentReceived, "LAN_LOCONET_Z21_RX", "");
        //     }

      } else if (buf[0] == 0x07 && buf[4] == 0x61 && buf[5] == 0x82) {

        trace(fromZ21, diffLastSentReceived, "LAN_X_UNKNOWN_COMMAND", receivedHex);

        // LAN_X_LOCO_INFO?
      } else if (buf[2] == 0x40 && buf[4] == 0xef) {

        int addr = ((buf[5] & 0x3F) << 8) + buf[6];

        // Unmittelbar durch LAN_X_GET_LOCO_INFO ausgelöst? Dann kommt leider das Übernahmeflag, obwohl
        // es keine Fremdsteuerung ist. In diesem Fall also nicht auswerten
        bool takenOver = lastControlledAddress == addr ? false : (buf[7] &  0b00001000) > 0;
        lastControlledAddress = 0;

        int numSpeedSteps = buf[7] & 0b0000111;
        numSpeedSteps = numSpeedSteps == 0 ? 14 : numSpeedSteps == 2  ? 28 : 128;
        bool forward = buf[8] > 127;
        int fst = buf[8] & 0b01111111;

        fst = dccToUserSpeed(fst);

        // boolean doubleTraction = (buf[9] & (byte)0b1000000) > 0;

        boolean f[MaxFct + 1]; for (int i = 0; i < MaxFct + 1; i++) f[i] = 0;

        f[0] = (buf[9] & 0b00010000) > 0;
        f[1] = (buf[9] & 0b00000001) > 0;
        f[2] = (buf[9] & 0b00000010) > 0;
        f[3] = (buf[9] & 0b00000100) > 0;
        f[4] = (buf[9] & 0b00001000) > 0;

        for (int i = 5; i <= 12; i++) {
          f[i] = (buf[10] & 0b00000001) > 0;
          buf[10] = buf[10] >> 1;
        }
        for (int i = 13; i <= 20; i++) {
          f[i] = (buf[11] & 0b00000001) > 0;
          buf[11] = buf[11] >> 1;
        }
        for (int i = 21; i <= 28; i++) {
          f[i] = (buf[12] & 0b00000001) > 0;
          buf[12] = buf[12] >> 1;
        }

        String functions = "";
        for (int i = 0; i < MaxFct + 1; i++)
          if (f[i]) functions += "F" + String(i) + " ";

        trace(fromZ21, diffLastSentReceived, "LAN_X_LOCO_INFO", "Adresse=" + String(addr) +
              " Richtung=" + String(forward) + " Fahrstufe=" + String(fst) + " &Uuml;bernommen=" + (takenOver ? "ja" : "nein") +
              " Funktionen=" + functions);

        notifyLocoInfoChanged(addr, forward ? Direction::Forward : Direction::Backward, fst, takenOver, numSpeedSteps, f);

        // LAN_X_TURNOUT_INFO?
      } else if (buf[2] == 0x40 && buf[4] == 0x43) {

        int addr = buf[5] * 256 + buf[6];
        addr++;
        bool plus = (buf[7] & 0b00000011) == 1;

        trace(fromZ21, diffLastSentReceived, "LAN_X_TURNOUT_INFO", "W" + String(addr) + (plus ? "+" : "-"));
        notifyAccessoryStateChanged(addr, plus);

      // LAN_X_BC_PROGRAMMING_MODE
      } else if (buf[4] == 0x61 && buf[5] == 0x02 && buf[6] == 0x63) {
        setProgState(BoolState::On);
        trace(fromZ21, diffLastSentReceived, "LAN_X_BC_PROGRAMMING_MODE", "");

        // LAN_X_CV_RESULT?
      } else if (buf[4] == 0x64 && buf[5] == 0x14) {
        int cv = buf[6] * 256 + buf[7] + 1;
        //byte value = buf[8];
        trace(fromZ21, diffLastSentReceived, "LAN_X_CV_RESULT", "CV=" + String(cv) + ", Wert=" + String(cv));
        notifyProgResult(Success, cv);

        //  z21State.cvReadInfo(cv, value);

        // LAN_X_CV_NACK?
      } else if (buf[4] == 0x61 && buf[5] == 0x13) {
        trace(fromZ21, diffLastSentReceived, "LAN_X_CV_NACK", "");
        notifyProgResult(Failure, UNDEFINED_CV_VALUE);

      } else {
          Serial.print(" < - == == NOT EVALUATED == == ");
          trace(fromZ21, diffLastSentReceived, "unausgewertet: ", receivedHex);
      }

      remainingLen -= buf[0];

      if (remainingLen > 0) {
        trace(fromZ21, diffLastSentReceived, "-----Weiteres Paket", String(remainingLen) + " Bytes");

        int handledLen = buf[0];
        // Rest vorkopieren, damit es im nächsten Schleifendurchlauf verarbeitet wird
        for (int i = 0; i < len - handledLen; i++) {
          buf[i] = buf[handledLen + i];
        }

      }             

    } while (remainingLen > 0);

    if (remainingLen < 0) {
      Serial.printf("---- Restpaket nicht stimmig\n");
    }

  } // if (Udp.parsePacket() > 0)

}


// ====================================================================================================
// Etc
// ====================================================================================================

// ----------------------------------------------------------------------------------------------------
//

String Z21::toString(BoolState state, String onLabel, String offLabel) {
  switch (state) {
    case BoolState::On: return onLabel;
    case BoolState::Off: return offLabel;
    case BoolState::Undefined: return "Unbekannt"; 
  }
}

// ----------------------------------------------------------------------------------------------------
//

int Z21::userToDccSpeed(int userSpeed) {
  if (userSpeed == 0) return 0;
  if (userSpeed >= 1) return userSpeed+1;
}

int Z21::dccToUserSpeed(int dccSpeed) {
  if (dccSpeed == 0) return 0;
  if (dccSpeed >= 2) return dccSpeed - 1;
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::setTrackPowerState(BoolState trackPowerState) {
  if (Z21::trackPowerState != trackPowerState) {
    Z21::trackPowerState = trackPowerState;
    notifyTrackPowerStateChanged(trackPowerState);
  }
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::setShortCircuitState(BoolState shortCircuitState) {
  if (Z21::shortCircuitState != shortCircuitState) {
    Z21::shortCircuitState = shortCircuitState;
    notifyShortCircuitStateChanged(shortCircuitState);
  }
  // Nicht immer wird dann auch die zugehörige Gleisspannung-Aus-Notifikation erhalten, es wird aber abgeschaltet, daher:
  if (getShortCircuitState() == BoolState::On) setTrackPowerState(BoolState::Off);
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::setEmergencyStopState(BoolState emergencyStopState) {
  if (Z21::emergencyStopState != emergencyStopState) {
    Z21::emergencyStopState = emergencyStopState;
    notifyEmergencyStopStateChanged(emergencyStopState);
  }
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::setProgState(BoolState progState) {
  if (Z21::progState != progState) {
    Z21::progState = progState;
    notifyProgStateChanged(progState);
  }
}

// ====================================================================================================
// Send state change notifications to observers
// ====================================================================================================

// ----------------------------------------------------------------------------------------------------
//

void Z21::notifyTrackPowerStateChanged(BoolState trackPowerOn) {
  for (int i = 0; i < numObservers; i++)
    if (observers[i] != 0)
      observers[i]->trackPowerStateChanged(trackPowerOn);
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::notifyShortCircuitStateChanged(BoolState shortCircuitState) {
  for (int i = 0; i < numObservers; i++)
    if (observers[i] != 0)
      observers[i]->shortCircuitStateChanged(shortCircuitState);
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::notifyEmergencyStopStateChanged(BoolState emergencyStopState) {
  for (int i = 0; i < numObservers; i++)
    if (observers[i] != 0)
      observers[i]->emergencyStopStateChanged(emergencyStopState);
}


// ----------------------------------------------------------------------------------------------------
//

void Z21::notifyLocoInfoChanged(int addr, Direction dir, int fst, bool takenOver, int numSpeedSteps, bool f[]) {
  for (int i = 0; i < numObservers; i++)
    if (observers[i] != 0)
      observers[i]->locoInfoChanged(addr, dir, fst, takenOver, numSpeedSteps, f);
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::notifyAccessoryStateChanged(int addr, bool plus) {
  for (int i = 0; i < numObservers; i++)
    if (observers[i] != 0)
      observers[i]->accessoryStateChanged(addr, plus);
}


// ----------------------------------------------------------------------------------------------------
//

void Z21::notifyProgStateChanged(BoolState progState) {
  for (int i = 0; i < numObservers; i++)
    if (observers[i] != 0)
      observers[i]->progStateChanged(progState);
}

// ----------------------------------------------------------------------------------------------------
//

void Z21::notifyProgResult(ProgResult result, int value) {
  for (int i = 0; i < numObservers; i++)
    if (observers[i] != 0)
      observers[i]->progResult(result, value);
}

// ----------------------------------------------------------------------------------------------------
//
void Z21::notifyTraceEvent(FromToZ21 direction, long diffLastSentReceived, String message, String parameters) {
  for (int i = 0; i < numObservers; i++)
    if (observers[i] != 0)
      observers[i]->traceEvent(direction, diffLastSentReceived, message, parameters);
}

void Z21::trace(FromToZ21 direction, long diffLastSentReceived, String message, String parameters) {

  notifyTraceEvent(direction, diffLastSentReceived, message, parameters);

  Serial.printf("%s %ldms %s (%s)\n",
                direction == toZ21 ? ">" : "<",
                diffLastSentReceived,
                message.c_str(),
                parameters.c_str()
               );
}



