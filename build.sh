#!/usr/bin/env bash

src='src'
out='out'

rm -rf $out
mkdir $out 

clang --target=aarch64-elf -c $src/boot.S -o $out/boot.o
clang --target=aarch64-elf -O1 -c $src/main.c -o $out/main.o

ld.lld -m aarch64elf $out/boot.o $out/main.o -T link.ld -o $out/kernel8.elf

llvm-objcopy -O binary $out/kernel8.elf $out/kernel8.img
