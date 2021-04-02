#ifndef Z21OBSERVER_H
#define Z21OBSERVER_H

#include <Z21Types.h>

/*
    Interface für Observer, die über Z21-Zustandswechsel informiert werden wollen
*/

class Z21Observer { 

    public:

        virtual void trackPowerStateChanged(bool trackPowerOff) {};
        virtual void progModeStateChanged(bool progModeOff) {};
        virtual void progResult(ProgResult result, int value) {}
        virtual void locoInfoChanged(int addr, Direction dir, int fst, bool takenOver, int numSpeedSteps, bool f[]) {};
        virtual void accessoryStateChanged(int addr, bool plus) {};

};

#endif