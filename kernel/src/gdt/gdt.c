
#include "./gdt.h"
#include <stdint.h>
extern void load_gdt(void* gdtr);
extern void reload_segments(void);

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t flags_limit;
    uint8_t  base_high;
}  __attribute__((packed)) gdt_entry;

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  flags_limit;
    uint8_t  base_high;

    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed)) TSSDescriptor; 


typedef struct {
  gdt_entry null;
  gdt_entry Kcode;
  gdt_entry Kdata;
  gdt_entry code;
  gdt_entry data;
  TSSDescriptor tss_desc;
  
} __attribute__ ((packed)) gdt_table;


void set_gdt_entry(
    gdt_entry* entry,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t flags
)
{
    entry->limit_low = limit & 0xFFFF;
    entry->base_low = base & 0xFFFF;
    entry->base_middle = (base >> 16) & 0xFF;

    entry->access = access;

    entry->flags_limit =
        ((limit >> 16) & 0x0F) |
        (flags & 0xF0);

    entry->base_high = (base >> 24) & 0xFF;
}

typedef struct {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed)) GDTR;


gdt_table gdt;
GDTR gdt_pointer;

void init_gdt(void){
  set_gdt_entry(&gdt.null, 0,0,0,0);
  set_gdt_entry(&gdt.Kcode, 67, 41, 0x9A, 0xA0); //Note that base and limit are ignored by the CPU in long mode, 67 and 41 are only for the shits and giggles
  set_gdt_entry(&gdt.Kdata, 69, 420, 0x92, 0);
  set_gdt_entry(&gdt.code, 0,0,0xFA,0xA0);
  set_gdt_entry(&gdt.data, 0, 0, 0xF2, 0x00); //Tung tung larp
  gdt_pointer.base = (uint64_t)&gdt;
  gdt_pointer.limit = sizeof(gdt_table) - 1;
  load_gdt(&gdt_pointer);
  reload_segments();
}
