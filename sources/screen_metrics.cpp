#include "screen_metrics.h"

qreal dpToPixels(qreal dp)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    static qreal scaleFactor = QApplication::primaryScreen()->physicalDotsPerInch() / 160.0;
    return dp * scaleFactor;
#else
    return dp;
#endif
}
