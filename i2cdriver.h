#pragma once

#define VERBOSE 0

#define MAX_I2C_ADDRESSES 112

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct i2c *i2c_handle;

typedef struct
{
  char     model[16];    // model string
  char     serial[9];    // Serial number of USB device
  uint64_t uptime;       // time since boot (seconds)
  float    voltage_v;    // USB voltage (Volts)
  float    current_ma;   // device current (mA)
  float    temp_celsius; // temperature (C)
  char     mode;         // I2C 'I' or bitbang 'B' mode
  uint8_t  sda;          // SDA state, 0 or 1
  uint8_t  scl;          // SCL state, 0 or 1
  uint8_t  speed;        // I2C line speed (in kHz)
  uint8_t  pullups;      // pullup state (6 bits, 1=enabled)
  uint16_t ccitt_crc;    // Hardware CCITT CRC
} i2c_status_t;

typedef enum
{
  write_op,
  read_op,
} i2c_rw_t;

typedef enum
{
  kHz100 = 0x31,
  kHz400 = 0x34,
} i2c_speed_t;

bool i2c_connect (i2c_handle *sd, char const *portname);
void i2c_disconnect (i2c_handle sd);
bool i2c_start (i2c_handle sd, uint8_t dev, i2c_rw_t op);
void i2c_stop (i2c_handle sd);
bool i2c_read_buffer (i2c_handle sd, size_t s, uint8_t bytes[s]);
bool i2c_read_register (i2c_handle sd, uint8_t device, uint8_t reg, size_t count, uint8_t buffer[count]);
bool i2c_write_buffer (i2c_handle sd, size_t s, uint8_t const bytes[s]);
bool i2c_write_register (i2c_handle sd, uint8_t device, uint8_t reg, size_t count, uint8_t buffer[count]);
void i2c_get_status (i2c_handle sd, i2c_status_t *status);
bool i2c_reset (i2c_handle sd);
void i2c_scan (i2c_handle sd, uint8_t devices[MAX_I2C_ADDRESSES]);
void i2c_print_info (i2c_handle sd, i2c_status_t *status);
bool i2c_check_crc (i2c_handle sd);
void i2c_set_speed (i2c_handle sd, i2c_speed_t speed);
void i2c_monitor (i2c_handle sd, bool enable);
void i2c_capture (i2c_handle sd);
