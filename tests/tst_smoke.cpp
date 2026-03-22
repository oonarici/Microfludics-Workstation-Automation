/**
 * @file tst_smoke.cpp
 * @brief Smoke test to verify the Qt Test framework is operational.
 * @author MWA Team
 * @date 2026-03-22
 * @copyright LGPL-v3
 */

#include <QObject>
#include <QString>
#include <QtTest>

/// @brief Smoke test class that validates basic Qt functionality.
class TstSmoke : public QObject {
  Q_OBJECT

 private slots:
  /// @brief Verifies QString::number produces the expected result.
  void intToStringConversion();
};

void TstSmoke::intToStringConversion() {
  const int value = 42;
  const QString result = QString::number(value);
  QCOMPARE(result, QStringLiteral("42"));
}

QTEST_MAIN(TstSmoke)
#include "tst_smoke.moc"
