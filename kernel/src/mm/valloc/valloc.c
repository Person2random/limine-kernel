#include "valloc.h"
#include "../../kstd/kstd.h"
#include "../../requests/requests.h"
#include "../../terminal/terminal.h"
#include "../pmm.h"
#include "../vmm.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
valloc_range *valloc_head;
page_descriptor descriptors[PAGE_DESC_COUNT];
void valloc_init() {
  uint64_t base = valloc_find_arena();
  if (!base)
    print("Holy fuck you dont have a free PML4E how does that even happen\n");
  memset(descriptors, 0, sizeof(page_descriptor) * PAGE_DESC_COUNT);
  uint64_t hhdm_offset = hhdm_request.response->offset;
  uint64_t page = pmm_alloc_page();
  valloc_head = (struct valloc_range *)(page + hhdm_offset);
  valloc_range first_entry;
  first_entry.start = base;
  first_entry.length =
      0x10000000000; // One terabyte of address space, Try making this 512 bytes
                     // one time and running the kernel again
  first_entry.next = 0;
  first_entry.free = true;

  *valloc_head = first_entry;
  descriptors[0].page_pointer = page;
  descriptors[0].used = 1;
  descriptors[0].lhbitmap = 1;
  valloc_head->owner = &descriptors[0];
  valloc_head->index = 0;
}

valloc_range *new_range_descriptor() {
  uint64_t hhdm_offset = hhdm_request.response->offset;
  uint16_t pmm_slot = 6969;
  for (size_t i = 0; i < PAGE_DESC_COUNT; i++) {
    if (descriptors[i].used < RANGES_PER_PAGE &&
        descriptors[i].page_pointer != 0) {
      pmm_slot = i;
      break;
    }
  }
  if (pmm_slot == 6969) {
    for (size_t i = 0; i < PAGE_DESC_COUNT; i++) {
      if (descriptors[i].page_pointer == 0) {
	uint64_t page = pmm_alloc_page();
	if(page == 0) return 0;
        descriptors[i].page_pointer = page;
        pmm_slot = i;
        break;
      }
    }
  }
  if (pmm_slot == 6969) {
    return 0;
  }
  uint16_t range_index = 6969;
  bool found = false;
  for (size_t i = 0; i < 64; i++) {
    if (!(((descriptors[pmm_slot].lhbitmap) >> i) & 1)) {
      range_index = i;
      descriptors[pmm_slot].lhbitmap |= (1ULL << i);
      found = true;
      break;
    }
  }
  for (size_t i = 0; i < RANGES_PER_PAGE-64; i++) {
	  if(found) break;
    if (!(((descriptors[pmm_slot].hhbitmap) >> i) & 1)) {
      range_index = i + 64;
      descriptors[pmm_slot].hhbitmap |= (1ULL << i);
      break;
    }
  }
  found = true;
  
  if(range_index == 6969) return 0;
  valloc_range *range_pointer =
      (valloc_range *)(descriptors[pmm_slot].page_pointer + hhdm_offset +
                       (sizeof(valloc_range) * range_index));
  descriptors[pmm_slot].used++;
  range_pointer->owner = &(descriptors[pmm_slot]);
  range_pointer->index = range_index;
  return range_pointer;
}

void *valloc(size_t pages) {
	if(pages == 0) return 0;
  uint64_t byte_size = pages * 4096; // Page size, 4 KiB, 0x1000
  valloc_range *current = valloc_head;
  uint64_t addr = 0;
  while (1) {
    if (current->length >= byte_size && current->free == true) {
      addr = current->start;
      break;
    }
    if (current->next == 0)
      return 0; // No valid range left
    current = current->next;
  }
  uint64_t old_len = current->length;
  if (old_len - byte_size == 0) {
    current->free = false;
    return (void *)addr;
  }
  valloc_range *new = new_range_descriptor();
  if (new == 0) {
    return 0;
  }
  current->length = byte_size;
  current->free = false;
  new->free = true;
  new->start = current->start + current->length;
  new->next = current->next;
  new->length = old_len - byte_size;
  current->next = new;
  return (void *)addr;
}

void vfree(void *ptr) {
  valloc_range *current = valloc_head;
  valloc_range *neighbor_left = 0;
  while (1) {
    if (current->start == (uint64_t)ptr) {
      break;
    }
    if (current->next == 0) {
      return;
    }
    neighbor_left = current;
    current = current->next;
  }
  if(current->free) return;
  current->free = true;
  valloc_range *neighbor_right = current->next;
  if (neighbor_right != 0 && neighbor_right->free == true) {
    current->length += neighbor_right->length;
    current->next = neighbor_right->next;
    neighbor_right->start = 0;
    page_descriptor* owner = neighbor_right->owner;
    if(neighbor_right->index >= 64){
    	owner->hhbitmap &= ~((uint64_t)1 << (neighbor_right->index-64));
    }
    else{
    	owner->lhbitmap &= ~((uint64_t)1 << (neighbor_right->index));
    }
    owner->used--;
  }
  if (neighbor_left != 0 && neighbor_left->free == true) {
    neighbor_left->length += current->length;
    neighbor_left->next = current->next;
    current->start = 0;
    page_descriptor* owner = current->owner;
    if(current -> index >= 64){
	    owner->hhbitmap &= ~((uint64_t)1 << (current->index-64));
    }
    else {
	    owner->lhbitmap &= ~((uint64_t)1 << (current->index));
    }
    owner->used--;
    current = neighbor_left;
  }
}

void valloc_dump() {
  valloc_range *current = valloc_head;
  uint16_t index = 0;

  while (current != 0) {
    char buf[32];

    utoa(index, buf);
    debug_print(buf);

    if (current->free)
      debug_terminal_write("\tFree");
    else
      debug_terminal_write("\tNot free");

    debug_terminal_write("\tStart: ");
    utoa(current->start, buf);
    debug_print(buf);

    debug_terminal_write("\tLength: ");
    utoa(current->length, buf);
    debug_print(buf);

    debug_terminal_write("\tNext: ");
    utoa((uint64_t)current->next, buf);
    debug_print(buf);

    debug_terminal_write("\n");

    current = current->next;
    index++;
  }
}
