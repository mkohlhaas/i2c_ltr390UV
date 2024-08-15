#include "error.h"
#include "i2cdriver.h"
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// #define DEVICE 0x1c
#define DEVICE 0x53

// registers from the documentation
#define MAIN_CTRL_REG     0x00
#define RES_MEAS_RATE_REG 0x04
#define GAIN_REG          0x05
#define PART_ID_REG       0x06
#define MAIN_STATUS_REG   0x07
#define UV_DATA_REG       0x10
#define INT_CONF_REG      0x19

typedef enum
{
  ALS_MODE = 0x02,
  UVS_MODE = 0x0A,
} measurement_mode_t;

typedef enum
{
  GAIN_1,
  GAIN_3,
  GAIN_6,
  GAIN_9,
  GAIN_18,
} gain_t;

typedef enum
{
  RES_20_BIT,
  RES_19_BIT,
  RES_18_BIT,
  RES_17_BIT,
  RES_16_BIT,
  RES_13_BIT,
} resolution_t;

// Measurement rate in ms.
typedef enum
{
  RATE_25,
  RATE_50,
  RATE_100,
  RATE_200,
  RATE_500,
  RATE_1000,
  RATE_2000,
} meas_rate_t;

// Sleep for the requested number of milliseconds.
// Returns `true` on success.
bool
msleep (long msec)
{

  if (msec < 0)
    {
      errno = EINVAL;
      return false;
    }

  struct timespec ts = {
    .tv_sec  = msec / 1000,
    .tv_nsec = (msec % 1000) * 1000000,
  };

  int res;
  do
    {
      res = nanosleep (&ts, &ts);
    }
  while (res && errno == EINTR);

  return !res;
}

uint8_t
get_int_conf_reg (i2c_handle sd)
{
  uint8_t int_config = {};
  i2c_read_register (sd, DEVICE, INT_CONF_REG, sizeof int_config, &int_config);
  return int_config;
}

uint8_t
get_status (i2c_handle sd)
{
  uint8_t status = {};
  i2c_read_register (sd, DEVICE, MAIN_STATUS_REG, sizeof status, &status);
  return status;
}

bool
set_int_conf_reg (i2c_handle sd, uint8_t val)
{
  uint8_t buffer[] = { val };
  return i2c_write_register (sd, DEVICE, INT_CONF_REG, sizeof buffer, buffer);
}

uint8_t
get_part_id (i2c_handle sd)
{
  uint8_t part_id = {};
  i2c_read_register (sd, DEVICE, PART_ID_REG, sizeof part_id, &part_id);
  return part_id;
}

bool
set_mode (i2c_handle sd, measurement_mode_t mode)
{
  uint8_t buffer[] = { mode };
  return i2c_write_register (sd, DEVICE, MAIN_CTRL_REG, sizeof buffer, buffer);
}

measurement_mode_t
get_mode (i2c_handle sd)
{
  uint8_t mode = {};
  i2c_read_register (sd, DEVICE, MAIN_CTRL_REG, sizeof mode, &mode);
  return mode;
}

void
print_mode (measurement_mode_t mode)
{
  switch (mode)
    {
    case ALS_MODE:
      printf ("mode: ALS\n");
      break;
    case UVS_MODE:
      printf ("mode: UVS\n");
      break;
    default:
      printf ("Unknown mode: %u\n", mode);
    }
}

bool
set_res_meas_rate (i2c_handle sd, resolution_t resolution, meas_rate_t rate)
{
  uint8_t buffer[] = { resolution << 4 | rate };
  return i2c_write_register (sd, DEVICE, RES_MEAS_RATE_REG, sizeof buffer, buffer);
}

uint8_t
get_res_meas_rate (i2c_handle sd)
{
  uint8_t meas_rate = {};
  i2c_read_register (sd, DEVICE, RES_MEAS_RATE_REG, sizeof meas_rate, &meas_rate);
  return meas_rate;
}

void
print_res_meas_rate (uint8_t res_meas_rate)
{
  uint8_t resolution = res_meas_rate >> 4;
  switch (resolution)
    {
    case RES_16_BIT:
      printf ("resolution: 16 Bit\n");
      break;
    case RES_17_BIT:
      printf ("resolution: 17 Bit\n");
      break;
    case RES_18_BIT:
      printf ("resolution: 18 Bit\n");
      break;
    case RES_19_BIT:
      printf ("resolution: 19 Bit\n");
      break;
    case RES_20_BIT:
      printf ("resolution: 20 Bit\n");
      break;
    default:
      printf ("Unknown resolution: %hhu\n", resolution);
    }

  uint8_t measurement_rate = res_meas_rate & 0x0f;
  switch (measurement_rate)
    {
    case RATE_25:
      printf ("measurement rate: 25ms\n");
      break;
    case RATE_50:
      printf ("measurement rate: 50ms\n");
      break;
    case RATE_100:
      printf ("measurement rate: 100ms\n");
      break;
    case RATE_200:
      printf ("measurement rate: 200ms\n");
      break;
    case RATE_500:
      printf ("measurement rate: 500ms\n");
      break;
    case RATE_1000:
      printf ("measurement rate: 1000ms\n");
      break;
    case RATE_2000:
      printf ("measurement rate: 2000ms\n");
      break;
    default:
      printf ("Unknown measurement rate: %hhu\n", measurement_rate);
    }
}

bool
set_gain (i2c_handle sd, gain_t gain)
{
  uint8_t buffer[] = { gain };
  return i2c_write_register (sd, DEVICE, GAIN_REG, sizeof buffer, buffer);
}

gain_t
get_gain (i2c_handle sd)
{
  uint8_t gain = {};
  i2c_read_register (sd, DEVICE, GAIN_REG, sizeof gain, &gain);
  return gain;
}

void
print_gain (gain_t gain)
{
  switch (gain)
    {
    case GAIN_1:
      printf ("gain: 1\n");
      break;
    case GAIN_3:
      printf ("gain: 3\n");
      break;
    case GAIN_6:
      printf ("gain: 6\n");
      break;
    case GAIN_9:
      printf ("gain: 9\n");
      break;
    case GAIN_18:
      printf ("gain: 18\n");
      break;
    default:
      printf ("Unknown gain: %u\n", gain);
    }
}

uint32_t
read_raw_data (i2c_handle sd)
{
  // uint8_t buffer[3] = {};
  uint8_t data0 = 0;
  uint8_t data1 = 0;
  uint8_t data2 = 0;
  // i2c_read_register (sd, DEVICE, UV_DATA_REG, sizeof buffer, buffer);
  i2c_read_register (sd, DEVICE, UV_DATA_REG + 0, sizeof data0, &data0);
  i2c_read_register (sd, DEVICE, UV_DATA_REG + 1, sizeof data1, &data1);
  i2c_read_register (sd, DEVICE, UV_DATA_REG + 2, sizeof data2, &data2);
  dbgPrint ("%hhu %hhu %hhu\n", data0, data1, data2);
  // uint32_t raw_data = buffer[2] << 16 | buffer[1] << 8 | buffer[0];
  uint32_t raw_data = data2 << 16 | data1 << 8 | data0;
  return raw_data;
}

int
main (int argc, char *argv[])
{
  // check args
  if (argc < 2)
    {
      printf ("Usage: %s <USB-DEVICE-TO-FT230-CHIP>\n", argv[0]);
      exit (EXIT_FAILURE);
    }

  // connect to FT230 board
  i2c_handle sd;
  if (!i2c_connect (&sd, argv[1]))
    {
      logError ("Could not connect to %s\n", argv[1]);
      exit (EXIT_FAILURE);
    }

  // check part id
  auto part_id = get_part_id (sd);
  if (part_id != 0xb2)
    {
      logExit ("Wrong Part ID: %hhu", part_id);
    }

  printf ("part id: 0x%hhx\n", part_id);

  printf ("--------------------\n");

  // -- check settings --
  // MODE
  auto mode = get_mode (sd);
  printf ("mode raw: 0x%hhx\n", mode);
  print_mode (mode);
  printf ("Setting mode to UVS_MODE.\n");
  set_mode (sd, UVS_MODE);
  mode = get_mode (sd);
  print_mode (mode);

  printf ("--------------------\n");

  // GAIN
  auto gain = get_gain (sd);
  printf ("gain raw: %u\n", gain);
  print_gain (gain);
  printf ("Setting gain to 18.\n");
  set_gain (sd, GAIN_18);
  gain = get_gain (sd);
  print_gain (gain);

  printf ("--------------------\n");

  // RESOLUTION & MEASUREMENT RATE
  auto res_meas_rate = get_res_meas_rate (sd);
  print_res_meas_rate (res_meas_rate);
  printf ("Setting resolution to 18 bit and rate to 100ms.\n");
  set_res_meas_rate (sd, RES_18_BIT, RATE_100);
  res_meas_rate = get_res_meas_rate (sd);
  print_res_meas_rate (res_meas_rate);

  printf ("--------------------\n");

  for (int i = 0; i < 10; i++)
    {
      printf ("Status: %hhx\n", get_status (sd));
      printf ("Config Register: %hhx\n", get_int_conf_reg (sd));
      set_int_conf_reg (sd, 0x30);
      printf ("raw data: %u\n", read_raw_data (sd));
      msleep (100);
    }

  // disconnect from FT230 board
  i2c_disconnect (sd);
}
