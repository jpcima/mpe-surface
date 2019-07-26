#pragma once
#include "mpe_definitions.h"

class TouchPianoListener {
public:
    virtual ~TouchPianoListener() {}

    virtual void touchBegin(MpeZone zone, int id, Point3d xyz, ulong timestamp) = 0;
    virtual void touchUpdate(MpeZone zone, int id, Point3d xyz, ulong timestamp) = 0;
    virtual void touchEnd(MpeZone zone, int id, ulong timestamp) = 0;
};
