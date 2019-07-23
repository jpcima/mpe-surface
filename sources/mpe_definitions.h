#pragma once

enum class MpeZone {
    Lower,
    Upper,
};

///
struct Point3d {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

///
inline bool operator==(const Point3d &a, const Point3d &b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

inline bool operator!=(const Point3d &a, const Point3d &b)
{
    return !operator==(a, b);
}

///
#include <QtGlobal>
#include <QDebug>

inline QDebug operator<<(QDebug debug, const Point3d &p)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '(' << p.x << ", " << p.y << ", " << p.z << ')';
    return debug;
}
