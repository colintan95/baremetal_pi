#ifndef INTERRUPTS_H_
#define INTERRUPTS_H_

// IRQ numbers
#define IRQ_TIMER_MATCH_1 1
#define IRQ_UART 57

void timer_init();

void irq_enable_irq(int irq);

#endif
