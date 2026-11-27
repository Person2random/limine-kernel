#include "./context.h"
#include "../../terminal/terminal.h"
void test_thread(void){
  print("Ready to mog all other kernel threads, threadgamy\n");
  for (;;) {
  }
}


extern void ring0_context_switch(struct thread_context* context);
extern void ring3_context_switch(struct thread_context* context);
void context_switch(struct thread_context* context){
  if((context->cs & 3) == 3){
    ring0_context_switch(context);
  }
  else {
	  ring3_context_switch(context);
  }
}
