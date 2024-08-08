#include "i2cdriver.h"
#include "error.h"
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

struct i2c
{
  int      port;        // port
  uint16_t e_ccitt_crc; // Host CCITT crc, should match hardware crc
  int      dummy;
};

// Returns a random integer between `min` and `max`.
static int
random_int (int min, int max)
{
  auto rnd = rand () % (max - min + 1);
  return min + rnd;
}

// ****************************   Serial port  ********************************

// Opens a terminal connection to the FT230 chip.
// Returns a file handle or `-1` if unsuccesful.
static int
openTerminalToFT230 (const char *portname)
{
  int fd = open (portname, O_RDWR | O_NOCTTY);
  if (fd == -1)
    {
      perror (portname);
      return -1;
    }
  struct termios term_settings;
  tcgetattr (fd, &term_settings);

  cfsetispeed (&term_settings, B1000000);
  cfsetospeed (&term_settings, B1000000);

  cfmakeraw (&term_settings);
  term_settings.c_cc[VMIN] = 1;
  if (tcsetattr (fd, TCSANOW, &term_settings) != 0)
    {
      perror ("Serial settings");
      return -1;
    }

  return fd;
}

// Reads `bytes` from FT230 chip.
// Returns number of bytes read.
static size_t
readFromFT230 (int fd, uint8_t *bytes, size_t s)
{
  ssize_t n;
  size_t  t = 0;
  while (t < s)
    {
      n = read (fd, bytes + t, s);
      if (n > 0)
        {
          t += n;
        }
    }

#if VERBOSE
  printf ("READ %zu %zd: ", s, n);
  for (size_t i = 0; i < s; i++)
    {
      printf ("%02x ", 0xff & bytes[i]);
    }
  printf ("\n");
#endif

  return s;
}

// Writes `bytes` to FT230 chip.
static void
writeToFT230 (int fd, const uint8_t *bytes, size_t s)
{
  write (fd, bytes, s);

#if VERBOSE
  printf ("WRITE %zu: ", s);
  for (size_t i = 0; i < s; i++)
    {
      printf ("%02x ", 0xff & bytes[i]);
    }
  printf ("\n");
#endif
}

// Closes terminal connection to FT230 chip.
static void
closeSerialPort (int hSerial)
{
  close (hSerial);
}

// ******************************  CCITT CRC  *********************************

// CRC checksum table.
static const uint16_t crc_table[256]
    = { 0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad,
        0xe1ce, 0xf1ef, 0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b, 0xa35a,
        0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b,
        0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d, 0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
        0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861,
        0x2802, 0x3823, 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
        0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 0x6ca6, 0x7c87,
        0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
        0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
        0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3,
        0x5004, 0x4025, 0x7046, 0x6067, 0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1, 0x1290,
        0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
        0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e,
        0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634, 0xd94c, 0xc96d, 0xf90e, 0xe92f,
        0x99c8, 0x89e9, 0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c,
        0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
        0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83,
        0x1ce0, 0x0cc1, 0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
        0x2e93, 0x3eb2, 0x0ed1, 0x1ef0 };

// Update CRC checksum.
static void
crc_update (i2c_handle sd, const uint8_t *data, size_t data_len)
{
  unsigned int tbl_idx;
  uint16_t     crc = sd->e_ccitt_crc;

  while (data_len--)
    {
      tbl_idx = ((crc >> 8) ^ *data) & 0xff;
      crc     = (crc_table[tbl_idx] ^ (crc << 8)) & 0xffff;
      data++;
    }
  sd->e_ccitt_crc = crc;
}

// ******************************  I2CDriver  *********************************

void
i2c_print_info (i2c_handle sd, i2c_status_t *status)
{
  fprintf (stdout, "connected:    %s\n", sd->port != -1 ? "true" : "false");
  fprintf (stdout, "model:        %s\n", status->model);
  fprintf (stdout, "serial:       %s\n", status->serial);
  fprintf (stdout, "uptime:       %lu seconds\n", status->uptime);
  fprintf (stdout, "voltage:      %.3fV\n", status->voltage_v);
  fprintf (stdout, "current:      %.1fmA\n", status->current_ma);
  fprintf (stdout, "temperature:  %.1f°C\n", status->temp_celsius);
  fprintf (stdout, "mode:         %s\n", status->mode == 'I' ? "I2C" : "BitBang");
  fprintf (stdout, "sda:          %hhu\n", status->sda);
  fprintf (stdout, "sdl:          %hhu\n", status->scl);
  fprintf (stdout, "i2c bus free: %s\n", status->sda && status->scl ? "true" : "false");
  fprintf (stdout, "speed:        %hhukHz\n", status->speed);
  fprintf (stdout, "pullups:      %hhu\n", status->pullups);
  fprintf (stdout, "hardware crc: 0x%hx\n", status->ccitt_crc);
  fprintf (stdout, "host crc:     0x%hx\n", sd->e_ccitt_crc);
}

// Echo tests connection to FT230.
// Write `byte` to FT230 and immediately read from FT230.
// Returns `true` if both values are the same, otherwise `false`.
static bool
echo_test (i2c_handle sd, uint8_t byte)
{
  // echo command
  uint8_t tx[] = {
    'e',
    byte,
  };

  // write
  writeToFT230 (sd->port, tx, sizeof tx);
  // read
  uint8_t rx;
  readFromFT230 (sd->port, &rx, sizeof rx);
  // compare
  if (rx == byte)
    {
      return true;
    }
  else
    {
      return false;
    }
}

// Connect to `FT230` chip via USB.
// `portname` is the name of the USB port.
// Returns `true` on successful connection, otherwise `false`.
bool
i2c_connect (i2c_handle *sd, char const *portname)
{
  *sd = malloc (sizeof (**sd));
  if (!(*sd))
    {
      logExit ("out of memory");
    }
  // open terminal to FT230
  (*sd)->port = openTerminalToFT230 (portname);
  if ((*sd)->port == -1)
    {
      return false;
    }

  // randomly test terminal connection
  if (!echo_test (*sd, random_int (0, 255)))
    {
      return false;
    }

  // update I2CDriver information if valid connection
  i2c_status_t status;
  i2c_get_status (*sd, &status);
  (*sd)->e_ccitt_crc = status.ccitt_crc; // initialize host crc checksum

  return true;
}

void
i2c_disconnect (i2c_handle sd)
{
  if (sd->port != -1)
    {
      closeSerialPort (sd->port);
    }
  sd->port = -1;
}

// Send `byte` to FT230.
static void
byte_command (i2c_handle sd, uint8_t byte)
{
  writeToFT230 (sd->port, &byte, 1);
}

// Check acknowledge of last i2c operation.
// Returns `true` if acknowleged, otherwise `false`.
static bool
check_ack (i2c_handle sd)
{
  uint8_t byte;
  readFromFT230 (sd->port, &byte, sizeof byte);
  return byte & 1;
}

void
i2c_set_speed (i2c_handle sd, i2c_speed_t speed)
{
  byte_command (sd, speed);
}

// Update current status of `sd`.
void
i2c_get_status (i2c_handle sd, i2c_status_t *status)
{
  uint8_t const response_size = 80;
  uint8_t       readbuffer[response_size + 1];
  readbuffer[response_size] = 0;

  byte_command (sd, '?');
  readFromFT230 (sd->port, readbuffer, response_size);

  sscanf ((char *)readbuffer, "[%15s %8s %" SCNu64 "%f %f %f %c %" SCNu8 "%" SCNu8 "%" SCNu8 "%" SCNu8 "%" SCNx16 "]",
          status->model, status->serial, &status->uptime, &status->voltage_v, &status->current_ma,
          &status->temp_celsius, &status->mode, &status->sda, &status->scl, &status->speed, &status->pullups,
          &status->ccitt_crc);
}

// Scan i2c bus for devices.
void
i2c_scan (i2c_handle sd, uint8_t devices[MAX_I2C_ADDRESSES])
{
  byte_command (sd, 'd');
  readFromFT230 (sd->port, devices + 8, MAX_I2C_ADDRESSES);
}

// Resets I2C bus.
// Returns `true` if bus is free, otherwise `false`.
bool
i2c_reset (i2c_handle sd)
{
  byte_command (sd, 'x');
  uint8_t sda_scl;
  readFromFT230 (sd->port, &sda_scl, 1);
  return ((sda_scl >> 1) & 1) && (sda_scl & 1);
}

// Starts a read or write operation.
// Returns `true` if operation has been acknowleged.
bool
i2c_start (i2c_handle sd, uint8_t dev, i2c_rw_t op)
{
  uint8_t start[2] = {
    's',
    (dev << 1) | op,
  };
  writeToFT230 (sd->port, start, sizeof start);
  return check_ack (sd);
}

// Stop the current i2c operation.
void
i2c_stop (i2c_handle sd)
{
  byte_command (sd, 'p');
}

// Returns true if hardware and host crc are the same.
bool
i2c_check_crc (i2c_handle sd)
{
  i2c_status_t status;
  i2c_get_status (sd, &status);
  return sd->e_ccitt_crc == status.ccitt_crc;
}

#define MAX_RW_SIZE 64

// Writes `buffer` to FT230.
bool
i2c_write_buffer (i2c_handle sd, size_t s, uint8_t const buffer[s])
{
  for (size_t i = 0; i < s; i += MAX_RW_SIZE)
    {
      size_t rest = s - i;
      size_t len  = rest < MAX_RW_SIZE ? rest : MAX_RW_SIZE;

      uint8_t cmd[MAX_RW_SIZE + 1] = {
        0xc0 + len - 1,
      };
      memcpy (cmd + 1, buffer + i, len);
      writeToFT230 (sd->port, cmd, len + 1);
      if (!check_ack (sd))
        {
          return false;
        }
    }
  crc_update (sd, buffer, s);
  return true;
}

bool
i2c_read_buffer (i2c_handle sd, size_t s, uint8_t buffer[s])
{
  for (size_t i = 0; i < s; i += MAX_RW_SIZE)
    {
      size_t  rest = s - i;
      size_t  len  = rest < MAX_RW_SIZE ? rest : MAX_RW_SIZE;
      uint8_t cmd  = 0x80 + len - 1;
      writeToFT230 (sd->port, &cmd, 1);
      size_t bytes_read = readFromFT230 (sd->port, buffer + i, len);
      if (bytes_read != len)
        {
          return false;
        }
      crc_update (sd, buffer + i, len);
    }
  return true;
}

// Reads from `device` at `address` `count` bytes into `buffer`.
// Returns `true` if read was succesful.
bool
i2c_read_register (i2c_handle sd, uint8_t device, uint8_t address, size_t count, uint8_t buffer[count])
{
  byte_command (sd, 'r');
  uint8_t cmd[] = { device, address, count };
  writeToFT230 (sd->port, cmd, sizeof cmd);
  size_t bytes_read = readFromFT230 (sd->port, buffer, count);
  if (bytes_read != count)
    {
      return false;
    }
  crc_update (sd, buffer, count);
  return true;
}

void
i2c_monitor (i2c_handle sd, bool enable)
{
  byte_command (sd, enable ? 'm' : ' ');
}

void
i2c_capture (i2c_handle sd)
{
  printf ("Capture started\n");
  byte_command (sd, 'c');
  uint8_t byte;
  int     starting = 0;
  int     nbits    = 0;
  int     bits     = 0;

  while (true)
    {
      readFromFT230 (sd->port, &byte, 1);
      for (int i = 0; i < 2; i++)
        {
          int symbol = (i == 0) ? (byte >> 4) : (byte & 0xf);
          switch (symbol)
            {
            case 0:
              break;
            case 1:
              starting = 1;
              break;
            case 2:
              printf ("STOP\n");
              starting = 1;
              break;
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
              bits = (bits << 3) | (symbol & 7);
              nbits += 3;
              if (nbits == 9)
                {
                  int b8 = (bits >> 1), ack = !(bits & 1);
                  if (starting)
                    {
                      starting = 0;
                      printf ("START %02x %s", b8 >> 1, (b8 & 1) ? "READ" : "WRITE");
                    }
                  else
                    {
                      printf ("BYTE %02x", b8);
                    }
                  printf (" %s\n", ack ? "ACK" : "NAK");
                  nbits = 0;
                  bits  = 0;
                }
            }
        }
    }
}
