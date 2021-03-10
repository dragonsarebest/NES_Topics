
/*
A simple "hello world" example.
Set the screen background color and palette colors.
Then write a message to the nametable.
Finally, turn on the PPU to display video.
*/

#include "neslib.h"
#include "neslib.h"

#include <stdio.h>
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



void updateMetaSprite(unsigned char attribute, unsigned char * meta)
{
  //palette, is behind, flip_hor, flip_vert
  //1 | (0 << 5) | (1 << 6) | (0 << 7);
  //0x3, 0x20, 0x40, 0x80 ^
  
  meta[3] = attribute;
  meta[7] = attribute;
  meta[11] = attribute;
  meta[15] = attribute;
  
  if(((attribute&0x40) >> 6) == 0)
  {
    //flip horizontally
    meta[0] = 0;
    meta[4] = 0;
    meta[8] = 8;
    meta[12] = 8;
  }
  else
  {
    //flip horizontally the other way
    meta[0] = 8;
    meta[4] = 8;
    meta[8] = 0;
    meta[12] = 0;
  }
  
  if(((attribute&0x80) >> 7) == 0)
  {
    //flip vertically
    meta[1] = 0;
    meta[5] = 8;
    meta[9] = 0;
    meta[13] = 8;
  }
  else
  {
    //flip vertically the other way
    meta[1] = 8;
    meta[5] = 0;
    meta[9] = 8;
    meta[13] = 0;
  }
}

typedef struct Actor
{
  unsigned char x;
  unsigned char y;
  char dx;
  char dy;
  unsigned char attribute;
} Actor;

typedef struct SpriteActor
{
  Actor act;
  unsigned char sprite;
} SpriteActor;

typedef struct MetaActor
{
  Actor act;
  unsigned char * metasprite;
} MetaActor;

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

//define variables here
DEF_METASPRITE_2x2(PlayerMetaSprite, 0xD8, 0);
MetaActor player;

DEF_METASPRITE_2x2(DoorMetaSprite, 0xC4, 0);
MetaActor Door;

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

// main function, run after console reset
void main(void) {
  
  //palette, behind, vert, horz
  unsigned int walk_timer = 5;
  unsigned int walk_wait = 0;
  unsigned int door_timer = 60*4;
  unsigned int door_wait = 0;
  
  unsigned char floorLevel = 20;
  unsigned char floorTile = 0xc0;
  
  unsigned int i = 0, j = 0;
  bool touchedDoor = false;
  
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
  
  Door.act.x = 29*8;
  Door.act.y = 18*8;
  Door.act.dx = 0;
  Door.act.dy = 0;
  Door.act.attribute = 1 | (1 << 5) | (0 << 6) | (0 << 7);
  updateMetaSprite(Door.act.attribute, DoorMetaSprite);
  
  player.act.x = 30;
  player.act.y = Door.act.y;
  player.act.dx = 1;
  player.act.dy = 0;
  player.act.attribute = 1 | (0 << 5) | (0 << 6) | (0 << 7);
  updateMetaSprite(player.act.attribute, PlayerMetaSprite);

  // enable PPU rendering (turn on screen)
  ppu_on_all();

  //always updates @ 60fps
  while (1)
  {
    //game loop
    char cur_oam = 0; // max of 64 sprites on screen @ once w/out flickering
    
    //x, y, sprite number, attribute, oam = sprite number
    //cur_oam = oam_spr(x, y, 0x19 ,attrib, cur_oam);
    
    cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite);
    
    cur_oam = oam_meta_spr(Door.act.x, Door.act.y, cur_oam, DoorMetaSprite);
    
    walk_wait = (walk_wait+1) % walk_timer;
    //only updates every walk_timer num of frames
    
    
    if(doIHit(player.act.x, player.act.y, Door.act.x+8, Door.act.y))
    {
      player.act.attribute = (player.act.attribute & 0xBF) | (1 << 6);
      //1011 1111 -> removes the current value there
      updateMetaSprite(player.act.attribute, PlayerMetaSprite);
      
      player.act.dx = -1;
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
    
    if(doIHit(player.act.x, player.act.y, 0, player.act.y))
    {
      /*
      char str[64];
      sprintf(str, "%d, %d, %d, %d",((player.act.attribute&0x3)), ((player.act.attribute&0x20) >> 5), ((player.act.attribute&0x40) >> 6), ((player.act.attribute&0x80) >> 7));
      updateScreen(2, 5, str, 12);
      */

      player.act.attribute = (player.act.attribute & 0xBF);
      //1011 1111 -> removes the current value there (should turn 1 to 0)
      updateMetaSprite(player.act.attribute, PlayerMetaSprite);
      
      player.act.dx = 1;
    }

    if(walk_wait == 0)
    {
      player.act.x += player.act.dx;
      player.act.y += player.act.dy;
    }
    
    //this makes it wait one frame in between updates
    //ppu_wait_frame();
  }
}
