struct mock_device_receiver {
  unsigned char received[10];
};

struct mock_device_sender {
  unsigned char to_send[20];
  int write_at;
};

void mock_device_sender_init(struct mock_device_sender *sender);
ssize_t mock_device_read(void *rx_device, void *buf, size_t size);
ssize_t mock_device_write(void *tx_device, const void *buf, size_t size);
ssize_t mock_device_no_read(void *rx_device, void *buf, size_t size);
ssize_t mock_device_no_write(void *tx_device, const void *buf, size_t size);
void fill_checksum(struct mock_device_sender *sender);
