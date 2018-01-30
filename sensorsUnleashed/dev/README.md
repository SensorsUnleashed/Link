# EEProm sensors
This is a module, that will make the board retrieve settings from a i2c attached eeprom.

The setup can contain system infos, log-data and even sensors to setup.


## Configuration options

### sensors
1. Pulse sensors  
GPIO pin
Interrupt
Timers
To be considered

CCP pins. s. 39
Compare: The GPTM is incremented or decremented by programmed events on the CCP input. The GPTM compares the current value with a stored value and generates an interrupt when a match occurs.
  
GPT_0 16bit used for usec delay  
We use GPT_1 16bit for pulse counting

2. GPIO level sensing



## Operation
Will be the same as with the uart sensors.
