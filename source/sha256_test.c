#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/sha256"
#define IOCTL_SHA256 _IOR('S', 0, uint32_t[8])

int main() {
    int fd;
    uint32_t result[8];  // Array to hold the SHA256 output
    char input_data[1024];  // Buffer to store user input

    printf("Enter string to hash: ");
    if (fgets(input_data, sizeof(input_data), stdin) == NULL) {
        printf("Error reading input.\n");
        return -1;
    }

    input_data[strcspn(input_data, "\n")] = 0;

    fd = open(DEVICE_PATH, O_RDWR);  // Open the SHA256 device
    if (fd < 0) {
        perror("Failed to open the device");
        return -1;
    }

    // Send the input data to the device for hashing and read back the result
    if (ioctl(fd, IOCTL_SHA256, input_data) < 0) {
        perror("Failed to perform ioctl operation");
        close(fd);
        return -1;
    }

    // Read the result using the read method
    if (read(fd, result, sizeof(result)) < 0) {
        perror("Failed to read hash result from device");
        close(fd);
        return -1;
    }

    close(fd);

    // Print the resulting SHA256 hash in hexadecimal
    printf("SHA256 Hash read with the user program: ");
    for (int i = 0; i < 8; i++) {
        printf("%08x", result[i]);
    }
    printf("\n");

    return 0;
}

