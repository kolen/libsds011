#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sds011.h"
#include "test_support.h"

void test_parse_measurement(void **state) {
  struct sds011_device_t device;
  struct mock_device_sender sender;
  mock_device_sender_init(&sender);
  sds011_init_with_read_write_fns(&device,
				  mock_device_read,
				  mock_device_no_write,
				  (void*)&sender,
				  NULL);
  static const unsigned char measurement[10] = {
    0xaa, 0xc0,
    0x34, 0x12, 0x78, 0x56,
    0xfe, 0xca,
    0x00, 0xab
  };
  memcpy(sender.to_send, measurement, 10);
  fill_checksum(&sender);
  struct sds011_reply_t reply;
  int result = sds011_read_reply(&device, &reply);
  assert_return_code(result, 0);
  assert_int_equal(reply.type, sds011_reply_measurement);
  assert_int_equal(reply.measurement.pm2_5, 0x1234);
  assert_int_equal(reply.measurement.pm10, 0x5678);
}

void test_parse_data_reporting_mode(void **state) {
  struct sds011_device_t device;
  struct mock_device_sender sender;
  mock_device_sender_init(&sender);
  sds011_init_with_read_write_fns(&device,
				  mock_device_read,
				  mock_device_no_write,
				  (void*)&sender,
				  NULL);
  static const unsigned char measurement[10] = {
    0xaa, 0xc5, 0x02,
    0x00, 0x01,
    0x00,
    0xfe, 0xca,
    0x00, 0xab
  };
  memcpy(sender.to_send, measurement, 10);
  fill_checksum(&sender);
  struct sds011_reply_t reply;
  int result = sds011_read_reply(&device, &reply);
  assert_return_code(result, 0);
  assert_int_equal(reply.type, sds011_reply_data_reporting_mode);
  assert_int_equal(reply.reporting_mode, SDS011_DATA_REPORTING_QUERY);
  assert_int_equal(reply.device_id, 0xcafe);
}

void test_parse_invalid(void **state) {
  struct sds011_device_t device;
  struct mock_device_sender sender;
  mock_device_sender_init(&sender);
  sds011_init_with_read_write_fns(&device,
				  mock_device_read,
				  mock_device_no_write,
				  (void*)&sender,
				  NULL);
  static const unsigned char measurement[10] = {
    0xaa, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xab
  };
  memcpy(sender.to_send, measurement, 10);
  struct sds011_reply_t reply;
  int result = sds011_read_reply(&device, &reply);
  assert_true(result < 0);
}

void test_parse_invalid_c5(void **state) {
  struct sds011_device_t device;
  struct mock_device_sender sender;
  mock_device_sender_init(&sender);
  sds011_init_with_read_write_fns(&device,
				  mock_device_read,
				  mock_device_no_write,
				  (void*)&sender,
				  NULL);
  static const unsigned char measurement[10] = {
    0xaa, 0xc5, 0x09,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xab
  };
  memcpy(sender.to_send, measurement, 10);
  struct sds011_reply_t reply;
  int result = sds011_read_reply(&device, &reply);
  assert_true(result < 0);
}

void test_parse_wait_for_header(void **state) {
  struct sds011_device_t device;
  struct mock_device_sender sender;
  mock_device_sender_init(&sender);
  sds011_init_with_read_write_fns(&device,
				  mock_device_read,
				  mock_device_no_write,
				  (void*)&sender,
				  NULL);
  static const unsigned char measurement[14] = {
    0x01, 0xff, 0x12, 0x02,
    0xaa, 0xc0,
    0x34, 0x12, 0x78, 0x56,
    0xfe, 0xca,
    (unsigned char)(0x34 + 0x12 + 0x78 + 0x56 + 0xfe + 0xca),
    0xab,
  };
  memcpy(sender.to_send, measurement, 14);
  struct sds011_reply_t reply;
  int result = sds011_read_reply(&device, &reply);
  assert_return_code(result, 0);
  assert_int_equal(reply.type, sds011_reply_measurement);
  assert_int_equal(reply.measurement.pm2_5, 0x1234);
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_parse_measurement),
    cmocka_unit_test(test_parse_data_reporting_mode),
    cmocka_unit_test(test_parse_invalid),
    cmocka_unit_test(test_parse_invalid_c5),
    cmocka_unit_test(test_parse_wait_for_header),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
