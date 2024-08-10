#include "error.h"
#include "i2cdriver.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEVICE 0x20

#define XL9535_INPUT_PORT     0x00
#define XL9535_OUTPUT_PORT    0x02
#define XL9535_INVERSION_PORT 0x04
#define XL9535_CONFIG_PORT    0x06

#define RELAY_0  (1 << 0)
#define RELAY_1  (1 << 1)
#define RELAY_2  (1 << 2)
#define RELAY_3  (1 << 3)
#define RELAY_4  (1 << 4)
#define RELAY_5  (1 << 5)
#define RELAY_6  (1 << 6)
#define RELAY_7  (1 << 7)
#define RELAY_8  (1 << 8)
#define RELAY_9  (1 << 9)
#define RELAY_10 (1 << 10)
#define RELAY_11 (1 << 11)
#define RELAY_12 (1 << 12)
#define RELAY_13 (1 << 13)
#define RELAY_14 (1 << 14)
#define RELAY_15 (1 << 15)

void
print_output_config_ports (i2c_handle sd)
{
  uint8_t buffer[2] = {};
  dbgPrint ("%hhu %hhu\n", buffer[0], buffer[1]);
  i2c_read_register (sd, DEVICE, XL9535_OUTPUT_PORT, sizeof buffer, buffer);
  dbgPrint ("0x%hhx 0x%hhx\n", buffer[0], buffer[1]);
  i2c_read_register (sd, DEVICE, XL9535_CONFIG_PORT, sizeof buffer, buffer);
  dbgPrint ("0x%hhx 0x%hhx\n", buffer[0], buffer[1]);
}

void
inversion_off (i2c_handle sd)
{
  dbgPrint ("inversion off\n");
  uint8_t buffer[] = { 0x0, 0x0 };
  i2c_write_register (sd, DEVICE, XL9535_INVERSION_PORT, sizeof buffer, buffer);
}

void
enable_circuits (i2c_handle sd)
{
  dbgPrint ("enable circuits\n");
  uint8_t buffer[] = { 0x0, 0x0 };
  i2c_write_register (sd, DEVICE, XL9535_CONFIG_PORT, sizeof buffer, buffer);
}

void
disable_circuits (i2c_handle sd)
{
  dbgPrint ("disable circuits\n");
  uint8_t buffer[] = { 0xff, 0xff };
  i2c_write_register (sd, DEVICE, XL9535_CONFIG_PORT, sizeof buffer, buffer);
}

void
switch_on_relays (i2c_handle sd)
{
  uint8_t buffer[] = { 0xff, 0xff };
  i2c_write_register (sd, DEVICE, XL9535_OUTPUT_PORT, sizeof buffer, buffer);
}

void
switch_off_relays (i2c_handle sd)
{
  uint8_t buffer[] = { 0x0, 0x0 };
  i2c_write_register (sd, DEVICE, XL9535_OUTPUT_PORT, sizeof buffer, buffer);
}

void
switch_on_relay (i2c_handle sd, uint16_t relay)
{
  uint8_t buffer[2] = {};
  i2c_read_register (sd, DEVICE, XL9535_OUTPUT_PORT, sizeof buffer, buffer);
  uint16_t status = (buffer[0] + (buffer[1] << 8)) | relay;
  buffer[0]       = status & 0xff;
  buffer[1]       = status >> 8;
  i2c_write_register (sd, DEVICE, XL9535_OUTPUT_PORT, sizeof buffer, buffer);
}

void
switch_off_relay (i2c_handle sd, uint16_t relay)
{
  uint8_t buffer[2] = {};
  i2c_read_register (sd, DEVICE, XL9535_OUTPUT_PORT, sizeof buffer, buffer);
  uint16_t status = (buffer[0] + (buffer[1] << 8)) & (~relay);
  buffer[0]       = status & 0xff;
  buffer[1]       = status >> 8;
  i2c_write_register (sd, DEVICE, XL9535_OUTPUT_PORT, sizeof buffer, buffer);
}

void
test_relays_0 (i2c_handle sd)
{
  switch_on_relay (sd, RELAY_0 | RELAY_2 | RELAY_4 | RELAY_6 | RELAY_8 | RELAY_10 | RELAY_12 | RELAY_14);
  sleep (1);
  switch_on_relay (sd, RELAY_1 | RELAY_3 | RELAY_5 | RELAY_7 | RELAY_9 | RELAY_11 | RELAY_13 | RELAY_15);
  sleep (1);
  switch_off_relays (sd);
}

void
test_relays_1 (i2c_handle sd)
{
  switch_off_relays (sd);
  for (uint16_t i = 0; i < 16; i++)
    {
      switch_on_relay (sd, 1 << i);
      sleep (1);
    }
  for (uint16_t i = 0; i < 16; i++)
    {
      switch_off_relay (sd, 1 << i);
      sleep (1);
    }
}

void
init_relay_board (i2c_handle sd)
{
  inversion_off (sd);
  enable_circuits (sd);
}

void
shutdown_relay_board (i2c_handle sd)
{
  disable_circuits (sd);
  i2c_disconnect (sd);
}

int
main (int argc, char *argv[])
{
  if (argc < 2)
    {
      printf ("Need USB device!");
      exit (EXIT_FAILURE);
    }

  i2c_handle sd;
  if (!i2c_connect (&sd, argv[1]))
    {
      exit (EXIT_FAILURE);
    }

  init_relay_board (sd);

  // tests
  test_relays_0 (sd);
  sleep (1);
  test_relays_1 (sd);

  shutdown_relay_board (sd);
}
