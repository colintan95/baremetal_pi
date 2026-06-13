#ifndef MBOX_H_
#define MBOX_H_

#include "mmio.h"

#define MBOX_BASE (MMIO_BASE + 0xb880)

#define MBOX_CHANNEL_TAGS 8

#define MBOX_TAG_LAST 0
#define MBOX_TAG_GET_SERIAL 0x10004

#define MBOX_REQUEST 0
#define MBOX_RESPONSE 0x80000000

// The buffer must be aligned to 16 bytes because we can only pass a 28-bit
// address to the mailbox.
int mbox_call(unsigned int* buffer, char channel);

#endif
