/**
 * @file test_serial_utils.cpp
 * @brief Tests for mwa::hardware::serial_utils helper functions.
 * @date 2026-03-27
 * @copyright LGPL-3.0-or-later
 */

#include <QObject>
#include <QString>
#include <QtTest>

#include "hardware/serial_utils.h"

namespace utils = mwa::hardware::serial_utils;

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------
class TestSerialUtils : public QObject {
  Q_OBJECT

 private slots:
  // -- A. isOkResponse ------------------------------------------------------
  void test_isOkResponse_exactOk();
  void test_isOkResponse_okWithTrailing();
  void test_isOkResponse_caseInsensitive();
  void test_isOkResponse_empty();
  void test_isOkResponse_errorString();
  void test_isOkResponse_partialMatch();

  // -- B. formatCommandError ------------------------------------------------
  void test_formatCommandError_withResponse();
  void test_formatCommandError_emptyResponse();
  void test_formatCommandError_whitespaceResponse();
};

// ---------------------------------------------------------------------------
// A. isOkResponse
// ---------------------------------------------------------------------------
void TestSerialUtils::test_isOkResponse_exactOk() {
  QVERIFY(utils::isOkResponse(QStringLiteral("OK")));
}

void TestSerialUtils::test_isOkResponse_okWithTrailing() {
  QVERIFY(utils::isOkResponse(QStringLiteral("OK done")));
}

void TestSerialUtils::test_isOkResponse_caseInsensitive() {
  QVERIFY(utils::isOkResponse(QStringLiteral("ok")));
  QVERIFY(utils::isOkResponse(QStringLiteral("Ok")));
  QVERIFY(utils::isOkResponse(QStringLiteral("oK")));
}

void TestSerialUtils::test_isOkResponse_empty() {
  QVERIFY(!utils::isOkResponse(QString()));
  QVERIFY(!utils::isOkResponse(QStringLiteral("")));
}

void TestSerialUtils::test_isOkResponse_errorString() {
  QVERIFY(!utils::isOkResponse(QStringLiteral("ERR")));
  QVERIFY(!utils::isOkResponse(QStringLiteral("FAIL")));
  QVERIFY(!utils::isOkResponse(QStringLiteral("NAK")));
}

void TestSerialUtils::test_isOkResponse_partialMatch() {
  // "O" alone should not match "OK".
  QVERIFY(!utils::isOkResponse(QStringLiteral("O")));
}

// ---------------------------------------------------------------------------
// B. formatCommandError
// ---------------------------------------------------------------------------
void TestSerialUtils::test_formatCommandError_withResponse() {
  const QString result = utils::formatCommandError(
      QStringLiteral("LedController"),
      QStringLiteral("setIntensity"),
      QStringLiteral("NAK"));

  QVERIFY(result.startsWith(
      QStringLiteral("LedController: setIntensity failed")));
  QVERIFY(result.endsWith(QStringLiteral("NAK")));
}

void TestSerialUtils::test_formatCommandError_emptyResponse() {
  const QString result = utils::formatCommandError(
      QStringLiteral("StageController"),
      QStringLiteral("HOME"),
      QString());

  QVERIFY(result.startsWith(
      QStringLiteral("StageController: HOME failed")));
  QVERIFY(result.endsWith(QStringLiteral("timeout")));
}

void TestSerialUtils::test_formatCommandError_whitespaceResponse() {
  // Non-empty whitespace is NOT treated as timeout.
  const QString result = utils::formatCommandError(
      QStringLiteral("Driver"),
      QStringLiteral("CMD"),
      QStringLiteral(" "));

  QVERIFY(result.contains(QStringLiteral(" ")));
  QVERIFY(!result.contains(QStringLiteral("timeout")));
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestSerialUtils)
#include "test_serial_utils.moc"
