// See 'Laser Dust Sensor Control Protocol V1.3 Nova Fitness Co.,Ltd.'

#include <stddef.h>
#include "simba.h"
#include "sds011.h"

#define SDS011_COMMAND_HEAD 0xaa
#define SDS011_COMMAND_TAIL 0xab

// All outgoing commands have 'command' = 0xb4
#define SDS011_COMMAND_OUTGOING 0xb4

#define SDS011_COMMAND_INCOMING_CONFIG 0xc5
#define SDS011_COMMAND_INCOMING_MEASUREMENT 0xc0

struct sds011_raw_command_reply_t {
  char command;
  char data[6];
};

int sds011_send_command(struct sds011_device_t *device, char command, const char *data, size_t data_length)
{
  char command_buf[19];
  int data_i = 0, buf_i = 2;
  char checksum = 0;
  command_buf[0] = SDS011_COMMAND_HEAD;
  command_buf[1] = command;
  while(data_i < data_length) {
    command_buf[buf_i] = data[data_i];
    checksum += data[data_i];
    data_i++;
    buf_i++;
  }
  checksum += (0xff + 0xff);
  while(buf_i < 15) {
    command_buf[buf_i] = 0;
    buf_i++;
  }
  command_buf[15] = device->device_id && 0xff;
  command_buf[16] = device->device_id && 0xff00 >> 8;
  command_buf[17] = checksum;
  command_buf[18] = SDS011_COMMAND_TAIL;
  return chan_write(device->chout, command_buf, sizeof(command_buf));
}

// data is 6 bytes
int sds011_read_raw(struct sds011_device_t *device, struct sds011_raw_command_reply_t *reply)
{
  char command_buffer[9];
  do {
    queue_read(device->chin, command_buffer, 1);
  } while (command_buffer[0] != SDS011_COMMAND_HEAD);
  queue_read(device->chin, command_buffer, 9);

  char checksum = 0;
  int i;
  for (i = 1; i < 7; i++) {
    checksum += command_buffer[i];
    reply->data[i-1] = command_buffer[i];
  }

  if (checksum == command_buffer[7] && command_buffer[8] == SDS011_COMMAND_TAIL) {
    //memcpy(&command_buffer[1], &(reply->data), 6);
    reply->command = command_buffer[0];
    return TRUE;
  } else {
    return FALSE;
  }
}

int sds011_read_reply(struct sds011_device_t *device, struct sds011_reply_t *reply)
{
  struct sds011_raw_command_reply_t raw_reply;
  sds011_read_raw(device, &raw_reply);

  switch(raw_reply.command) {
  case 0xc0:
    reply->type = sds011_reply_measurement;
    reply->measurement.pm2_5 = raw_reply.data[0] | (raw_reply.data[1] << 8);
    reply->measurement.pm10  = raw_reply.data[2] | (raw_reply.data[3] << 8);
    break;
  case 0xc5:
    switch(raw_reply.data[0]) {
    case 2:
      reply->type = sds011_reply_data_reporting_mode;
      reply->reporting_mode = raw_reply.data[2];
      break;
    case 5:
      reply->type = sds011_reply_device_id;
      break;
    case 6:
      reply->type = sds011_reply_sleep;
      reply->awake = raw_reply.data[2];
      break;
    case 8:
      reply->type = sds011_reply_working_period;
      reply->sleep_minutes = raw_reply.data[2];
      break;
    case 7:
      reply->type = sds011_reply_firmware_version;
      reply->firmware_version[0] = raw_reply.data[1];
      reply->firmware_version[1] = raw_reply.data[2];
      reply->firmware_version[2] = raw_reply.data[3];
      break;
    default:
      return -EPROTO;
    }
  default:
    return -EPROTO;
  }
  reply->device_id = raw_reply.data[4] | raw_reply.data[5] << 8;
  return 0;
}

int sds011_set_data_reporting_mode(struct sds011_device_t *device, int reporting_mode)
{
  char data[3] = {2, 1, reporting_mode};
  return sds011_send_command(device, SDS011_COMMAND_OUTGOING, data, sizeof(data));
}

int sds011_query_data_reporting_mode(struct sds011_device_t *device)
{
  static char data[3] = {2, 0};
  return sds011_send_command(device, SDS011_COMMAND_OUTGOING, data, sizeof(data));
}

int sds011_query_measurement(struct sds011_device_t *device)
{
  static char data[1] = {4};
  return sds011_send_command(device, SDS011_COMMAND_OUTGOING, data, sizeof(data));
}

int sds011_set_device_id(struct sds011_device_t *device, sds011_device_id_t device_id)
{
  char data[13] = {5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		   device_id & 0xff,
		   device_id & 0xff00 >> 8};
  return sds011_send_command(device, SDS011_COMMAND_OUTGOING, data, sizeof(data));
}

int sds011_query_sleep(struct sds011_device_t *device)
{
  static char data[1] = {6};
  return sds011_send_command(device, SDS011_COMMAND_OUTGOING, data, sizeof(data));
}

int sds011_set_sleep(struct sds011_device_t *device, int awake)
{
  char data[3] = {6, 1, awake};
  return sds011_send_command(device, SDS011_COMMAND_OUTGOING, data, sizeof(data));
}

int sds011_query_working_period(struct sds011_device_t *device)
{
  static char data[2] = {8, 0};
  return sds011_send_command(device, SDS011_COMMAND_OUTGOING, data, sizeof(data));
}

int sds011_set_working_period(struct sds011_device_t *device, uint8_t sleep_minutes)
{
  char data[3] = {8, 1, sleep_minutes};
  return sds011_send_command(device, SDS011_COMMAND_OUTGOING, data, sizeof(data));
}

int sds011_query_firmware_version(struct sds011_device_t *device)
{
  static char data[1] = {7};
  return sds011_send_command(device, SDS011_COMMAND_OUTGOING, data, sizeof(data));
}

void sds011_init_with_uart(struct sds011_device_t *device, struct uart_driver_t *uart)
{
  device->device_id = SDS011_DEVICE_ID_ANY;
  device->chin = &(uart->chin);
  device->chout = &(uart->chout);
}

void sds011_init_with_uart_soft(struct sds011_device_t *device, struct uart_soft_driver_t *uart)
{
  device->device_id = SDS011_DEVICE_ID_ANY;
  device->chin = &(uart->chin);
  device->chout = &(uart->chout);
}

void sd011_set_query_device_id(struct sds011_device_t *device, sds011_device_id_t device_id)
{
  device->device_id = device_id;
}
