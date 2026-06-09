#!/usr/bin/env bash

src='src'
out='out'

rm -rf $out
mkdir $out 

clang --target=aarch64-elf -c $src/boot.S -o $out/boot.o

clang --target=aarch64-elf -O1 -c $src/uart.c -o $out/uart.o
clang --target=aarch64-elf -O1 -c $src/main.c -o $out/main.o

# boot.o must come first so that the boot instructions start at 0x80000.
ld.lld -m aarch64elf $out/boot.o $out/uart.o $out/main.o -T link.ld -o $out/kernel8.elf

llvm-objcopy -O binary $out/kernel8.elf $out/kernel8.img
