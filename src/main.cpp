/**
 * @file main.cpp
 * @brief Entry point for the Microfluidics Workstation Automation application.
 * @author MWA Team
 * @date 2026-03-22
 * @copyright LGPL-v3
 */

#include <QApplication>

/**
 * @brief Application entry point.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Exit code from QApplication::exec().
 */
int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  return app.exec();
}
