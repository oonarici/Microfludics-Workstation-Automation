/**
 * @file test_settings_manager.cpp
 * @brief Adversarial tests for mwa::core::SettingsManager.
 * @date 2026-03-22
 * @copyright LGPL-3.0-or-later
 */

#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QVariant>
#include <QtTest>

#include "core/settings_manager.h"

using mwa::core::SettingsManager;

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------
class TestSettingsManager : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();
  void init();

  void test_instance_returnsSameObject();
  void test_setValue_roundTrip_int();
  void test_setValue_roundTrip_double();
  void test_setValue_roundTrip_string();
  void test_setValue_roundTrip_bool();
  void test_value_returnsDefault_whenKeyMissing();
  void test_value_returnsStoredValue_notDefault();
  void test_settingChanged_emittedOnNewValue();
  void test_settingChanged_notEmitted_onSameValueSetAgain();
  void test_settingChanged_emittedOnValueChange();
  void test_settingChanged_signalCarriesGroupKeyValue();
  void test_differentGroups_isolateSettings();
  void test_emptyKey_doesNotCrash();
  void test_emptyGroup_doesNotCrash();
  void test_saveAll_loadAll_doNotCrash();
  void test_overwrite_previouslySetKey();

 private:
  // Unique group names per test slot to avoid cross-contamination with
  // the persistent QSettings store (which the singleton always uses).
  QString uniqueGroup(const QString& suffix) const;
};

// ---------------------------------------------------------------------------
void TestSettingsManager::initTestCase() {
  // Touch singleton to ensure it exists.
  SettingsManager::instance();
}

void TestSettingsManager::init() {
  // Nothing to reset — tests use unique group names.
}

QString TestSettingsManager::uniqueGroup(const QString& suffix) const {
  // Use the test object name + suffix so each test gets its own namespace.
  return QStringLiteral("Test_%1_%2")
      .arg(QTest::currentTestFunction())
      .arg(suffix);
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_instance_returnsSameObject() {
  SettingsManager* a = &SettingsManager::instance();
  SettingsManager* b = &SettingsManager::instance();
  QVERIFY2(a == b,
           "instance() must return the same address on every call");
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_setValue_roundTrip_int() {
  const QString group = uniqueGroup("int");
  SettingsManager::instance().setValue(group, QStringLiteral("key"), 42);
  QVariant result =
      SettingsManager::instance().value(group, QStringLiteral("key"));
  QCOMPARE(result.toInt(), 42);
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_setValue_roundTrip_double() {
  const QString group = uniqueGroup("dbl");
  SettingsManager::instance().setValue(group, QStringLiteral("key"), 3.14);
  QVariant result =
      SettingsManager::instance().value(group, QStringLiteral("key"));
  QVERIFY2(qAbs(result.toDouble() - 3.14) < 1e-9,
           "Double round-trip must preserve value");
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_setValue_roundTrip_string() {
  const QString group = uniqueGroup("str");
  const QString expected = QStringLiteral("Hello MWA");
  SettingsManager::instance().setValue(group, QStringLiteral("key"), expected);
  QVariant result =
      SettingsManager::instance().value(group, QStringLiteral("key"));
  QCOMPARE(result.toString(), expected);
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_setValue_roundTrip_bool() {
  const QString group = uniqueGroup("bool");
  SettingsManager::instance().setValue(group, QStringLiteral("key"), true);
  QVariant result =
      SettingsManager::instance().value(group, QStringLiteral("key"));
  QVERIFY2(result.toBool() == true, "Bool round-trip must return true");
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_value_returnsDefault_whenKeyMissing() {
  const QString group = uniqueGroup("nokey");
  const QVariant def = QStringLiteral("default_val");
  QVariant result =
      SettingsManager::instance().value(group, QStringLiteral("absent"), def);
  QCOMPARE(result.toString(), def.toString());
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_value_returnsStoredValue_notDefault() {
  const QString group = uniqueGroup("pref");
  SettingsManager::instance().setValue(group, QStringLiteral("k"),
                                       QStringLiteral("stored"));
  QVariant result = SettingsManager::instance().value(
      group, QStringLiteral("k"), QStringLiteral("fallback"));
  QCOMPARE(result.toString(), QStringLiteral("stored"));
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_settingChanged_emittedOnNewValue() {
  const QString group = uniqueGroup("sig_new");
  const QString key = QStringLiteral("k");
  // Pre-seed a sentinel value distinct from what we will set, so that
  // even if a previous test run persisted this key the spy will observe
  // a genuine change.
  SettingsManager::instance().setValue(group, key,
                                       QStringLiteral("__sentinel__"));

  QSignalSpy spy(&SettingsManager::instance(),
                 &SettingsManager::settingChanged);
  SettingsManager::instance().setValue(group, key, QStringLiteral("first"));
  QVERIFY2(spy.count() >= 1,
           "settingChanged must fire when value changes to a new value");
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_settingChanged_notEmitted_onSameValueSetAgain() {
  const QString group = uniqueGroup("sig_same");
  // Write the value once so it is persisted.
  SettingsManager::instance().setValue(group, QStringLiteral("k"),
                                       QStringLiteral("stable"));

  QSignalSpy spy(&SettingsManager::instance(),
                 &SettingsManager::settingChanged);
  // Set the exact same value again.
  SettingsManager::instance().setValue(group, QStringLiteral("k"),
                                       QStringLiteral("stable"));
  QCOMPARE(spy.count(), 0);
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_settingChanged_emittedOnValueChange() {
  const QString group = uniqueGroup("sig_change");
  SettingsManager::instance().setValue(group, QStringLiteral("k"),
                                       QStringLiteral("old"));
  QSignalSpy spy(&SettingsManager::instance(),
                 &SettingsManager::settingChanged);
  SettingsManager::instance().setValue(group, QStringLiteral("k"),
                                       QStringLiteral("new"));
  QCOMPARE(spy.count(), 1);
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_settingChanged_signalCarriesGroupKeyValue() {
  const QString group = uniqueGroup("sig_payload");
  const QString key = QStringLiteral("myKey");
  const QVariant val = QStringLiteral("myValue");

  // Ensure a prior different value so the signal fires.
  SettingsManager::instance().setValue(group, key,
                                       QStringLiteral("__initial__"));

  QSignalSpy spy(&SettingsManager::instance(),
                 &SettingsManager::settingChanged);
  SettingsManager::instance().setValue(group, key, val);

  QCOMPARE(spy.count(), 1);
  const QList<QVariant>& args = spy.at(0);
  QCOMPARE(args.at(0).toString(), group);
  QCOMPARE(args.at(1).toString(), key);
  QCOMPARE(args.at(2).toString(), val.toString());
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_differentGroups_isolateSettings() {
  const QString group_a = uniqueGroup("grp_A");
  const QString group_b = uniqueGroup("grp_B");
  const QString key = QStringLiteral("shared_key");

  SettingsManager::instance().setValue(group_a, key,
                                       QStringLiteral("valueA"));
  SettingsManager::instance().setValue(group_b, key,
                                       QStringLiteral("valueB"));

  QCOMPARE(
      SettingsManager::instance().value(group_a, key).toString(),
      QStringLiteral("valueA"));
  QCOMPARE(
      SettingsManager::instance().value(group_b, key).toString(),
      QStringLiteral("valueB"));
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_emptyKey_doesNotCrash() {
  const QString group = uniqueGroup("emptykey");
  // Should not throw or crash — result is implementation-defined.
  SettingsManager::instance().setValue(group, QString(), 99);
  SettingsManager::instance().value(group, QString());
  QVERIFY(true);  // Reaching here means no crash.
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_emptyGroup_doesNotCrash() {
  // Should not throw or crash.
  SettingsManager::instance().setValue(QString(),
                                       QStringLiteral("k_empty_grp"), 1);
  SettingsManager::instance().value(QString(),
                                    QStringLiteral("k_empty_grp"));
  QVERIFY(true);
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_saveAll_loadAll_doNotCrash() {
  SettingsManager::instance().saveAll();
  SettingsManager::instance().loadAll();
  QVERIFY(true);
}

// ---------------------------------------------------------------------------
void TestSettingsManager::test_overwrite_previouslySetKey() {
  const QString group = uniqueGroup("overwrite");
  const QString key = QStringLiteral("k");
  SettingsManager::instance().setValue(group, key, 1);
  SettingsManager::instance().setValue(group, key, 2);
  QCOMPARE(SettingsManager::instance().value(group, key).toInt(), 2);
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestSettingsManager)
#include "test_settings_manager.moc"
