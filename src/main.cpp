#include "microfluidics/app.h"

#include <iostream>

#ifdef MICROFLUIDICS_HAVE_QT
#include <QCoreApplication>
#endif

int main(int argc, char** argv) {
#ifdef MICROFLUIDICS_HAVE_QT
    QCoreApplication app(argc, argv);
    (void)app;
#else
    (void)argc;
    (void)argv;
#endif

    std::cout << microfluidics::app_name() << "\n";
    return 0;
}
