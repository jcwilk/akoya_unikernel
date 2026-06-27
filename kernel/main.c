#include "console/console.h"

#define AKOYA_BOOTSTRAP_MESSAGE "akoya_unikernel bootstrap ok"

#ifndef AKOYA_BUILD_ID
#define AKOYA_BUILD_ID "dev"
#endif

void kernel_main(void)
{
    console_init();
    console_write_line(AKOYA_BOOTSTRAP_MESSAGE);
    console_write("build_id=");
    console_write_line(AKOYA_BUILD_ID);

    /* QEMU isa-debug-exit (also harmless on bare metal). */
    __asm__ volatile ("outb %%al, $0xf4" : : "a"((unsigned char)0));

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
