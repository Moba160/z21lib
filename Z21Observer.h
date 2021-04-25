#ifndef Z21OBSERVER_H
#define Z21OBSERVER_H

#include <Arduino.h>
#include <Z21Types.h>

/*
    Interface für Observer, die über Z21-Zustandswechsel informiert werden wollen
*/

class Z21Observer { 

    public:

        virtual void trackPowerStateChanged(BoolState trackPowerState) {};
        virtual void shortCircuitStateChanged(BoolState shortCircuitState) {};
        virtual void emergencyStopStateChanged(BoolState emergencyStopState) {};
        virtual void progStateChanged(BoolState progState) {};
        virtual void progResult(ProgResult result, int value) {}
        virtual void locoInfoChanged(int addr, Direction dir, int fst, bool takenOver, int numSpeedSteps, bool f[]) {};
        virtual void accessoryStateChanged(int addr, bool plus) {};
        virtual void traceEvent(FromToZ21 direction, long diffLastSentReceived, String message, String parameters) {};


};

#endif