# 1. SHA-256 Cryptocore Device Test 

Now that we have developed the device in the previous steps, we need to test it with some simple functions without even developing the driver. To test a device with BusyBox's devmem write and read functions, you can follow these steps:
        
### Write to the Device:
Use the devmem command to write a value to a specific memory address.

devmem address width value

address: The memory address of the device register.
width: The data width (8, 16, 32, or 64 bits).
value: The value to write.

Example:

        devmem 0x4000010 32 0x1

### Read from the Device:
Use the devmem command to read a value from a specific memory address.

devmem address width

address: The memory address of the device register.
width: The data width (8, 16, 32, or 64 bits).

Example:

        devmem 0x4000010 32

We wrote the text 'firat kagitci' in hex version to input region of the device which is 0x4000010, then to enable the device we wrote 0x1 to the enable register 0x4000008, as soon as we enable the device, it gave us the output result which was a correct one (that print was for the debug code inside the crypto core). Finally veried the result with an online SHA256 tool.

![image](https://github.com/user-attachments/assets/b31b856e-db9f-4c7d-9f45-e243978a3f31)






