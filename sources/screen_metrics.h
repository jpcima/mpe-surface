#pragma once
#include <QApplication>
#include <QScreen>

/** Convert the provided dimension in dp (device-independent pixels) to the
 *  corresponding number of actual pixels on the current display. */
qreal dpToPixels(qreal dp);
