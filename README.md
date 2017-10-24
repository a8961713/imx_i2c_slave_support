# imx i2c slave support

Add I2C slave provider using the generic slave interface for i.MX devices

This package provides:
- **kernel patches**:
    1) Add the basic i2c-slave support
    2) User-space Notification support from the slave-eeprom backend
- **eeprom_listener**: Sample for testing the notification support
- **eeprog**: open source i2c-tool for testing eeproms

---

# Install imx i2c slave support
#### Apply kernel patches
 Go to your kernel source directory and apply the patches.

    $ cd <kernel_dir>
    $ git am -3 <imx_i2c_slave_support/kernel_patches>/*.patch

 Rebuild you kernel

    $ make

 Replace zImage on your boot partition device

    $ cp <kernel_dir>/arch/arm/boot/zImage /run/media/<boot_part>/
---

# Test the i2c slave eeprom

You will need to connect a master device (like a Raspberry Pi or other board that acts as the master device) to your i.mx board by connecting the I2C_SDA and I2C_SCL lines between them.

For this example we will instantiate a slave-eeprom backend device with address 0x64 on the i2c bus 0

## On the SLAVE BOARD (i.mx)
Instantiate the slave-eeprom backend

       $ echo slave-24c02 0x64 > /sys/bus/i2c/devices/i2c-0/new_device

## On the MASTER BOARD
Dump the content of the eeprom

        $ i2cdump -y 0 0x64

The output shold look like this

         0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f    0123456789abcdef
    00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
    10: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
    20: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
    30: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................


 Write the date at address 0x10 using eeprog. (Check Section below to see how to build/install eeprog)

        $ cd eeprog-0.7.6-tear12
        $ date > dateFile
        $ ./eeprog -f -i dateFile /dev/i2c-0 0x64 -w 0x10

Read the contents again

        $ i2cdump -y 0 0x64

You should see the content now contains the previuos writing

         0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f    0123456789abcdef
    00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
    10: 54 68 75 20 4f 63 74 20 31 32 20 32 33 3a 34 38    Thu Oct 12 23:48
    20: 3a 30 31 20 55 54 43 20 32 30 31 37 0a 00 00 00    :01 UTC 2017?...
    30: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
---

# Install eeprog on your master board
Copy the eeprog-0.7.6-tear12 directory to your MASTER device and just build it with make

    $ cd eeprog-0.7.6-tear12
    $ make

*NOTE*: A prebuild binary for the eeprog (for arm v7) is already included

---
