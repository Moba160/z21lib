English readers: This is a Z21 library for digital command-control stations implementing the X-Bus (Z21, DR5000, XP-Multi). It is published for intra-project use, but can also be used stand-alone with your project.

This libary is registerd in PlatformIO as "Moba160/z21lib".

# Z21 Library
Diese Library ist die Schnittstelle zu X-Bus-basierten Zentralen wie Z21 (Roco), DR5000 (Digikeijs) oder dem XpressNet-Adapter XP-Multi (MD). Sie implementiert die derzeit von mir benötigten Routinen des Z21 LAN-Protokolls (derzeitig Version 1.10), basierend auf UDP.

Diese Library ist bei PlatformIO registriert als "Moba160/z21lib".

# Benutzung
## Voraussetzung
Eine WiFi-Verbindung ist Voraussetzung und auf die übliche Weise herzustellen. Ebenso ist die UDP-Library einzubinden.

## Include
Nicht überraschend wird die Library mit

`#include <Z21.h>`
eingebunden.

## Setup
Im Setup ist der Library die Adresse der Z21 bekanntzugeben:

```
Z21::setIPAddress("192.168.0.111"); // Standard-Adresse, ggf. abändern
```

Ebenfalls ist die Library noch zu initialisieren mit Z21::init(), was aber zweckmäßigerweise erst später erfolgt, siehe Loop-Abschnitt:

## Loop
In der Loop ist regelmäßig
```
Z21::receive();
```
aufzurufen, damit asynchron von der Z21 erhaltene Nachrichten empfangen und verarbeitet werden können.

Weiterhin muss mindestens einmal in 60s die Z21 adressiert werden, damit sie die Clientverbindung für Notifikationen aufrechterhält.

Die loop()-Funktion könnte so aussehen (Z21_PORT ist in der Library mit 21105 definiert und Z21_HEARBEAT auf einen Wert kleiner 60s):

```
bool UDPServerInitialised = false;
long lastHeartbeatSent = 0;

void loop() {

  // Da die WLAN-Verbindung noch nicht unbedingt abgeschlossen ist,
  // kann der UDP-Server auch erst nach dem Verbindungsaufbau gestartet werden
  if (WiFi.status() == WL_CONNECTED) {

    if (!UDPServerInitialised) {
      Udp.begin(Z21_PORT);
      UDPServerInitialised = true;
      Z21::init();
    }

    if (millis() - lastHeartbeatSent > Z21_HEARTBEAT) {
      Z21::heartbeat();
      lastHeartbeatSent = millis();
    }

  }

  // Notifikationen erhalten
  Z21::receive();

}

```

# Z21-Kommandos
Im Headerfile Z21.h sind die Funktionen beschrieben. Am Beispiel von

```
// Lok mit Adresse <adr> fahren in Richtung <dir> und mit Geschwindigkeit <speed>
static void LAN_X_SET_LOCO_DRIVE(int addr, Direction dir, int speed);
```
wäre die Benutzung

```
Z21::LAN_X_SET_LOCO_DRIVE(3, FORWARD, 120);
```
um die Lok mit Adresse 3 vorwärts mit Fahrstufe 120 zu fahren.

# Notifikationen
Sowohl als Ergebnisse auf Kommandos (z.B. LAN_X_GET_LOCO_INFO) als auch durch andere X-Bus-Geräte ausgelöst, schickt die Z21 Notifikationen. Um diese zu erhalten, muss im Client von der Klasse Z21Observer abgeleitet werden, welche entsprechende Notifikationen erhält:

```
class Z21Observer { 

  public:

    virtual void trackPowerStateChanged(bool trackPowerOff) {};
    virtual void progModeStateChanged(bool progModeOff) {};
    virtual void progResult(ProgResult result, int value) {}
    virtual void locoInfoChanged(int addr, Direction dir, int fst, bool takenOver, int numSpeedSteps, bool f[]) {};
    virtual void accessoryStateChanged(int addr, bool plus) {};
    virtual void traceEvent(FromToZ21 direction, long diffLastSentReceived, String message, String parameters) {};

};
```
(Die Methoden sind nicht abstrakt (=0) sondern leer ({}) vordefiniert, damit ableitende Klassen nicht gezwungen sind, Methoden für Notifikationen zu implementieren, an denen sie nicht interessiert sind.

Eine eigene Klasse, die diese Notifikationen enthält, könnte also so aussehen:

```
#include <Z21Observer.h>

class MyClass : public Z21Observer {
  
  public: // oder andere Sichtbarkeit
  
  virtual void trackPowerStateChanged(bool trackPowerOff) {
      Serial.printf("Die Gleisspannung ist %s\n", trackPowerOff ? "AUS" : "EIN");
  }
  
};
```