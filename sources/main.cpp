#include "application.h"

int main(int argc, char *argv[])
{
    Application::initGL();

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    Application::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    Application a(argc, argv);
    a.init();
    return a.exec();
}
