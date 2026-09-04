#include <stdint.h>
#include "../../io/io.h"
#include "./tsc.h"
#include "../../terminal/terminal.h"
#include "../../kstd/kstd.h"
uint64_t tsc_freq;

uint64_t calibrate(){
  outb(0x43, 0x30); //Dumbass magic number tells it to only interrupt when it reaches 0
  //Load maximum available value 
  outb(0x40, 0xFF);
  outb(0x40, 0xFF);
  uint64_t start_tsc = rdtsc();
  uint16_t current = 0xFFFF;

  while (current >  0x9FFF) {
    outb(0x43, 0x00);
    uint8_t low = inb(0x40);
    uint8_t high = inb(0x40);
    current = ((uint16_t)high << 8) | low;
  }
  uint64_t end_tsc = rdtsc();

  uint32_t elapsed_pit_ticks = 0xFFFF - current;
  uint64_t elapsed_tsc = end_tsc - start_tsc;
  char buf[32];
  int_to_base_string(start_tsc, buf, 10);
  print(buf);
  int_to_base_string(current, buf, 10);
  print(buf);
  int_to_base_string(end_tsc , buf, 10);
  print(buf);
  int_to_base_string(elapsed_pit_ticks, buf, 10);
  print(buf);
  int_to_base_string(elapsed_tsc, buf, 10);
  print(buf);
  return (unsigned long)((elapsed_tsc * 1193182) / elapsed_pit_ticks);
}

uint64_t rdtsc(void) {
  uint32_t lo, hi;
  asm volatile("lfence\n\t"
               "rdtsc\n\t"
               : "=a"(lo), "=d"(hi)
               :
               : "memory");
  return ((uint64_t)hi << 32) | lo;
}

void init_tsc(){
  uint64_t freq = calibrate();
  tsc_freq = freq;
}

__attribute__((noinline))
uint64_t tsc_get_freq(){
  return tsc_freq;
}
