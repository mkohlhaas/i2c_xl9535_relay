./i2c-c-template /dev/ttyUSB0 w 32 6,0
./i2c-c-template /dev/ttyUSB0 w 32 7,0
sleep 1
./i2c-c-template /dev/ttyUSB0 w 32 6,255
./i2c-c-template /dev/ttyUSB0 w 32 7,255
