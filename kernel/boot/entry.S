#include "multiboot.h"

.section .multiboot
.align 4
multiboot_header:
    .long MULTIBOOT_HEADER_MAGIC
    .long MULTIBOOT_HEADER_FLAGS
    .long MULTIBOOT_HEADER_CHECKSUM

.section .text
.global _start
.type _start, @function
_start:
    cli
    movl $stack_top, %esp
    pushl %ebx
    pushl %eax
    call kernel_main
1:
    hlt
    jmp 1b

.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:
