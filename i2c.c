#include "i2cdriver.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
print_usage ()
{
  printf ("Commands are:");
  printf ("\n");
  printf ("  i              display status information (uptime, voltage, current, temperature)\n");
  printf ("  x              I2C bus reset\n");
  printf ("  d              device scan\n");
  printf ("  w dev <bytes>  write bytes to I2C device dev\n");
  printf ("  p              send a STOP\n");
  printf ("  r dev N        read N bytes from I2C device dev, then STOP\n");
  printf ("  m              enter I2C bus monitor mode\n");
  printf ("  c              enter I2C bus capture mode\n");
}

static int
i2c_commands (i2c_handle sd, int argc, char *argv[])
{
  for (int i = 0; i < argc; i++)
    {
      char *token = argv[i];
      if (strlen (token) != 1)
        {
          goto badcommand;
        }

      switch (token[0])
        {
        case 'i':
          i2c_status_t status;
          i2c_get_status (sd, &status);
          i2c_print_info (sd, &status);
          break;

        case 'x':
          {
            printf ("I2C bus is free: %s\n", i2c_reset (sd) ? "true" : "false");
          }
          break;

        case 'd':
          {
            uint8_t devices[MAX_I2C_ADDRESSES];
            i2c_scan (sd, devices);
            for (int i = 0; i < MAX_I2C_ADDRESSES; i++)
              {
                if (devices[i] == '1')
                  {
                    printf ("%02x  ", i);
                  }
                else
                  {
                    printf ("--  ");
                  }
                if ((i % 8) == 7)
                  {
                    printf ("\n");
                  }
              }
          }
          break;

        case 'w':
          {
            token            = argv[++i];
            unsigned int dev = strtol (token, NULL, 0);

            token = argv[++i];
            uint8_t bytes[8192];
            char   *endptr = token;
            size_t  count  = 0;
            while (count < sizeof (bytes))
              {
                bytes[count++] = strtol (endptr, &endptr, 0);
                if (*endptr == '\0')
                  {
                    break;
                  }
                if (*endptr != ',')
                  {
                    fprintf (stderr, "Invalid bytes '%s'\n", token);
                    return 1;
                  }
                endptr++;
              }

            i2c_start (sd, dev, write_op);
            i2c_write_buffer (sd, count, bytes);
          }
          break;

        case 'r':
          {
            token            = argv[++i];
            unsigned int dev = strtol (token, NULL, 0);

            token      = argv[++i];
            size_t  nn = strtol (token, NULL, 0);
            uint8_t bytes[8192];

            i2c_start (sd, dev, read_op);
            i2c_read_buffer (sd, nn, bytes);
            i2c_stop (sd);

            size_t i;
            for (i = 0; i < nn; i++)
              {
                printf ("%s0x%02x", i ? "," : "", 0xff & bytes[i]);
              }
            printf ("\n");
          }
          break;

        case 'p':
          i2c_stop (sd);
          break;

        case 'm':
          {
            char line[100];

            i2c_monitor (sd, true);
            printf ("[Hit return to exit monitor mode]\n");
            fgets (line, sizeof (line) - 1, stdin);
            i2c_monitor (sd, false);
          }
          break;

        case 'c':
          {
            i2c_capture (sd);
          }
          break;

        default:
        badcommand:
          print_usage ();
          return EXIT_FAILURE;
        }
    }

  return EXIT_SUCCESS;
}

int
main (int argc, char *argv[])
{
  if (argc < 3)
    {
      print_usage ();
      exit (EXIT_FAILURE);
    }

  i2c_handle i2c;
  if (!i2c_connect (&i2c, argv[1]))
    {
      exit (EXIT_FAILURE);
    }

  return i2c_commands (i2c, argc - 2, argv + 2);
}
