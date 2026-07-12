#include <stddef.h>
#include <stdint.h>
#include "graphics.h"
#include "../requests/requests.h"
struct limine_framebuffer *fb;
uint64_t width;
uint64_t height;

 void graphics_init(void){
  fb = framebuffer_request.response->framebuffers[0];
  width = fb->width;
  height = fb->height;
 }
//X: The X coordinate
//Y: The Y coordinate
//color: The color of the pixel to be drawn in RGB
void put_pixel(uint32_t x, uint32_t y, uint32_t color){
 if (!(x >= 0 && x < fb->width && y >= 0 && y < fb->height)) return;
 uint32_t* fb_address = (uint32_t *)fb->address;
 size_t pitch_stride = fb->pitch / 4;

 fb_address[y*pitch_stride + x] = color ;
}
