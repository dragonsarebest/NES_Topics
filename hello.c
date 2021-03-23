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
#define SHADOW_SIZE 4 // 30 columns, 32 rows
char shadow[SHADOW_SIZE];

byte outsideHelper; //used for debugging
byte outsideHelper2;
byte outsideHelper3;

int world_x = 0;
int world_y = 0;

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

void scrollShadow(int deltaX, char * newData)
{
  int i = 0;
  int j = 0;
  char leftover = 0;
  char old_leftover = 0;
  int numTiles_1;
  int numBytes;
  int remainder;
  byte oldLeftovers;

  outsideHelper = 0;
  outsideHelper2 = 0;
  outsideHelper3 = 0;
  deltaX = 0;

  numTiles_1 = 1;
  numBytes = 0;
  remainder = 2;

  oldLeftovers = newData[0];
  
  outsideHelper = oldLeftovers;
  
  oldLeftovers = oldLeftovers >> (8-remainder);
  //oldLeftovers = oldLeftovers >> 6;
  //1111 1111 -> 0000 0011
  
  outsideHelper = oldLeftovers;
  
  for(i = SHADOW_SIZE; i >= 0; i--)
  {
    byte leftover = shadow[i];
    outsideHelper = leftover;
    leftover = leftover >> (8-remainder);
    outsideHelper = leftover;
    //1010 1010 -> 0000 0010
    
    outsideHelper = (shadow[i] << remainder);
    //outsideHelper = 1010 1010 -> 1010 1000
    
    outsideHelper = (shadow[i] << remainder) | oldLeftovers;
    //outsideHelper = 1010 1000 -> 1010 1011
    
    //old_leftover = 0000 0011
    shadow[i] = (shadow[i] << remainder) | oldLeftovers;

    oldLeftovers = leftover;
    outsideHelper = oldLeftovers;
    //oldLeftovers = 0000 0010
  }
  newData[0] = 0xAA; //to keep it scrolling 

}

// main function, run after console reset
void main(void) {

  unsigned char floorLevel = 20;
  unsigned char floorTile = 0xc0;
  char newData[1] = {0x00};

  unsigned int i = 0, j = 0;

  pal_all(PALETTE);// generally before game loop (in main)

  for(i = 1; i < 31; i++)
  {
    vram_adr(NTADR_A(i,floorLevel));
    vram_put(floorTile);
  }

  for(i = 0; i < SHADOW_SIZE; i++)
  {
    shadow[i] = 0xAA;
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
    // AA AA AA A8 -gets this
    // AA AA AA A2 -gets AA AA AA A0
    // AA AA AA 8A -gets AA AA AA 80
    // AA AA AA 2A -gets AA AA AA 00
    // AA AA A8 AA...
    */
    scrollShadow(deltaX, newData);
    scroll(world_x, world_y);

    for(i = 0; i < 5; i++)
    {
      ppu_wait_frame();
    }
  }
}
