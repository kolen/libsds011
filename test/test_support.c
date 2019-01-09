#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "test_support.h"

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
