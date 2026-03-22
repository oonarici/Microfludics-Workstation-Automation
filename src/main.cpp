/**
 * @file main.cpp
 * @brief Entry point for the Microfluidics Workstation Automation application.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Initialises QApplication metadata, creates the MainWindow, and enters
 * the Qt event loop.
 *
 * @copyright LGPL-3.0-or-later
 */

#include <QApplication>

#include "app/main_window.h"

/**
 * @brief Application entry point.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Exit code from QApplication::exec().
 */
int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("MWA"));
  app.setApplicationVersion(QStringLiteral("0.1.0"));
  app.setOrganizationName(QStringLiteral("MWA Team"));

  mwa::app::MainWindow window;
  window.show();

  return app.exec();
}
