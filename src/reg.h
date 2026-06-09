#ifndef REG_H_
#define REG_H_

static inline void reg_write(unsigned long reg, unsigned int value) {
  *(volatile unsigned int*)reg = value;
}

static inline unsigned int reg_read(unsigned long reg) {
  return *(volatile unsigned int*)reg;
}

#endif // REG_H_
