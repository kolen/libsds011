#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sds011.h"


struct mock_device_receiver {
  unsigned char received[10];
};

struct mock_device_sender {
  unsigned char to_send[20];
  int write_at;
};

void mock_device_sender_init(struct mock_device_sender *sender)
{
  sender->write_at = 0;
}

ssize_t mock_device_read(void *rx_device, void *buf, size_t size)
{
  struct mock_device_sender *reader = (struct mock_device_sender*)rx_device;
  if (reader->write_at + size > 20) {
    fprintf(stderr,
	    "Trying to read too much data, pointer: %d, trying to write: %d",
	    reader->write_at, (int)size);
    abort();
  }
  memcpy(buf, &(reader->to_send[reader->write_at]), size);
  reader->write_at += size;
  return size;
}

ssize_t mock_device_write(void *tx_device, const void *buf, size_t size)
{
  if (size != 10) {
    fprintf(stderr, "Only writes of 10 bytes are supported\n");
    abort();
  }
  memcpy(((struct mock_device_receiver*)tx_device)->received, buf, 10);
  return size;
}

ssize_t mock_device_no_read(void *rx_device, void *buf, size_t size)
{
  fprintf(stderr, "Reading is not supported");
  abort();
}

ssize_t mock_device_no_write(void *tx_device, const void *buf, size_t size)
{
  fprintf(stderr, "Writing is not supported");
  abort();
}

void fill_checksum(struct mock_device_sender *sender) {
  int i;
  unsigned char sum = 0;
  for(i=2; i<=7; i++) { sum += sender->to_send[i]; }
  sender->to_send[8] = sum;
}

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

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_parse_measurement),
    cmocka_unit_test(test_parse_data_reporting_mode),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
