
/*
A simple "hello world" example.
Set the screen background color and palette colors.
Then write a message to the nametable.
Finally, turn on the PPU to display video.
*/

#include "neslib.h"
#include "neslib.h"

#include <nes.h>
#include "vrambuf.h"
#include "bcd.h"

// link the pattern table into CHR ROM
//#link "chr_generic.s"

//#link "vrambuf.c"

typedef struct Actor
{
  unsigned char x;
  unsigned char y;
  char dx;
  char dy;
  unsigned char attribute;
} Actor;

unsigned char metasprite[]={
        0,      0,      0xD8+0,   0, 
        0,      8,      0xD8+1,   0, 
        8,      0,      0xD8+2,   0, 
        8,      8,      0xD8+3,   0, 
        128}; //128 is the num of chars * 8 = num of bits


char PALETTE[32] = 
{
  0x03, // screen color
  0x24, 0x16, 0x20, 0x0,// background palette 0
  0x1c, 0x20, 0x2c, 0x0,// background palette 1
  0x00, 0x1a, 0x20, 0x0,// background palette 2
  0x00, 0x1a, 0x20, 0x0,// background palette 3

  0x3F, 0x3F, 0x22, 0x0,// sprite palette 0
  0x00, 0x37, 0x25, 0x0,// sprite palette 1
  0x36, 0x21, 0x19, 0x0,// sprite palette 2
  0x1d, 0x37, 0x2b,// sprite palette 3
};

void writeText(char * data, int addressX, int addressY)
{
  char * line = data;
  int startIndex = 0;
  int currentIndex = 0;
  //set init address
  vram_adr(NTADR_A(addressX, addressY));
  while(*data)
  {
    //newline = 10
    if(data[0] == 10)
    {
      vram_write(line, currentIndex-startIndex);
      line = data+1;
      startIndex = currentIndex+1;
      addressY++;
      vram_adr(NTADR_A(addressX, addressY));
    }
    currentIndex++;
    data++;
  }
  if(startIndex!=currentIndex)
  {
    vram_write(line, currentIndex-startIndex);
  }
}

unsigned int getAbs(int n) 
{ 
    int const mask = n >> (1); 
    return ((n + mask) ^ mask); 
} 

int doIHit(int x, int y, int x2, int y2)
{
  if(getAbs(x - x2) <= 8 && getAbs(y - y2) <= 8)
  {
    return true;
  }
  return false;
}

int multiply(int a, int b)
{
  int result = 0;
  while(b)
  {
  	result += a;
    	if(b < 0)
        {
          	b++;
        }
    	else
        {
    		b--;
        }
  }
  return result;
}

void updateScreen(unsigned char column, unsigned char row, char * buffer, unsigned char num_bytes)
{
  vrambuf_clear();
  set_vram_update(updbuf);
  vrambuf_put(NTADR_A(column, row), buffer, num_bytes);
  vrambuf_flush();
}

// main function, run after console reset
void main(void) {
  
  //palette, behind, vert, horz
  unsigned int walk_timer = 1;
  unsigned int walk_wait = 0;
  unsigned int door_timer = 60*2;
  unsigned int door_wait = 0;
  
  unsigned char floorLevel = 20;
  unsigned char floorTile = 0xc0;
  
  unsigned int i = 0, j = 0;
  bool touchedDoor = false;
  Actor player;
  
  unsigned char doorX = multiply(29,8), doorY = multiply(18,8);
  //unsigned char doorX = 0, doorY = multiply(18,8);
  unsigned char doorW = 16, doorH = 16;
  
  // [sprite palette_0, sprite palette_1, ?, ?, ?, behind foreground, flip hor, flip vert]
  
  pal_all(PALETTE);// generally before game loop (in main)
  
  // write text to name table
  //vram_adr(NTADR_A(2,2));		// set address
  //vram_write("This is\nJoshua Byron's\nfirst NES 'Game'!", 41);	// write bytes to video RAM
  //vram_put for one byte
  writeText("This is\nJoshua Byron's\nfirst NES 'Game'!", 2, 2);
  for(i = 1; i < 31; i++)
  {
    vram_adr(NTADR_A(i,floorLevel));
    vram_put(floorTile);
  }
  
  
  vram_adr(NTADR_A(29, 18));
  vram_put(0xc4);
  vram_put(0xc6);
  vram_adr(NTADR_A(29, 19));
  vram_put(0xc5);
  vram_put(0xc7);
  
  //vram_adr(NTADR_A(1, 18));
  //vram_put(0xc4);
  //vram_put(0xc6);
  //vram_adr(NTADR_A(1, 19));
  //vram_put(0xc5);
  //vram_put(0xc7);
  
  
  
  player.x = 30;
  player.y = doorY;
  player.dx = 1;
  player.dy = 0;
  player.attribute = 1 | (0 << 5) | (0 << 6) | (0 << 7);
  metasprite[3] = player.attribute;
  metasprite[7] = player.attribute;
  metasprite[11] = player.attribute;
  metasprite[15] = player.attribute;
  
  // enable PPU rendering (turn on screen)
  ppu_on_all();

  //always updates @ 60fps
  while (1)
  {
    //game loop
    char cur_oam = 0;
    
    
    //x, y, sprite number, attribute, oam
    //cur_oam = oam_spr(x, y, 0x19 ,attrib, cur_oam);
    
    cur_oam = oam_meta_spr(player.x, player.y, cur_oam, metasprite);
    
    
    walk_wait = (walk_wait+1) % walk_timer;
    //only updates every walk_timer num of frames
    
    
    if(doIHit(player.x, player.y, doorX+8, doorY))
    {
      //attrib = 2 | (0 << 5) | (0 << 6) | (0 << 7);
      player.attribute = 1 | (0 << 5) | (1 << 6) | (0 << 7);
      metasprite[3] = player.attribute;
      metasprite[7] = player.attribute;
      metasprite[11] = player.attribute;
      metasprite[15] = player.attribute;
      
      metasprite[0] = 8;
      metasprite[1] = 0;
      metasprite[4] = 8;
      metasprite[5] = 8;
      metasprite[8] = 0;
      metasprite[9] = 0;
      metasprite[12] = 0;
      metasprite[13] = 8;
      
      player.dx = -1;
      updateScreen(2, 5, "On the door!", 12);
      touchedDoor = true;
    }
    else
    {
      if(touchedDoor == true && door_wait == 0)
      {
        updateScreen(2, 5, "            ", 12);
        touchedDoor = false;
      }
      else
      {
        door_wait = (door_wait+1) % door_timer;
      }
    }
    
    if(doIHit(player.x, player.y, 0, player.y))
    {
      //attrib = 0 | (0 << 5) | (1 << 6) | (0 << 7);
      player.attribute = 1 | (0 << 5) | (0 << 6) | (0 << 7);
      metasprite[3] = player.attribute;
      metasprite[7] = player.attribute;
      metasprite[11] = player.attribute;
      metasprite[15] = player.attribute;
      
      metasprite[0] = 0;
      metasprite[1] = 0;
      metasprite[4] = 0;
      metasprite[5] = 8;
      metasprite[8] = 8;
      metasprite[9] = 0;
      metasprite[12] = 8;
      metasprite[13] = 8;
      player.dx = 1;
      //x++;
    }

    if(walk_wait == 0)
    {
      player.x += player.dx;
      player.y += player.dy;
    }
    
    //this makes it wait one frame in between updates
    ppu_wait_frame();
  }
}
