#include "../kstd/kstd.h"
#include "../memory.h"
#include "../requests/requests.h"
#include "../terminal/terminal.h"
#include <stdint.h>
#include <stdbool.h>
extern uint8_t _kernel_size[];
uint8_t* bitmap;
uint64_t bitmap_size;
uint64_t KbasePhys;
uint64_t KbaseVirt;
void pmm_mark(uint64_t frame, bool used){
  if(used) bitmap[frame/8] |= (1 << (frame % 8 ));
  else bitmap[frame/8] &= ~(1 << (frame % 8));
 }

void pmm_set_region(uint64_t base, uint64_t limit, bool used){
  uint64_t start = base/4096;
  uint64_t count = limit/4096;
  for(uint64_t i = 0; i < count; i++){
    uint64_t frame = start+i;
    pmm_mark(frame, used);    
  }
}

void init_pmm(void){
  KbasePhys = executable_address_request.response->physical_base;
  KbaseVirt =  executable_address_request.response->virtual_base;
  uint64_t hhdm_offset = hhdm_request.response->offset;
  uint64_t entry_count = memmap_request.response->entry_count;
  uint64_t highest_address = 0;
  struct limine_memmap_entry **entries = memmap_request.response ->entries;
  debug_print("Starting search for highest address...");
  for(uint64_t i = 0; i < entry_count;i++){
    struct limine_memmap_entry *entry = entries[i];
    if(entry->type != LIMINE_MEMMAP_USABLE) continue;
    uint64_t high_address = entry->base + entry->length;
    if (high_address > highest_address)
      highest_address = high_address;
  }
  
  highest_address = (highest_address + 4095) & ~4095;
  char buf1[32];
  utoa(highest_address, buf1);
  debug_print("Highest address: ");
  debug_terminal_write(buf1);
  debug_print("");
  uint64_t page_count = highest_address/4096;
  bitmap_size = (page_count + 7)/8;
  debug_print("Finding entry for bitmap...");
  for(uint64_t i = 0; i < entry_count; i++){
    struct limine_memmap_entry *entry = entries[i];
    if(entry->type ==LIMINE_MEMMAP_USABLE &&entry->length >= bitmap_size && entry->base >= 0x200000 ){
      bitmap = (uint8_t*)(entry->base+hhdm_offset);
      char buf[32];
      utoa((uint64_t)entry->base, buf);
      debug_terminal_write("Base address of phys map of bitmap: ");
      debug_print(buf);
      break;
    }
  }
  char buf2[32];
  utoa((uint64_t)bitmap-hhdm_offset, buf2);
  debug_print("Bitmap physical address: ");
  debug_terminal_write(buf2);
  debug_print("");
  char buf3[32];
  utoa(bitmap_size, buf3);
  debug_print("Bitmap size: ");
  debug_terminal_write(buf3);
  debug_print("");
  debug_terminal_write("Kernel size: ");
  utoa((uint64_t)_kernel_size, buf3);
  debug_print(buf3);
  if(bitmap == NULL){
    debug_print("Problem occured while init pmm");
    __asm__ volatile("sti;hlt"); //In place of a panic function, Replace later
  }
  debug_print("Setting bitmap memory to 1...");
  memset((void*)bitmap, 0xFF, bitmap_size); //set to bitmap_size instead of 1024^2
  debug_print("Setting usable regions to free...");
  for(uint64_t i = 0; i < entry_count; i++){
    struct limine_memmap_entry *entry = entries[i];
    if(entry->type == LIMINE_MEMMAP_USABLE){
      pmm_set_region(entry->base,entry-> length, false);
    }
  }
  debug_print("Marking bitmap reigons as used...");
  uint64_t bitmap_pages = (bitmap_size + 4095) / 4096;
  pmm_set_region((uint64_t)bitmap-hhdm_offset,bitmap_pages * 4096 , true);
  debug_print("Setting anything below 2 MiB as used...");
  pmm_set_region(0x0, 0x200000, true);
  debug_print("Setting kernel memory as used");
  uint64_t KlimitPhys = KbasePhys + (uint64_t)_kernel_size;
  pmm_set_region(KbasePhys, KlimitPhys, true);
  debug_print("Done!");
  return;
}

uint64_t pmm_alloc_page(void){
  for(uint64_t byte = 0; byte < bitmap_size; byte++){
    if(bitmap[byte] != 0xFF){
      for(uint8_t bit = 0; bit < 8; bit++){
        if(!(bitmap[byte] & (1 << bit))){
          uint64_t frame = (byte * 8) + bit;
          bitmap[byte] |= (1<<bit);
          return frame * 4096;
        }
      }
    }
  }
  return 0;
}



void pmm_free_page(uint64_t address){
  if(address % 4096 != 0) return;
  
  uint64_t frame = address/4096;
  uint64_t byte = frame/8;
  uint64_t bit = frame % 8;
  bitmap[byte] &= ~(1 << (bit));
}
