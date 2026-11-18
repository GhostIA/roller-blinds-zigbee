// Host tests for the percent <-> step conversions. Run: pio test -e native
#include <unity.h>

#include "BlindMath.h"

using blindmath::percentToSteps;
using blindmath::stepsToPercent;

void setUp() {}
void tearDown() {}

void test_open_is_step_zero() {
  TEST_ASSERT_EQUAL_INT32(0, percentToSteps(100, 3200));
}

void test_closed_is_full_travel() {
  TEST_ASSERT_EQUAL_INT32(3200, percentToSteps(0, 3200));
}

void test_halfway() {
  TEST_ASSERT_EQUAL_INT32(1600, percentToSteps(50, 3200));
}

void test_percent_above_100_is_clamped() {
  TEST_ASSERT_EQUAL_INT32(0, percentToSteps(150, 3200));
  TEST_ASSERT_EQUAL_INT32(0, percentToSteps(255, 3200));
}

void test_steps_to_percent_endpoints() {
  TEST_ASSERT_EQUAL_UINT8(100, stepsToPercent(0, 3200));
  TEST_ASSERT_EQUAL_UINT8(0, stepsToPercent(3200, 3200));
  TEST_ASSERT_EQUAL_UINT8(50, stepsToPercent(1600, 3200));
}

void test_steps_out_of_range_are_clamped() {
  TEST_ASSERT_EQUAL_UINT8(100, stepsToPercent(-500, 3200));
  TEST_ASSERT_EQUAL_UINT8(0, stepsToPercent(9999, 3200));
}

void test_every_percent_round_trips() {
  const int32_t travels[] = {200, 1000, 3200, 12345, 400000};
  for (int32_t travel : travels) {
    for (int p = 0; p <= 100; p++) {
      const int32_t steps = percentToSteps((uint8_t)p, travel);
      TEST_ASSERT_EQUAL_UINT8_MESSAGE((uint8_t)p, stepsToPercent(steps, travel),
                                      "percent did not survive a round trip");
    }
  }
}

void test_more_closed_means_more_steps() {
  int32_t previous = percentToSteps(100, 3200);
  for (int p = 99; p >= 0; p--) {
    const int32_t steps = percentToSteps((uint8_t)p, 3200);
    TEST_ASSERT_GREATER_OR_EQUAL_INT32(previous, steps);
    previous = steps;
  }
}

void test_large_travel_does_not_overflow() {
  // 100 * 40,000,000 overflows int32, so this fails if the math is not 64-bit.
  TEST_ASSERT_EQUAL_INT32(20000000, percentToSteps(50, 40000000));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_open_is_step_zero);
  RUN_TEST(test_closed_is_full_travel);
  RUN_TEST(test_halfway);
  RUN_TEST(test_percent_above_100_is_clamped);
  RUN_TEST(test_steps_to_percent_endpoints);
  RUN_TEST(test_steps_out_of_range_are_clamped);
  RUN_TEST(test_every_percent_round_trips);
  RUN_TEST(test_more_closed_means_more_steps);
  RUN_TEST(test_large_travel_does_not_overflow);
  return UNITY_END();
}