#ifndef UART_H_
#define UART_H_

void uart_init();

void uart_send(char c);

void uart_print(const char* s);
void uart_print_hex(unsigned int value);
 
#endif // UART_H_
