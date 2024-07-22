#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/hw.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "hw/misc/crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#define TYPE_SHA256_DEVICE "sha256_device"
typedef struct SHA256DeviceState SHA256DeviceState;
DECLARE_INSTANCE_CHECKER(SHA256DeviceState, SHA256_DEVICE, TYPE_SHA256_DEVICE)

#define ID_REG      0x0000
#define CTRL_REG    0x0008
#define STATUS_REG  0x000C
#define INPUT_REG   0x0010
#define OUTPUT_REG  0x0410

#define deviceEN            0x00000001
#define DEVICE_ID           0xFEEDCAFE
#define inputBufferSize     1024
#define outputBufferSize    32
#define CHUNK_SIZE          64
#define RIGHT_ROTATE(value, n) (((value) >> (n)) | ((value) << (32 - (n))))

uint32_t digest[8];


struct SHA256DeviceState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    char inputBuffer[inputBufferSize];
    uint32_t outputBuffer[outputBufferSize / 4];
    uint32_t control;
    uint32_t status;
};

static uint64_t sha_device_read(void *opaque, hwaddr addr, unsigned int size) {
    SHA256DeviceState *s = (SHA256DeviceState *)opaque;

    switch (addr) {
        case ID_REG:
            return DEVICE_ID;

        case CTRL_REG:
            return s->control;

        case STATUS_REG:
            return s->status;

        default:
            if (addr >= INPUT_REG && addr < INPUT_REG + inputBufferSize) {
                int offset = addr - INPUT_REG;
                return s->inputBuffer[offset];
            } else if (addr >= OUTPUT_REG && addr < OUTPUT_REG + outputBufferSize) {
                int offset = (addr - OUTPUT_REG) / 4;
                return s->outputBuffer[offset];
            } else {
                qemu_log_mask(LOG_GUEST_ERROR, "sha_device_read: Invalid read address 0x%08x\n", (int)addr);
                return 0xDEADBEEF;
            }
    }

    return 0;
}

static void sha_device_write(void *opaque, hwaddr addr, uint64_t data, unsigned int size) {
    SHA256DeviceState *s = (SHA256DeviceState *)opaque;

    switch (addr) {
        case CTRL_REG:
            s->control = data;
            if (data == deviceEN) {
                perform_sha256_hashing(s->inputBuffer);
                memcpy(s->outputBuffer, digest, outputBufferSize);
                s->status = 1;
            } else if (data == 0) {
                s->status = 0;
            }
            return;
    }

    if (addr >= INPUT_REG && addr < INPUT_REG + inputBufferSize) {
        int offset = addr - INPUT_REG;
        switch (size) {
        
               default:
               
        // Handle unexpected sizes here, for better data allignment :) 
        // fill only up to the buffer limit
        for (int i = 0; i < size && i < sizeof(s->inputBuffer) - offset; i++) {
            s->inputBuffer[offset + i] = (data >> (8 * i)) & 0xFF;
        }
        
                break;
        }
        return;
    } else {
        qemu_log_mask(LOG_GUEST_ERROR, "sha_device_write: Invalid write address 0x%08x\n", (int)addr);
    }
}

static const MemoryRegionOps sha_device_ops = {
    .read = sha_device_read,
    .write = sha_device_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void sha_instance_init(Object *obj) {
    SHA256DeviceState *s = SHA256_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &sha_device_ops, s, "sha256_device", 0x1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->iomem);

    s->status = 0;
    s->control = 0;
    memset(s->inputBuffer, 0, inputBufferSize);
    memset(s->outputBuffer, 0, outputBufferSize);
}

static TypeInfo sha256_device_info = {
    .name = TYPE_SHA256_DEVICE,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(SHA256DeviceState),
    .instance_init = sha_instance_init,
};

static void sha256_device_register_types(void) {
    type_register_static(&sha256_device_info);
}

type_init(sha256_device_register_types)

DeviceState *sha_device_create(hwaddr addr) {
    DeviceState *dev = qdev_new(TYPE_SHA256_DEVICE);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, addr);

    return dev;
}


int perform_sha256_hashing(char *inputStr) {
    size_t inStrSize = strlen(inputStr);

    unsigned char* messageBlock;
    int numBlocks;
    int messageBlockSize;
    int numChunks;
    unsigned char** chunks;
    int chunkIndex = 0;
    uint32_t w[64];

    uint32_t hashVal[8] = {
        0x6a09e667,
        0xbb67ae85,
        0x3c6ef372,
        0xa54ff53a,
        0x510e527f,
        0x9b05688c,
        0x1f83d9ab,
        0x5be0cd19
    };

    messageBlockSize = ((inStrSize * 8) + 72 + 511);
    numBlocks = messageBlockSize / 512;
    messageBlockSize = (numBlocks * 512) / 8;

    messageBlock = (unsigned char*)malloc(messageBlockSize * sizeof(unsigned char));
    if (messageBlock == NULL) {
        printf("Memory allocation failed for message block.\n");
        return 1;
    }

    memset(messageBlock, 0, messageBlockSize * sizeof(unsigned char));
    encodeMessageBlock(inputStr, messageBlock, inStrSize, messageBlockSize);

    numChunks = (messageBlockSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
    chunks = (unsigned char**)malloc(numChunks * sizeof(unsigned char*));
    if(chunks == NULL) {
        printf("Memory allocation failed for chunks.\n");
        free(messageBlock);
        return 1;
    }

    for (int i = 0; i < numChunks; ++i) {
        chunks[i] = (unsigned char*)malloc(CHUNK_SIZE * sizeof(unsigned char));
        if (chunks[i] == NULL) {
            printf("Memory allocation failed for chunk %d.\n", i);
            for (int j = 0; j < i; ++j) {
                free(chunks[j]);
            }
            free(chunks);
            free(messageBlock);
            return 1;
        }
        memset(chunks[i], 0, CHUNK_SIZE * sizeof(unsigned char));
    }

    for (int i = 0; i < messageBlockSize; ++i) {
        chunks[chunkIndex][i % CHUNK_SIZE] = messageBlock[i];
        if ((i + 1) % CHUNK_SIZE == 0) {
            chunkIndex++;
        }
    }

    for(int  i = 0; i < numChunks; ++i){
        messageSchedule(i, chunks, numChunks, w);
        compression(hashVal, w);
    }

    for (int i = 0; i < 8; ++i) {
        digest[i] = hashVal[i];
    }

    free(messageBlock);
    for (int i = 0; i < numChunks; ++i) {
        free(chunks[i]);
    }
    free(chunks);

    return 0;
}

void encodeMessageBlock(char* inStr, unsigned char messageBlock[], const int inSize, const int messageBlockSize) {
    int index = 0;
    int binaryDigit = 0;
    int ascii = 0;

    for(int i = 0; inStr[i] != '\0'; ++i) {
        ascii = inStr[i];
        for (int j = 7; j >= 0; --j) {
            binaryDigit = (ascii >> j) & 1;
            messageBlock[index / 8] |= (binaryDigit << (7- (index % 8)));
            index++;
        }
    }

    messageBlock[index / 8] |= (1 << (7 - (index % 8)));
    uint64_t length = (uint64_t)inSize * 8;
    int lengthIndex = (messageBlockSize - 8);

    for (int i = 0; i < 8; ++i) {
        unsigned char byte = (length >> ((7 - i) * 8)) & 0xFF;
        messageBlock[lengthIndex + i] = byte;
    }
}

void messageSchedule(int chunkIndex, unsigned char** chunks, int numChunks, uint32_t w[]) {
    for (int i = 0; i < 16; ++i) {
        w[i] = ((uint32_t)chunks[chunkIndex][i * 4] << 24) |
               ((uint32_t)chunks[chunkIndex][i * 4 + 1] << 16) |
               ((uint32_t)chunks[chunkIndex][i * 4 + 2] << 8) |
               (uint32_t)chunks[chunkIndex][i * 4 + 3];
    }

    for (int i = 16; i < 64; ++i) {
        uint32_t sigma0 = RIGHT_ROTATE(w[i-15], 7) ^ RIGHT_ROTATE(w[i-15], 18) ^ (w[i-15] >> 3);
        uint32_t sigma1 = RIGHT_ROTATE(w[i-2], 17) ^ RIGHT_ROTATE(w[i-2], 19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + sigma0 + w[i-7] + sigma1;
    }
}

void compression(uint32_t hashVal[], uint32_t w[]) {
    const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
        0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
        0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
        0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
        0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
        0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
        0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
        0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint32_t a = hashVal[0];
    uint32_t b = hashVal[1];
    uint32_t c = hashVal[2];
    uint32_t d = hashVal[3];
    uint32_t e = hashVal[4];
    uint32_t f = hashVal[5];
    uint32_t g = hashVal[6];
    uint32_t h = hashVal[7];

    uint32_t temp1, temp2, sumA, sumE, choice, majority;

    for (int i = 0; i < 64; ++i) {
        sumA = RIGHT_ROTATE(e, 6) ^ RIGHT_ROTATE(e, 11) ^ RIGHT_ROTATE(e, 25);
        choice = (e & f) ^ (~e & g);
        temp1 = h + sumA + choice + k[i] + w[i];

        sumE = RIGHT_ROTATE(a, 2) ^ RIGHT_ROTATE(a, 13) ^ RIGHT_ROTATE(a, 22);
        majority = (a & b) ^ (a & c) ^ (b & c);
        temp2 = sumE + majority;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    hashVal[0] += a;
    hashVal[1] += b;
    hashVal[2] += c;
    hashVal[3] += d;
    hashVal[4] += e;
    hashVal[5] += f;
    hashVal[6] += g;
    hashVal[7] += h;
}


