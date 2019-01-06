#define SDS011_DATA_REPORTING_ACTIVE 0
#define SDS011_DATA_REPORTING_QUERY 1

#define SDS011_DEVICE_ID_ANY 0xffff

// Known as 'sleep' and 'work' in doc
#define SDS011_SLEEP 0
#define SDS011_AWAKE 1

typedef uint16_t sds011_device_id_t;

struct sds011_device_t {
  struct queue_t *chin;
  struct chan_t *chout;
  uint16_t device_id;
};

// ug/m^3 * 10^-1, i.e. 1234 means 123.4 ug/m^3
struct sds011_measurement {
  int pm2_5;
  int pm10;
};

enum sds011_reply_type_t {
  sds011_reply_data_reporting_mode = 2,    // 0xc5, 2
  sds011_reply_measurement = 0xc0,         // 0xc0
  sds011_reply_device_id = 5,              // 0xc5, 5
  sds011_reply_sleep = 6,                  // 0xc5, 6
  sds011_reply_working_period = 8,         // 0xc5, 8
  sds011_reply_firmware_version = 7        // 0xc5, 7
};

struct sds011_reply_t {
  enum sds011_reply_type_t type;
  union {
    int reporting_mode;
    struct sds011_measurement measurement;
    int awake;
    int sleep_minutes;
    char firmware_version[3];
  };
  sds011_device_id_t device_id;
};

void sds011_init_with_uart(struct sds011_device_t *device, struct uart_driver_t *uart);
void sds011_init_with_uart_soft(struct sds011_device_t *device, struct uart_soft_driver_t *uart);

void sd011_set_query_device_id(struct sds011_device_t *device, sds011_device_id_t device_id);

int sds011_set_data_reporting_mode(struct sds011_device_t *device, int reporting_mode);
int sds011_query_data_reporting_mode(struct sds011_device_t *device);
int sds011_query_measurement(struct sds011_device_t *device);
int sds011_set_device_id(struct sds011_device_t *device, sds011_device_id_t device_id);
int sds011_query_sleep(struct sds011_device_t *device);
int sds011_set_sleep(struct sds011_device_t *device, int wake);
int sds011_query_working_period(struct sds011_device_t *device);
int sds011_set_working_period(struct sds011_device_t *device, uint8_t sleep_minutes);
int sds011_query_firmware_version(struct sds011_device_t *device);

int sds011_read_reply(struct sds011_device_t *device, struct sds011_reply_t *reply);
