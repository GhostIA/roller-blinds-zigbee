// On-device tests for BlindMotor. Run with the Feather plugged in: pio test
//
// Leave the motor power OFF. The tests only toggle the STEP/DIR/EN pins; they
// never need the driver or motor to be powered. They use their own NVS
// namespace, so the saved blind position is not touched.
#include <Arduino.h>
#include <unity.h>

#include "BlindMotor.h"
#include "config.h"

namespace {

constexpr char kNamespace[] = "blindtest";
constexpr int32_t kTravel = 4000;

BlindMotor::Config testConfig() {
  return BlindMotor::Config{
      cfg::kPinStep,
      cfg::kPinDir,
      cfg::kPinEnable,
      kTravel,
      4000.0f,   // steps/s: a full move takes about a second
      20000.0f,  // steps/s^2: stopping from full speed takes about 400 steps
      false,
      false,
      kNamespace,
      false,  // no TMC UART, the driver is not powered
      &Serial1,
      cfg::kPinUartRx,
      cfg::kPinUartTx,
      cfg::kUartBaud,
      cfg::kTmcAddress,
      cfg::kRSense,
      cfg::kRunCurrentMa,
      cfg::kMicrosteps,
  };
}

BlindMotor* motor = nullptr;

// Calls update() until the move finishes. Returns false on timeout.
bool runUntilIdle(BlindMotor& m, uint32_t timeoutMs = 5000) {
  const uint32_t start = millis();
  do {
    m.update();
  } while (m.isMoving() && millis() - start < timeoutMs);
  return !m.isMoving();
}

}  // namespace

void setUp() {
  Preferences prefs;
  prefs.begin(kNamespace, false);
  prefs.clear();
  prefs.end();

  motor = new BlindMotor(testConfig());
  
  motor->begin();
}

void tearDown() {
  delete motor;
  motor = nullptr;
}

void test_starts_fully_open_with_no_saved_position() {
  TEST_ASSERT_EQUAL_UINT8(100, motor->openPercent());
  TEST_ASSERT_FALSE(motor->isMoving());
}

void test_moves_to_requested_percent() {
  motor->requestOpenPercent(50);
  TEST_ASSERT_TRUE(runUntilIdle(*motor));
  TEST_ASSERT_EQUAL_UINT8(50, motor->openPercent());
}

void test_close_then_open() {
  motor->requestClose();
  TEST_ASSERT_TRUE(runUntilIdle(*motor));
  TEST_ASSERT_EQUAL_UINT8(0, motor->openPercent());

  motor->requestOpen();
  TEST_ASSERT_TRUE(runUntilIdle(*motor));
  TEST_ASSERT_EQUAL_UINT8(100, motor->openPercent());
}

void test_percent_above_100_means_fully_open() {
  motor->requestClose();
  TEST_ASSERT_TRUE(runUntilIdle(*motor));

  motor->requestOpenPercent(200);
  TEST_ASSERT_TRUE(runUntilIdle(*motor));
  TEST_ASSERT_EQUAL_UINT8(100, motor->openPercent());
}

void test_stop_halts_partway() {
  motor->requestClose();

  // Let it get at least 20% of the way, then ask it to stop.
  const uint32_t start = millis();
  while (motor->openPercent() > 80 && millis() - start < 5000) {
    motor->update();
  }
  TEST_ASSERT_LESS_OR_EQUAL_UINT8(80, motor->openPercent());

  motor->requestStop();
  TEST_ASSERT_TRUE(runUntilIdle(*motor));

  const uint8_t percent = motor->openPercent();
  TEST_ASSERT_LESS_THAN_UINT8(80, percent);  // it kept going while slowing
  TEST_ASSERT_GREATER_THAN_UINT8(0, percent);  // but did not reach closed
}
void test_speed_defaults_to_the_config_values() {
  TEST_ASSERT_EQUAL_FLOAT(4000.0f, motor->maxSpeed());
  TEST_ASSERT_EQUAL_FLOAT(20000.0f, motor->acceleration());
}

void test_speed_and_acceleration_survive_a_restart() {
  motor->setMaxSpeed(2500.0f);
  motor->setAcceleration(9000.0f);
  motor->update();  // update() applies and saves the change

  BlindMotor restarted(testConfig());
  restarted.begin();
  TEST_ASSERT_EQUAL_FLOAT(2500.0f, restarted.maxSpeed());
  TEST_ASSERT_EQUAL_FLOAT(9000.0f, restarted.acceleration());
}

void test_non_positive_speed_is_ignored() {
  motor->setMaxSpeed(0.0f);
  motor->setMaxSpeed(-5.0f);
  motor->setAcceleration(0.0f);
  motor->update();
  TEST_ASSERT_EQUAL_FLOAT(4000.0f, motor->maxSpeed());
  TEST_ASSERT_EQUAL_FLOAT(20000.0f, motor->acceleration());
}
  
void test_new_request_during_a_move_changes_the_target() {
  motor->requestClose();
  const uint32_t start = millis();
  while (motor->openPercent() > 90 && millis() - start < 5000) {
    motor->update();
  }

  motor->requestOpenPercent(100);
  TEST_ASSERT_TRUE(runUntilIdle(*motor));
  TEST_ASSERT_EQUAL_UINT8(100, motor->openPercent());
}

void test_position_survives_a_restart() {
  motor->requestOpenPercent(25);
  TEST_ASSERT_TRUE(runUntilIdle(*motor));
  TEST_ASSERT_EQUAL_UINT8(25, motor->openPercent());

  // A new instance with the same namespace is what a reboot looks like.
  BlindMotor restarted(testConfig());
  restarted.begin();
  TEST_ASSERT_EQUAL_UINT8(25, restarted.openPercent());
}

void setup() {
  delay(3000);  // give the host time to open the serial port
  UNITY_BEGIN();
  RUN_TEST(test_starts_fully_open_with_no_saved_position);
  RUN_TEST(test_moves_to_requested_percent);
  RUN_TEST(test_close_then_open);
  RUN_TEST(test_percent_above_100_means_fully_open);
  RUN_TEST(test_stop_halts_partway);
  RUN_TEST(test_new_request_during_a_move_changes_the_target);
  RUN_TEST(test_position_survives_a_restart);
  RUN_TEST(test_speed_defaults_to_the_config_values);
  RUN_TEST(test_speed_and_acceleration_survive_a_restart);
  RUN_TEST(test_non_positive_speed_is_ignored);
  UNITY_END();
}

void loop() {} 