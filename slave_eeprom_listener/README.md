## eeprom_listener

Sample to use the notification support added to the i2c slave-eeprom backend.
The eeprom_listener receives the i2c-bus and i2c-address of the slave-eeprom
as an argument to register itself (by its pid) as the listener process.
After starting the eeprom_listener will load the contents of the romfile
(passed as a third argument) to the eeprom registers.
After a notification from the backend driver the eeprom_listener will backup
the content of the eeprom into the romfile.

Usage:

	./eeprom_listener <i2c-bus> <addr> <romFile> &

## Build instructions

	$ cd <slave_eeprom_listener>
	$ gcc eeprom_listener.c -o eprom_listener -lrt -lpthread

## Run the listener on SLAVE board

A quick example for instantiating the slave-eeprom driver from userspace at
address 0x64 on bus 1 and run the eeprom_listener to start backing up the 
eeprom content at a romFile.


##### 1. Instantiate the slave-eeprom backend

	echo slave-24c02 0x64 > /sys/bus/i2c/devices/i2c-0/new_device

##### 2. Start the listener

	./eeprom_listener i2c-0 0x64 romFile &

##### 3. On the master device start some reads/writes to the i2c device

On the MASTER device 

 Dump the content of the eeprom

	$ i2cdump -y 0 0x64

 Write the date at address 0xC0

	$ date > dateFile
	$ ./eeprog -i dateFile /dev/i2c-0 0x64 -w 0xc0

 Read the contents again
	
	$ i2cdump -y 0 0x64

On the SLAVE device

 You will see some logs like:

	Backup eeprom content on file...

## Kill the process

You can kill the listener by name

	pkill eeprom_listener
