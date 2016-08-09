#include <stdlib.h>
#include <platform/platform.h>
#include <platform/bcm28xx.h>
#include <string.h>
#include <stdio.h>

#define MAILBOX_REGS_BASE (ARMCTRL_0_SBM_BASE + 0x80)

void* MemoryAllocate(u32 length) {
    return malloc(length);
}

void MemoryDeallocate(void* address) {
    free(address);
}

void* MemoryReserve(u32 length, void* physicalAddress) {
    return physicalAddress + BCM_PERIPH_BASE_VIRT;
}

void MemoryCopy(void* destination, void* source, u32 length) {
    memcpy(destination, source, length);
}

void LogPrint(const char* message, u32 messageLength) {
    if (!messageLength) return;
    // message[messageLength - 1] = '\0';
    printf("%s", message);
}

void MicroDelay(u32 delay) {
    volatile u64* timeStamp = (u64*)(BCM_PERIPH_BASE_VIRT + 0x20003004);
    u64 stop = *timeStamp + delay;

    while (*timeStamp < stop) 
        __asm__("nop");
}

Result PowerOnUsb() {
    volatile u32* mailbox;
    u32 result;

    mailbox = (volatile uint32_t*)MAILBOX_REGS_BASE;
    while (mailbox[6] & 0x80000000);
    mailbox[8] = 0x80;
    do {
        while (mailbox[6] & 0x40000000);        
    } while (((result = mailbox[0]) & 0xf) != 0);
    return result == 0x80 ? OK : ErrorDevice;
}