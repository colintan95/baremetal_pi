CC = clang
CFLAGS = --target=aarch64-elf -O1
DEP_FLAGS = -MMD -MP

out/kernel8.img: out out/boot.o out/main.o out/mmu.o out/interrupts.o out/uart.o out/mbox.o
	ld.lld -m aarch64elf out/boot.o out/main.o out/mmu.o out/interrupts.o out/uart.o out/mbox.o -T link.ld -o out/kernel8.elf
	llvm-objcopy -O binary out/kernel8.elf out/kernel8.img

out/boot.o: src/boot.S
	clang $(CFLAGS) -c src/boot.S -o out/boot.o

out/main.o: src/main.c
	clang $(CFLAGS) $(DEP_FLAGS) -c src/main.c -o out/main.o

out/mmu.o: src/mmu.c
	clang $(CFLAGS) $(DEP_FLAGS) -c src/mmu.c -o out/mmu.o

out/interrupts.o: src/interrupts.c
	clang $(CFLAGS) $(DEP_FLAGS) -c src/interrupts.c -o out/interrupts.o

out/uart.o: src/uart.c
	clang $(CFLAGS) $(DEP_FLAGS) -c src/uart.c -o out/uart.o

out/mbox.o: src/mbox.c
	clang $(CFLAGS) $(DEP_FLAGS) -c src/mbox.c -o out/mbox.o

-include out/main.d out/mmu.d out/interrupts.d out/uart.d out/mbox.d

out:
	mkdir -p out

clean:
	rm -rf out
