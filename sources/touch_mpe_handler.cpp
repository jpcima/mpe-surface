#include "touch_mpe_handler.h"
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <cstdint>

void TouchMpeHandler::setMpeLowerValue(unsigned value)
{
    mpeLowerValue_ = value;
}

void TouchMpeHandler::setBendRangeValue(unsigned value)
{
    bendRangeValue_ = value;
}

void TouchMpeHandler::sendConfiguration()
{
    clear();

    // lower zone
    sendMidi3(0xb0, 0x79, 0x00, 0);
    sendMidi3(0xb0, 0x64, 0x06, 0);
    sendMidi3(0xb0, 0x65, 0x00, 0);
    sendMidi3(0xb0, 0x06, (uint8_t)mpeLowerValue_, 0);
    // upper zone
    sendMidi3(0xbf, 0x79, 0x00, 0);
    sendMidi3(0xbf, 0x64, 0x06, 0);
    sendMidi3(0xbf, 0x65, 0x00, 0);
    sendMidi3(0xbf, 0x06, (uint8_t)(14 - mpeLowerValue_), 0);

    // lower bend range
    sendMidi3(0xb0, 0x64, 0x00, 0);
    sendMidi3(0xb0, 0x65, 0x00, 0);
    sendMidi3(0xb0, 0x06, (uint8_t)bendRangeValue_, 0);
    // upper bend range
    sendMidi3(0xbf, 0x64, 0x00, 0);
    sendMidi3(0xbf, 0x65, 0x00, 0);
    sendMidi3(0xbf, 0x06, (uint8_t)bendRangeValue_, 0);
}

void TouchMpeHandler::sendMidi2(quint8 data0, quint8 data1, ulong timestamp)
{
    uint8_t data[] = {data0, data1};
    sendMidi(data, sizeof(data), timestamp);
}

void TouchMpeHandler::sendMidi3(quint8 data0, quint8 data1, quint8 data2, ulong timestamp)
{
    uint8_t data[] = {data0, data1, data2};
    sendMidi(data, sizeof(data), timestamp);
}

void TouchMpeHandler::clear()
{
    lowerIndex_ = firstZoneChannel(MpeZone::Lower);
    upperIndex_ = firstZoneChannel(MpeZone::Upper);

    for (int ch = 0; ch < 16; ++ch) {
        if (channels_[ch].active) {
            sendChannelRelease(ch, 0);
            channels_[ch].active = false;
        }
    }
}

auto TouchMpeHandler::getChannelType(int ch) const -> ChannelType
{
    ChannelType t = (ChannelType)-1;

    if (ch == 0 && hasZone(MpeZone::Lower))
        t = LowerMaster;
    else if (ch == 15 && hasZone(MpeZone::Upper))
        t = UpperMaster;
    else if (ch >= firstZoneChannel(MpeZone::Lower) && ch <= lastZoneChannel(MpeZone::Lower))
        t = LowerMember;
    else if (ch >= firstZoneChannel(MpeZone::Upper) && ch <= lastZoneChannel(MpeZone::Upper))
        t = UpperMember;

    return t;
}

int TouchMpeHandler::activeChannelForId(ChannelType type, int id)
{
    for (int ch = 0; ch < 16; ++ch) {
        if (channels_[ch].active && channels_[ch].touchId == id && getChannelType(ch) == type)
            return ch;
    }
    return -1;
}

bool TouchMpeHandler::hasZone(MpeZone z) const
{
    return mpeLowerValue_ != ((z == MpeZone::Lower) ? 0 : 15);
}

int TouchMpeHandler::firstZoneChannel(MpeZone z) const
{
    if (!hasZone(z))
        return -1;
    return (z == MpeZone::Lower) ? 1 : (mpeLowerValue_ + 1);
}

int TouchMpeHandler::lastZoneChannel(MpeZone z) const
{
    if (!hasZone(z))
        return -1;
    return (z == MpeZone::Lower) ? mpeLowerValue_ : 14;
}

void TouchMpeHandler::touchBegin(MpeZone zone, int id, Point3d xyz, ulong timestamp)
{
    // qDebug() << "touchBegin" << id << xyz;

    if (!hasZone(zone))
        return;

    ChannelType type = (zone == MpeZone::Lower) ? LowerMember : UpperMember;

    int ch = activeChannelForId(type, id);
    if (ch != -1) {
        sendMidi3(0x80|ch, channels_[ch].note, 0, timestamp);
        channels_[ch].active = false;
    }

    int *index = (zone == MpeZone::Lower) ? &lowerIndex_ : &upperIndex_;
    ch = *index;
    *index = (ch == lastZoneChannel(zone)) ? firstZoneChannel(zone) : (ch + 1);

    ChannelInfo &channel = channels_[ch];
    channel.active = true;
    channel.touchId = id;
    unsigned note = channel.note = std::max(0, std::min(127, (int)xyz.x));

    sendChannelUpdate(ch, xyz, timestamp);
    sendMidi3(0x90|ch, note, 100/*default velocity*/, timestamp);
}

void TouchMpeHandler::touchUpdate(MpeZone zone, int id, Point3d xyz, ulong timestamp)
{
    // qDebug() << "touchUpdate" << id << xyz;

    if (!hasZone(zone))
        return;

    ChannelType type = (zone == MpeZone::Lower) ? LowerMember : UpperMember;

    int ch = activeChannelForId(type, id);
    if (ch != -1) {
        sendChannelUpdate(ch, xyz, timestamp);
    }
}

void TouchMpeHandler::touchEnd(MpeZone zone, int id, ulong timestamp)
{
    // qDebug() << "touchEnd" << id;

    if (!hasZone(zone))
        return;

    ChannelType type = (zone == MpeZone::Lower) ? LowerMember : UpperMember;

    int ch = activeChannelForId(type, id);
    if (ch != -1) {
        sendChannelRelease(ch, timestamp);
        channels_[ch].active = false;
    }
}

void TouchMpeHandler::sendChannelUpdate(int ch, Point3d xyz, ulong timestamp)
{
    ChannelInfo &channel = channels_[ch];
    unsigned note = channel.note;
    double bend = ((xyz.x - note) / bendRangeValue_ + 1.0) * 8192.0;
    unsigned bend14 = std::max(0, std::min(16383, (int)std::lround(bend)));
    unsigned mod7 = std::max(0, std::min(127, (int)(xyz.y * 127.0)));
    unsigned pressure7 = std::max(0, std::min(127, (int)(xyz.z * 127.0)));

    // Note on, Pitch bend, Controller 74, Channel Pressure
    sendMidi3(0xe0|ch, bend14 & 0x7f, (bend14 >> 7) & 0x7f, timestamp);
    sendMidi3(0xb0|ch, 74, mod7, timestamp);
    sendMidi2(0xd0|ch, pressure7, timestamp);
}

void TouchMpeHandler::sendChannelRelease(int ch, ulong timestamp)
{
    sendMidi2(0xd0|ch, 0, timestamp);
    sendMidi3(0x80|ch, channels_[ch].note, 0, timestamp);
}
