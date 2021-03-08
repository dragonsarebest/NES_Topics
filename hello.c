
/*
A simple "hello world" example.
Set the screen background color and palette colors.
Then write a message to the nametable.
Finally, turn on the PPU to display video.
*/

#include "neslib.h"
#include "vrambuf.h"
#include "bcd.h"

// link the pattern table into CHR ROM
//#link "chr_generic.s"

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

void fillTile(unsigned char tile, int startAddressX, int startAddressY, int endAddressX, int endAddressY)
{
  int i, j = 0;
  vram_adr(NTADR_A(startAddressX, startAddressY));
  
  for(i = startAddressX; i < endAddressX; i++)
  {
    for(j = startAddressY; j < endAddressY; j++)
    {
      vram_adr(NTADR_A(i, j));
      vram_put(tile);
    }
  }
}

struct hitbox
{
  int x;
  int y;
  unsigned char w;
  unsigned char h;
  unsigned char sprite;
};

// main function, run after console reset
void main(void) {
  char x = 30, y = 90;
  //palette, behind, vert, horz
  unsigned char attrib = 0;
  unsigned int walk_timer = 1;
  unsigned int walk_wait = 0;
  unsigned char floorLevel = 20;
  unsigned char floorTile = 0xc0;
  unsigned int i = 0, j = 0;
  
  // [sprite palette_0, sprite palette_1, ?, ?, ?, behind foreground, flip hor, flip vert]
  const char PALETTE[32] = 
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
  pal_all(PALETTE);// generally before game loop (in main)
  
  //attrib is 8 bits...
  attrib = 2 | (0 << 5) | (0 << 6) | (0 << 7);
  
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
  
  // enable PPU rendering (turn on screen)
  ppu_on_all();

  //always updates @ 60fps
  while (1)
  {
    //game loop
    char cur_oam = 0;
    char oam_id = 0;
    
    //x, y, sprite number, attribute??, oam
    oam_id = oam_spr(x, y, 0x19 ,attrib, cur_oam);
    oam_hide_rest(oam_id);
    
    //turn 0-> 1, turn 1-> -1
    walk_wait = (walk_wait+1) % walk_timer;
    //only updates every walk_timer num of frames
    if(x >= 100)
    {
      attrib = 1 | (0 << 5) | (1 << 6) | (0 << 7);
    }
    if(x <= 1)
    {
      attrib = 2 | (0 << 5) | (0 << 6) | (0 << 7);
    }
    x += (((attrib & 0x40) >> 6)* -2 + 1) * walk_wait == 0;
    
    
    
    //this makes it wait one frame in between updates
    ppu_wait_frame();
  }
}
