##1. SHA-256 Cryptocore Device Test 

Now that we have developed the device in the previous steps, we need to test it with some simple functions without even developing the driver. To test a device with BusyBox's devmem write and read functions, you can follow these steps:
        
# Write to the Device:
Use the devmem command to write a value to a specific memory address.

devmem <address> <width> <value>

<address>: The memory address of the device register.
<width>: The data width (8, 16, 32, or 64 bits).
<value>: The value to write.

Example:

devmem 0x40000000 32 0x1

# Read from the Device:
Use the devmem command to read a value from a specific memory address.

devmem <address> <width>

<address>: The memory address of the device register.
<width>: The data width (8, 16, 32, or 64 bits).

Example:

devmem 0x40000000 32

