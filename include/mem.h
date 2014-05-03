#ifndef _MEM_H_
#define _MEM_H_

#define REG_ERR 19
#define PG_WRITE 0x2
#define PG_PRESENT 0x1
#define PG_BITS 12
#define PG_SIZE (1 << (PG_BITS))

// Convert address to the start of the page.
static inline uintptr_t PGADDR(uintptr_t addr) {
  return addr & ~(PG_SIZE - 1);
}

// Convert page number to address of start of page.
static inline uintptr_t PGNUM_TO_PGADDR(uintptr_t pgnum) {
  return pgnum << PG_BITS;
}


#endif  // _MEM_H_
