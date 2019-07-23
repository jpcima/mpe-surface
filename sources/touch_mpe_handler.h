#pragma once
#include "touch_piano_listener.h"

class TouchMpeHandler : public TouchPianoListener {
public:
    void setMpeLowerValue(unsigned value);
    void setBendRangeValue(unsigned value);
    void sendConfiguration();

public:
    virtual void sendMidi(const quint8 *data, size_t length, ulong timestamp) = 0;
    void sendMidi2(quint8 data0, quint8 data1, ulong timestamp);
    void sendMidi3(quint8 data0, quint8 data1, quint8 data2, ulong timestamp);

private:
    void clear();

    enum ChannelType {
        LowerMaster, LowerMember,
        UpperMaster, UpperMember,
    };

    ChannelType getChannelType(int ch) const;
    int activeChannelForId(ChannelType type, int id);

    bool hasZone(MpeZone z) const;
    int firstZoneChannel(MpeZone z) const;
    int lastZoneChannel(MpeZone z) const;

public:
    void touchBegin(MpeZone zone, int id, Point3d xyz, ulong timestamp) override;
    void touchUpdate(MpeZone zone, int id, Point3d xyz, ulong timestamp) override;
    void touchEnd(MpeZone zone, int id, ulong timestamp) override;

    void sendChannelUpdate(int ch, Point3d xyz, ulong timestamp);
    void sendChannelRelease(int ch, ulong timestamp);

private:
    unsigned mpeLowerValue_ = 7;
    unsigned bendRangeValue_ = 48;

    int lowerIndex_ = 1;
    int upperIndex_ = 14;

    struct ChannelInfo {
        bool active = false;
        int touchId;
        unsigned note;
    };

    ChannelInfo channels_[16];
};
