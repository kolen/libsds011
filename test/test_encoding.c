#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sds011.h"
#include "test_support.h"

void test_encode_query_measurement()
{
  struct sds011_device_t device;
  struct mock_device_receiver receiver;
  sds011_init_with_read_write_fns(&device,
				  mock_device_no_read,
				  mock_device_write,
				  NULL,
				  (void*)&receiver);
  int result = sds011_query_measurement(&device);
  assert_return_code(result, 0);

  static const unsigned char expected_packet[19] = {
    0xaa, 0xb4, 0x04,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0xff, 0xff,
    (unsigned char)(0x04 + 0xff + 0xff),
    0xab
  };
  assert_memory_equal(expected_packet, receiver.received, 19);
}

void test_encode_device_id()
{
  struct sds011_device_t device;
  struct mock_device_receiver receiver;
  sds011_init_with_read_write_fns(&device,
				  mock_device_no_read,
				  mock_device_write,
				  NULL,
				  (void*)&receiver);
  sd011_set_query_device_id(&device, 0xdead);
  int result = sds011_query_sleep(&device);
  assert_return_code(result, 0);

  assert_int_equal(receiver.received[15], (int)0xad);
  assert_int_equal(receiver.received[16], (int)0xde);
}


int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_encode_query_measurement),
    cmocka_unit_test(test_encode_device_id),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
