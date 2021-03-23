/*
game idea:
you're storming a castle and can break blocks and other objects -> they explode into chunks
I will need to sprinkle in some larger chunks too.....
*/

#include "neslib.h"
#include "neslib.h"

#include <stdio.h>
#include <stdlib.h>

#include <nes.h>
#include "vrambuf.h"
#include "bcd.h"

// link the pattern table into CHR ROM
//#link "chr_generic.s"

//#link "vrambuf.c"

// define a 2x2 metasprite
#define DEF_METASPRITE_2x2(name, code, attribute)\
unsigned char name[]={\
        0,      0,      (code)+0,   attribute, \
        0,      8,      (code)+1,   attribute, \
        8,      0,      (code)+2,   attribute, \
        8,      8,      (code)+3,   attribute, \
        128};

#define NES_MIRRORING 1("vertical", 0 = "horizontal")
//Nametable A starts at 0x2000, Nametable B starts at 0x2400

#define WORLD_WIDTH 240*2
#define WORLD_HEIGHT 256

//1920 bits, 1 for am i ground, 1 for am i breakable...
//#define SHADOW_SIZE 240
#define SHADOW_SIZE 8 // 2 rows of 4
char shadow[SHADOW_SIZE] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

byte outsideHelper; //used for debugging
byte outsideHelper2;
byte outsideHelper3;

int world_x = 0;
int world_y = 0;
int shadowWorldX = 0;

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

void updateScreen(unsigned char column, unsigned char row, char * buffer, unsigned char num_bytes)
{
  vrambuf_clear();
  set_vram_update(updbuf);
  vrambuf_put(NTADR_A(column, row), buffer, num_bytes);
  vrambuf_flush();
}

void writeBinary(unsigned char x, unsigned char y, byte value)
{
  char dx[16];
  //sprintf(dx, "%d", player.act.dx);
  sprintf(dx, "%d %d %d %d %d %d %d %d", (value&0x80)>>7, (value&0x40)>>6, (value&0x20) >>5, (value&0x10)>>4, (value&0x08)>>3, (value&0x04)>>2, (value&0x02)>>1, value&0x01);
  updateScreen(x, y, dx, 16);
}


void scrollShadow(int deltaX, char * newData)
{
  int y = 0;
  int x = 0;
  char leftover = 0;
  char old_leftover = 0;
  int numTiles_1;
  int numBytes;
  int remainder;
  byte oldLeftovers;

  outsideHelper = 0;
  outsideHelper2 = 0;
  outsideHelper3 = 0;

  deltaX+= shadowWorldX; //shadowWorldX accounts for any deltaX
  
  numTiles_1 = deltaX >> 3; //each tile is 8 pixels
  shadowWorldX += deltaX & 0x07; // if we have any leftover we keep that
  numBytes = numTiles_1 >> 2; //divide number by 4, or shift twice since we have 4 tiles per byte
  remainder = (numTiles_1 & 0x07) << 1; //4 tiles = 1 byte, how many bits are left

  for(y = 0; y < 2; y++)
  {
    oldLeftovers = newData[y] >> (8-remainder);
    // y = row number
    for(x = SHADOW_SIZE/2; x >= 0; x--)
    {
      //x = column number
      byte index = (y*4 + x) >> 3;
      {
        char buffer[32];
        sprintf(buffer, "index: %d", index);
        updateScreen(2, 2 buffer, 32);
      }
      byte leftover = shadow[index] >> (8-remainder);
      shadow[index] = (shadow[index] << remainder) | oldLeftovers;
      oldLeftovers = leftover;
    }
  }

}

// main function, run after console reset
void main(void) {

  unsigned char floorLevel = 20;
  unsigned char floorTile = 0xc0;
  //equal to num rows
  char newData[2] = {0xC0, 0xC0};

  unsigned int i = 0, j = 0;

  pal_all(PALETTE);// generally before game loop (in main)

  for(i = 1; i < 31; i++)
  {
    vram_adr(NTADR_A(i,floorLevel));
    vram_put(floorTile);
  }

  vram_adr(NTADR_A(100/8,18));
  vram_put(0x0F);

  ppu_on_all();

  for(i = 0; i < 20; i++)
  {
    ppu_wait_frame();
  }

  //always updates @ 60fps
  while (1)
  {

    char pad_result= pad_poll(0) | pad_poll(1);

    //game loop
    int deltaX = 8;

    world_x += deltaX;
    /*
    //expected result!
    // AA AA AA AA 
    // AA AA AA A8 -gets AA AA AA A8 - correct
    // AA AA AA A2 -gets AA AA AA A2
    // AA AA AA 8A -gets AA AA AA 8A
    // AA AA AA 2A -gets AA AA AA 2A
    // AA AA A8 AA...
    */
    scrollShadow(deltaX, newData);
    //scroll(world_x, world_y);

    for(i = 0; i < 15; i++)
    {
      ppu_wait_frame();
    }
  }
}
