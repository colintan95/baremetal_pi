#ifndef UART_H_
#define UART_H_

#include "stdint.h"

void uart_init();

void uart_clear_interrupts();

void uart_send(char c);

// Returns -1 if there is nothing to read
int uart_read(char* c);

void uart_print(const char* s);
void uart_print_hex(unsigned int value);

void uart_dump_u32(uint32_t value);

void uart_mem_dump(unsigned int addr, int size);

#endif // UART_H_
