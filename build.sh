#!/usr/bin/env bash

src='src'
out='out'

rm -rf $out
mkdir $out 

clang --target=aarch64-elf -c $src/boot.S -o $out/boot.o
ld.lld -m aarch64elf $out/boot.o -T $src/link.ld -o $out/kernel8.elf

llvm-objcopy -O binary $out/kernel8.elf $out/kernel8.img
