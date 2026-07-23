#include <limine.h>
#ifndef REQUESTS_H
#define REQUESTS_H

extern volatile struct limine_framebuffer_request framebuffer_request; 
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_executable_address_request executable_address_request;
#endif
