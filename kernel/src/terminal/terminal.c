#include "./font.h"
#include "../graphics/graphics.h"
#include <stddef.h>
#include <stdint.h>


// array size is 4100unsigned char *font_ptr;
uint32_t fg_default;
uint32_t bg_default;
size_t font_size;
uint8_t mode;
uint8_t charsize;
uint8_t *glyphs;
uint32_t cursor_x;
uint32_t cursor_y;
uint32_t max_x;
uint32_t max_y;
void terminal_init(){

  mode = font[2];
  charsize = font[3];
  glyphs = font + 4;
  cursor_x = 0;
  cursor_y = 0;
  max_x = width / 8;
  max_y = height / charsize;
  if (charsize == 16)
    put_pixel(10, 10, 0x00FF00);
  else
    put_pixel(10, 10, 0xFF0000);
    fg_default = 0xFFFFFF;
    bg_default = 0X000000;
}


void put_char(uint32_t x, uint32_t y, char c ,uint32_t fg, uint32_t bg){
    if(x >= max_x || y > max_y) return;
    if(x < 0 || y < 0) return;
    uint32_t realx = x * 8;
    uint32_t realy = y * charsize;
    uint8_t* glyph = glyphs + ((uint8_t)c * charsize);
    for(uint32_t row = 0; row < charsize; row++){
      uint8_t bits = glyph[row];
      for(uint32_t col = 0; col < 8; col++){
        if(bits & (0x80 >> col)){
          put_pixel(realx + col,  realy + row,  fg);
        }
        else {
          put_pixel(realx + col,  realy + row,  bg);
        }
      }
    }    
    
  
}



void write_char(char c){
  if(c == '\n'){
        cursor_y++;
        cursor_x = 0;
        return;
    }
  if(c == '\r'){
      cursor_x = 0;
      return;
    }
   if(c == '\t'){
      cursor_x += 4;
      return;
    }
  
  put_char(cursor_x,cursor_y,c,fg_default,bg_default);
  if(cursor_x + 1 == max_x){
    cursor_y++;
    cursor_x = 0;
  }
  else {
    cursor_x++;
  }
}


void terminal_write(char c[]){
  while(*c){
         write_char(*c);
    c++;
  }
}


void print(char c[]){
  while(*c){
   write_char(*c);
   c++;
   
  }
  cursor_y++;
  cursor_x = 0;
}
