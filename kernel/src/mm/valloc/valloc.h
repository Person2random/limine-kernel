#ifndef VALLOC_H
#define VALLOC_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define VALLOC_BASE 0xFFFF800000000000ULL
typedef struct valloc_range{
  uint64_t start;
  uint64_t length;
  bool free;  
  struct valloc_range* next;


  struct page_descriptor* owner;
  uint16_t index;
} valloc_range;

typedef struct page_descriptor {
  uint16_t used;
  uint64_t hhbitmap;
  uint64_t lhbitmap;
  uint64_t page_pointer; //Page pointer
} page_descriptor;
#define RANGES_PER_PAGE (4096 / sizeof(struct valloc_range))
#define PAGE_DESC_COUNT 256
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
