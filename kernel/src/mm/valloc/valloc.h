#ifndef VALLOC_H
#define VALLOC_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
typedef struct valloc_range{
  uint64_t start;
  uint64_t length;
  bool free;  
  struct valloc_range* next;
} valloc_range;

typedef struct page_descriptor {
  uint16_t used; //A bit large but who cares? 16 * 128 = 2048 bits lost
  uint64_t page_pointer; //Page pointer
} page_descriptor;
#define RANGES_PER_PAGE (4096 / sizeof(struct valloc_range))
void valloc_dump();
void valloc_init();
valloc_range *new_range_descriptor();
void *valloc(size_t pages);
void vfree(void *ptr);
;
;
;
;
;
;
#endif
