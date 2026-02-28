#include <Arduino.h>
#include <unity.h>

#include "ConfigData.h"

void test_servo_angle_order_is_safe() {
  TEST_ASSERT_TRUE(SERVO_OPEN_ANGLE < SERVO_PREOFF_ANGLE);
  TEST_ASSERT_TRUE(SERVO_PREOFF_ANGLE < SERVO_CLOSED_ANGLE);
}

void test_config_identity_constants() {
  TEST_ASSERT_EQUAL_HEX32(0xCAFEBAC4, CONFIG_MAGIC);
  TEST_ASSERT_TRUE(CONFIG_VERSION >= 1);
}

void test_power_mode_enum_layout() {
  TEST_ASSERT_EQUAL_INT(0, MODE_ACTIVE);
  TEST_ASSERT_EQUAL_INT(1, MODE_LOW_POWER);
  TEST_ASSERT_EQUAL_INT(2, MODE_DEEP_SLEEP);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_servo_angle_order_is_safe);
  RUN_TEST(test_config_identity_constants);
  RUN_TEST(test_power_mode_enum_layout);
  UNITY_END();
}

void loop() {}
