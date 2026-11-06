#include "./context.h"
#include "../../terminal/terminal.h"
void test_thread(void){
  print("Ready to mog all other kernel threads, threadgamy\n");
  for (;;) {
  }
}

extern void ring0_context_switch(struct thread_context* context);
void context_switch(struct thread_context* context,uint8_t ring){
  if(ring == 0){
    ring0_context_switch(context);
  }
  
}
