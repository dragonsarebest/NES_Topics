/*
  game idea:
  you're storming a castle and can break blocks and other objects -> they explode into chunks
  I will need to sprinkle in some larger chunks too.....
  */


#define NES_MIRRORING 1 //(1 = "vertical", 0 = "horizontal")
//Nametable A starts at 0x2000, Nametable B starts at 0x2400

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

#define SELECTED 0xB0

// define a 2x2 metasprite
#define DEF_METASPRITE_2x2(name, code, attribute)\
  unsigned char name[]={\
          0,      0,      (code)+0,   attribute, \
          0,      8,      (code)+1,   attribute, \
          8,      0,      (code)+2,   attribute, \
          8,      8,      (code)+3,   attribute, \
          128};


#define WORLD_WIDTH 240*2
#define WORLD_HEIGHT 256

#define NUM_SHADOW_ROW 30
#define NUM_SHADOW_COL 32

#define BYTE_PER_COL 32

byte outsideHelper; //used for debugging
byte outsideHelper2;
byte outsideHelper3;
byte outsideHelper4;
byte outsideHelper5;

#define SHADOW_SIZE (NUM_SHADOW_ROW * NUM_SHADOW_COL)/4
char shadow[SHADOW_SIZE];

byte outsideHelper6;

#define NumWorlds 2
#define LargestWorld NUM_SHADOW_ROW * NUM_SHADOW_COL
//960 is the largest map size...
const char worldData[NumWorlds][LargestWorld] = {
  {
    0x01,0x00,0x01,0x2f,0x0f,0x0f,0x00,0x01,0x1c,0x0f,0x01,0x02,0x00,0x01,0x07,0x03,
    0x03,0x00,0x01,0x12,0x0f,0x0e,0x0f,0x0f,0x00,0x01,0x06,0x03,0x03,0x00,0x01,0x12,
    0x0f,0x0e,0x0e,0x0f,0x00,0x01,0x05,0x03,0x01,0x02,0x00,0x01,0x12,0x0f,0x0e,0x0e,
    0x0f,0x0f,0x00,0x01,0x04,0x03,0x01,0x03,0x00,0x01,0x11,0x0f,0x0e,0x01,0x02,0x0f,
    0x0f,0x00,0x01,0x02,0x03,0x01,0x04,0x00,0x01,0x11,0x0f,0x0e,0x01,0x03,0x0f,0x00,
    0x01,0x02,0x03,0x03,0x0f,0x01,0x02,0x00,0x01,0x11,0x0f,0x0e,0x01,0x03,0x0f,0x00,
    0x01,0x02,0x03,0x03,0x0f,0x0e,0x0f,0x0f,0x00,0x01,0x10,0x0f,0x0e,0x01,0x03,0x0f,
    0x00,0x00,0x03,0x03,0x02,0x0f,0x0e,0x0e,0x0f,0x0f,0x00,0x01,0x0f,0x0f,0x0f,0x0e,
    0x01,0x02,0x0f,0x00,0x03,0x03,0x02,0x03,0x0f,0x0e,0x01,0x02,0x0f,0x00,0x01,0x10,
    0x0f,0x0e,0x01,0x02,0x0f,0x03,0x01,0x02,0x02,0x02,0x0f,0x0e,0x01,0x02,0x0f,0x00,
    0x01,0x10,0x0f,0x0e,0x01,0x02,0x0f,0x02,0x03,0x02,0x01,0x02,0x0f,0x0e,0x01,0x02,
    0x0f,0x00,0x01,0x10,0x0f,0x0e,0x01,0x02,0x0f,0x02,0x01,0x04,0x0f,0x0e,0x01,0x02,
    0x0f,0x00,0x01,0x0f,0x03,0x0f,0x0e,0x01,0x02,0x0f,0x01,0x06,0x0e,0x01,0x02,0x0f,
    0x00,0x01,0x0e,0x03,0x03,0x0f,0x0e,0x01,0x0c,0x0f,0x00,0x01,0x0e,0x03,0x03,0x0f,
    0x0e,0x01,0x0b,0x0f,0x0f,0x00,0x01,0x0e,0x03,0x03,0x0f,0x0e,0x01,0x0b,0x0f,0x02,
    0x00,0x01,0x0d,0x03,0x02,0x03,0x0f,0x0e,0x01,0x04,0xc4,0xc6,0x0e,0x01,0x04,0x0f,
    0x02,0x00,0x01,0x04,0x03,0x03,0x00,0x01,0x06,0x02,0x03,0x02,0x0f,0x0e,0x01,0x04,
    0xc5,0xc7,0x0e,0x01,0x04,0x0f,0x02,0x00,0x01,0x04,0x03,0x03,0x00,0x01,0x05,0x02,
    0x01,0x03,0xc1,0x01,0x0c,0x0f,0x02,0x00,0x01,0x03,0x03,0x01,0x05,0x00,0x01,0x02,
    0x02,0x01,0x02,0xc1,0x01,0x0e,0x02,0x00,0x01,0x03,0x03,0x02,0x02,0x03,0x01,0x03,
    0x00,0x00,0x02,0xc1,0x01,0x10,0x02,0x00,0x01,0x03,0x02,0x03,0x02,0x01,0x03,0x03,
    0x03,0x02,0x02,0xc1,0x01,0x11,0x00,0x01,0x02,0x02,0x01,0x09,0xc1,0x01,0x12,0x00,
    0x01,0x02,0x02,0x01,0x06,0xc1,0x01,0x15,0x00,0x00,0x02,0x01,0x02,0xc1,0x01,0x1a,
    0x00,0x00,0xc0,0x01,0x1d,0x00,0x01,0x3f,0x00,0x01,0x00
    },

  {
    0x01,0x00,0x01,0x20,0xc1,0xc1,0x00,0x01,0x1d,0xc1,0xc1,0x00,0x01,0x12,0x0a,0x01,
    0x04,0x00,0x01,0x05,0xc1,0xc1,0x00,0x01,0x0e,0x0a,0x01,0x09,0x00,0x01,0x04,0xc1,
    0xc1,0x00,0x01,0x08,0x0a,0x01,0x06,0x02,0x0a,0x01,0x07,0x00,0x01,0x04,0xc1,0xc1,
    0x00,0x01,0x07,0x0a,0x01,0x07,0x02,0x0a,0x01,0x07,0x00,0x01,0x04,0xc1,0xc1,0x00,
    0x01,0x07,0x0a,0x01,0x10,0x00,0x01,0x04,0xc1,0xc1,0x00,0x01,0x07,0x0a,0x01,0x03,
    0x02,0x0a,0x01,0x06,0x02,0x0a,0x01,0x04,0x00,0x01,0x03,0xc1,0xc1,0x00,0x01,0x07,
    0x0a,0x01,0x03,0x02,0x0a,0x01,0x05,0x02,0x02,0x0a,0x01,0x04,0x00,0x01,0x03,0xc1,
    0xc1,0x00,0x01,0x07,0x0a,0x01,0x04,0x02,0x0a,0x02,0x0a,0x0a,0x02,0x02,0x0a,0x01,
    0x05,0x00,0x01,0x03,0xc1,0xc1,0x00,0x01,0x08,0x0a,0x01,0x03,0x02,0x01,0x02,0x0a,
    0x02,0x02,0x0a,0x01,0x05,0x00,0x01,0x04,0xc1,0xc1,0x00,0x01,0x09,0x0a,0x01,0x03,
    0x02,0x01,0x03,0x0a,0x01,0x04,0x00,0x01,0x06,0xc1,0xc1,0x00,0x01,0x0d,0x02,0x01,
    0x03,0x00,0x01,0x0b,0xc1,0xc1,0x00,0x01,0x0e,0x02,0x02,0x00,0x01,0x0c,0xc1,0xc1,
    0x00,0x01,0x0e,0x02,0x02,0x00,0x01,0x0c,0xc1,0xc1,0x00,0x01,0x0e,0x02,0x02,0x00,
    0x01,0x0c,0xc1,0xc1,0x00,0x01,0x0e,0x02,0x02,0x00,0x01,0x0c,0xc1,0xc1,0x00,0x01,
    0x0e,0x02,0x02,0x00,0x01,0x0c,0xc1,0xc1,0x00,0x01,0x0e,0x02,0x02,0x00,0x01,0x1d,
    0x02,0x02,0x00,0x01,0x1a,0x0f,0x01,0x03,0x02,0x00,0x01,0x19,0x0f,0x01,0x0a,0x00,
    0x01,0x12,0x0f,0x01,0x03,0xc1,0x0f,0xc1,0xc1,0x0f,0x01,0x06,0x00,0x01,0x05,0xc1,
    0xc1,0x00,0x01,0x07,0x0f,0x01,0x03,0xc1,0x01,0x08,0x0f,0x01,0x04,0x00,0x01,0x03,
    0xc1,0xc1,0x00,0x01,0x04,0x0f,0x01,0x05,0xc1,0xc1,0x0f,0x01,0x06,0xc1,0x0f,0x01,
    0x05,0x00,0x01,0x02,0xc4,0xc6,0x00,0x01,0x02,0x0f,0x01,0x06,0xc1,0xc1,0x0f,0x01,
    0x07,0xc1,0x01,0x02,0x0f,0x01,0x04,0x00,0x00,0xc5,0xc7,0x00,0x0f,0x01,0x08,0xc1,
    0x0f,0x01,0x0a,0xc1,0x0f,0x01,0x04,0x00,0x00,0xc0,0x01,0x1d,0x00,0x01,0x3f,0x00,
    0x01,0x00
    }
};
char worldNumber;
char transition;
byte scrollSwap;
byte worldScrolling;

word setVRAMAddress(int x, int y, byte setNow )
{
  word addr;
  if(scrollSwap == true)
  {
    addr = NTADR_B(x,y);
  }
  else
  {
    addr = NTADR_A(x, y);
  }
  if(setNow)
  {
    vram_adr(addr);
  }
  return addr;
}

byte getchar_vram(byte x, byte y) {
  // compute VRAM read address
  word addr;
  byte rd;

  addr = setVRAMAddress(x, y, false);
  // result goes into rd

  // wait for VBLANK to start
  ppu_wait_nmi();
  //ppu_off();
  // set vram address and read byte into rd
  vram_adr(addr);
  vram_read(&rd, 1);
  // scroll registers are corrupt
  // fix by setting vram address

  //setVRAMAddress(0, 0, true);
  vram_adr(0x00);

  //ppu_on_all();
  return rd;
}

void setGround(unsigned char x, unsigned char y, byte placeMe)
{
  //placeMe is the replacement 2 bits of data
  int bytenum;
  int remainder;
  byte mask = 0xFF;
  int bitNum;

  //if we are NOT scrolling....
  //x -= world_x;
  //y -= world_y;
  bitNum = (y*BYTE_PER_COL*2 + x*2);
  //x & y = 0->32. row & column = 0->8
  bytenum = bitNum >> 3;//same as dividing by 8...
  remainder = (bitNum & 0x07); //the lost 3 bits from divison
  //go remainder number of bits into shadow

  //remainder is 0->8
  mask = mask ^ (0x03 << (remainder));
  //lower bit major. tile4, tile3, tile2, tile1

  //mask = mask ^ (0xC0 >> (remainder));
  //higher bit major tile1, tile2, tile3, tile4...

  shadow[bytenum] = ((shadow[bytenum] & mask) | (placeMe << remainder));
}

short checkGround(unsigned char x, unsigned char y, byte val)
{
  //val = 0 if you want the 1st value and  = 1 if you want the 2nd
  // 1 = ground, 0 = breakable
  int bytenum;
  int remainder;
  int value;
  int bitNum;

  //x -= world_x;
  //y -= world_y;
  bitNum = (y*BYTE_PER_COL*2 + x*2);
  //x & y are tile pos
  //bitNum = (y/4) * NUM_SHADOW_COL + (x/4);
  bytenum = bitNum >> 3;//same as dividing by 8...
  remainder = (bitNum & 0x07) + val; //the lost 3 bits from divison
  //go remainder number of bits into shadow

  //remainder = 0, 2, 4, 6 or 1, 3, 5, 7
  //xx11 1111
  //0-> 6, 2->4, 4->2, 6->0

  //value = (shadow[bytenum] >> (6-remainder) ) & 0x01;
  value = (shadow[bytenum] >> (remainder) ) & 0x01;
  return value;
}

byte searchPlayer(unsigned char x, unsigned char y, byte groundOrBreak, byte facingRight, byte offset)
{
  byte inFront = 0;
  int collision;
  //if checking for collision = 1 .. look forward 1 less when lookung right
  //char infront = checkGround((x/8)+ (facingRight*3 -1), ((y)/8), groundOrBreak);
  //char infront2 = checkGround((x/8)+ (facingRight*3 -1), ((y)/8)+1, groundOrBreak);

  if(facingRight)
  {
    collision = -2;
  }
  else
  {
    collision = 1;
  }
  // feet.act.x = ((player.act.x/8)+(lastFacingRight*3 - 1) + collision) * 8;
  // feet.act.y = (player.act.y/8) *8;
  inFront = checkGround(((x/8 + offset)+(facingRight*3 - 1) + collision), ((y)/8), groundOrBreak);

  inFront = inFront | checkGround(((x/8 + offset)+(facingRight*3 - 1) + collision), ((y)/8)+1, groundOrBreak) << 1;

  if(facingRight)
  {
    collision = -1;
  }
  else
  {
    collision = 2;
  }

  inFront = inFront | checkGround(((x/8 + offset)+(facingRight*3 - 1) + collision), ((y)/8), groundOrBreak) << 2;

  inFront = inFront | checkGround(((x/8 + offset)+(facingRight*3 - 1) + collision), ((y)/8)+1, groundOrBreak) << 3;

  //inFront = inFront | checkGround((x/8)+(facingRight*3 - 1 - collision*(!facingRight)), ((y)/8), groundOrBreak) << 2;
  //inFront = inFront | checkGround((x/8)+(facingRight*3 - 1 - collision*(!facingRight)), ((y)/8)+1, groundOrBreak) << 3;

  return inFront;
}

byte aboveOrBellowPlayer(unsigned char x, unsigned char y, byte groundOrBreak, byte above, byte lastFacingRight, byte offset)
{
  byte inFront = 0;
  int deltaY = 0;
  int deltaX = 0;
  if(above)
  {
    deltaY = -1;
  }
  else
  {
    deltaY = 2;
  }
  if(x % 8 != 0)
  {
    if(lastFacingRight)
    {
      deltaX = 1;
    }
  }
  else
  {
    if(!lastFacingRight)
    {
      deltaX = 1;
    }
  }

  deltaY += offset;

  inFront = checkGround((x/8) + deltaX, (y/8) + deltaY, groundOrBreak);
  inFront = inFront | checkGround((x/8) + 1 + deltaX, (y/8) + deltaY, groundOrBreak) << 1;
  return inFront;
}

byte UpTo8DoorsOpen = 0;
//8*2char's 
char DoorInfo[8];
char StairsGoToWorld[8];
char DoorPositions[16];
byte numDoors;
//each bit represents one door, ordered left to right up down.

byte countUp(char lookForMe, byte useDoorLocation, char * DoorInfo)
{
  byte count = 0;
  byte current[NUM_SHADOW_COL];
  byte doorCount = 0;
  int x, y;
  word addr;

  //ppu_wait_nmi();
  addr = setVRAMAddress(0, 0, true);

  for(y = 0; y < NUM_SHADOW_ROW; y++)
  {
    vram_read(current, NUM_SHADOW_COL);
    setVRAMAddress(0, y, true);
    //addr += NUM_SHADOW_COL;

    for(x = 0; x < NUM_SHADOW_COL; x++)
    {
      if(current[x] == lookForMe)
      {
        if(useDoorLocation)
        {
          //doorCount = a num 0->8
          byte temp = (doorCount << 4) | 0x04;

          DoorPositions[doorCount*2] = x;
          DoorPositions[doorCount*2+1] = y;

          DoorInfo[doorCount++] = temp;
        }
        count++;
      }
    }
  }
  setVRAMAddress(0,0,true);
  return count;
}

void updateScreen(unsigned char column, unsigned char row, char * buffer, unsigned char num_bytes)
{
  vrambuf_clear();
  set_vram_update(updbuf);
  if(scrollSwap)
  {
    vrambuf_put(NTADR_B(column, row), buffer, num_bytes);
  }
  else
  {
    vrambuf_put(NTADR_A(column, row), buffer, num_bytes);
  }

  vrambuf_flush();
}

void writeBinary(unsigned char x, unsigned char y, byte value)
{
  char dx[16];
  //sprintf(dx, "%d", player.act.dx);
  sprintf(dx, "%d %d %d %d %d %d %d %d", (value&0x80)>>7, (value&0x40)>>6, (value&0x20) >>5, (value&0x10)>>4, (value&0x08)>>3, (value&0x04)>>2, (value&0x02)>>1, value&0x01);
  updateScreen(x, y, dx, 16);
}

void debugnameTable()
{
  char dx[32];
  word addr;

  if(!scrollSwap)
  {
    addr = NTADR_A(0,0);
    sprintf(dx, "NametableA: %d   ", addr);
    updateScreen(2, 2, dx, 32);
  }
  else
  {
    addr = NTADR_B(0,0); 
    sprintf(dx, "NametableB: %d   ", addr);
    updateScreen(2, 2, dx, 32);
  }


  sprintf(dx, "world #: %d        ", worldNumber);
  updateScreen(2, 3, dx, 32);

  sprintf(dx, "scrollSwap: %d     ", scrollSwap);
  updateScreen(2, 4, dx, 32);

}


void debugDisplayShadow()
{

  int rleInt, x, y, ground, breakable;

  ppu_off();

  setVRAMAddress(0, 0, true);

  for(rleInt = 0; rleInt < LargestWorld; rleInt++)
  {
    y = rleInt / NUM_SHADOW_COL; //tileNum / 32 = y value
    x = rleInt % NUM_SHADOW_COL;

    ground = checkGround(x, y, 1);
    breakable = checkGround(x, y, 0);

    if(ground && !breakable)
    {
      vram_put(0x01);
      //vram_put(0x05);
    }
    else if(breakable && ! ground)
    {
      vram_put(0x02);
    }
    else if(ground && breakable)
    {
      vram_put(0x03);
    }
    else
    {
      vram_put(0x00);
    }


  }
  ppu_on_all();
}

#define pastHereBeBlocks 0x70

byte loadWorld()
{ 
  int i = 0;
  int tileNum = 0;
  int index = 0;
  byte current[NUM_SHADOW_COL];
  byte doorCount = 0;
  word addr;
  byte temp = false;

  scrollSwap = !scrollSwap;

  if(scrollSwap)
  {
    addr = NTADR_B(0,0);
  }
  else
  {
    addr = NTADR_A(0,0);
  }

  //vrambuf_flush();
  vram_adr(addr);

  //addr = setVRAMAddress(0, 0, true);

  ppu_off();

  vram_inc(0);
  vram_unrle(worldData[worldNumber]);
  vrambuf_flush();

  //addr = setVRAMAddress(0, 0, true);
  vram_adr(addr);
  //ppu_on_all();

  worldScrolling = true;
  worldNumber = 1;
  if(temp){
    while(tileNum < LargestWorld)
    {
      int y = tileNum / NUM_SHADOW_COL; //tileNum / 32 = y value
      int x = tileNum % NUM_SHADOW_COL;

      setVRAMAddress(x, y, true);

      vram_read(current, NUM_SHADOW_COL);
      for(i = 0; i < NUM_SHADOW_COL; i++)
      {
        byte shadowBits = 0x00;
        y = tileNum / NUM_SHADOW_COL; //tileNum / 32 = y value
        x = tileNum % NUM_SHADOW_COL;

        if(current[x] == 0xC4)
        {
          //byte temp = (doorCount << 4) | 0x04;
          byte temp = 0x04;

          outsideHelper = x;
          outsideHelper2 = y;

          DoorPositions[doorCount*2] = x;
          DoorPositions[doorCount*2+1] = y;
          DoorInfo[doorCount++] = temp;
          numDoors++;
        }

        if((current[x] & 0xF0) > pastHereBeBlocks)
        {
          shadowBits = 0x03; //breakable and ground
        }

        if(current[x] == 0xc0)
        {
          shadowBits = 0x02; //just ground
        }

        if(current[x] == 0xc4 || current[x] == 0xc5 || current[x] == 0xc6 || current[x] == 0xc7)
        {
          shadowBits = 0x01; // just breakable
        }

        setGround(x, y, shadowBits);

        tileNum++;
      }


    }
  }
  vram_adr(addr);

  ppu_on_all();

  //ppu_wait_nmi();
  //debugnameTable();

  if(worldNumber == 0)
  {
    //set all world stair destinations here!
    for(i = 0; i < 8; i++)
    {
      StairsGoToWorld[i] = 1;
    }
  }
  if(worldNumber == 1)
  {
    //set all world stair destinations here!
    for(i = 0; i < 8; i++)
    {
      StairsGoToWorld[i] = 0;
    }
  }

  //numDoors = countUp(0xC4, true, DoorInfo);
  //debugDisplayShadow();


  /*{
    char dx[32];
    sprintf(dx, "%d", addr);
    updateScreen(2, 2, dx, 32);
  }*/

  return true;
}




typedef struct Particles
{
  unsigned char x;
  unsigned char y;
  int dx;
  int dy;
  unsigned char attribute;
  short lifetime;

  unsigned char sprite;
} Particles;


typedef struct Actor
{
  unsigned char x;
  unsigned char y;
  int dx;
  int dy;
  unsigned char attribute;
  unsigned char alive;
  unsigned char moveSpeed;
  unsigned char jumpSpeed;
  char jumpTimer;
  char grounded;
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

int world_x = 0;
int world_y = 0;

DEF_METASPRITE_2x2(PlayerMetaSprite, 0xD8, 0);
MetaActor player;

DEF_METASPRITE_2x2(PlayerMetaSprite_Attack_1, 0xE0, 0);
DEF_METASPRITE_2x2(PlayerMetaSprite_Attack_2, 0xE4, 0);

DEF_METASPRITE_2x2(PlayerMetaSprite_Jump, 0xE8, 0);

DEF_METASPRITE_2x2(PlayerMetaSprite_Run, 0xDC, 0);

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

#define MAX_JUMP 7
int jumpTable[MAX_JUMP] = 
{
  -4, -2, -1, 0, 1, 2, 4
  };

#define NUM_BRICKS 16

//functions defined here



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


void writeText(char * data, unsigned char addressX, unsigned char addressY)
{
  char * line = data;
  int startIndex = 0;
  int currentIndex = 0;
  //set init address

  while(*data)
  {
    //newline = 10
    if(data[0] == 10)
    {
      vram_write(line, currentIndex-startIndex);
      line = data+1;
      startIndex = currentIndex+1;
      addressY++;
      setVRAMAddress(addressX, addressY, true);
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

short rectanglesHit(unsigned char x, unsigned char y, unsigned char width, unsigned char height, unsigned char x2, unsigned char y2, unsigned char width2, unsigned char height2)
{
  if(!(x ^ x2) && !(y ^ y2))
    return true;
  //If one rectangle is on left side of other 
  if (x > x2+width2 || x2 > x+width) 
    return false; 

  // If one rectangle is above other 
  if (y > y2+height2 || y2 < y+height) 
    return false; 
  return true; 
}

void randomizeParticle(Particles * singleBricks, short brickSpeed, int x, int y)
{
  int i = 0;
  for(i = 0; i < NUM_BRICKS; i++)
  {
    Particles bck;
    int temp;
    bck.x = x + 2 * (i%4);
    bck.y = y + i%8;
    bck.attribute = 1+i%2;
    bck.lifetime = 20;

    temp = rand()%brickSpeed*2;
    if(temp > brickSpeed)
    {
      temp = - temp%brickSpeed;
    }
    bck.dx = temp;
    temp = rand()%brickSpeed*2;
    if(temp > brickSpeed)
    {
      temp = - temp%brickSpeed;
    }
    bck.dy= temp;


    temp = rand()%brickSpeed;
    if(temp > brickSpeed/2)
    {
      bck.sprite = 0x80;
    }
    else
    {
      bck.sprite = 0x81;
    }

    singleBricks[i] = bck;
  }
}

void updatePlayerSprites()
{
  updateMetaSprite(player.act.attribute, PlayerMetaSprite);
  updateMetaSprite(player.act.attribute, PlayerMetaSprite_Run);
  updateMetaSprite(player.act.attribute, PlayerMetaSprite_Attack_1);
  updateMetaSprite(player.act.attribute, PlayerMetaSprite_Attack_2);
  updateMetaSprite(player.act.attribute, PlayerMetaSprite_Jump);
}


byte old_worldScrolling = false;
void scrollWorld(byte direction, MetaActor* PlayerActor)
{
  int deltaX = 0, deltaY = 0;
  int playerDeltaX, playerDeltaY;
  byte returnVal;

  playerDeltaX = PlayerActor->act.dx * PlayerActor->act.moveSpeed;
  playerDeltaY = PlayerActor->act.dy * PlayerActor->act.jumpSpeed;

  outsideHelper = direction;

  if(old_worldScrolling == false && worldScrolling == true)
  {
    old_worldScrolling = true;


    setVRAMAddress(0,0,true);
    returnVal = loadWorld();
    ppu_wait_nmi();
    setVRAMAddress(0,0,true);

    //scrollSwap = !scrollSwap;

    //swaps it if needed... (since if you last scrolled right, now you need to scroll left...)

    if(direction == 0x02 && scrollSwap == 0)
    {
      direction == 0x01;
    }

    if(direction == 0x01 && scrollSwap == 1)
    {
      direction == 0x02;
    }

    if(direction == 0x04 && scrollSwap == 0)
    {
      direction == 0x03;
    }

    if(direction == 0x03 && scrollSwap == 1)
    {
      direction == 0x04;
    }

    if(direction != 0)
    {
      outsideHelper = direction;
    }


    if(direction == 0x02)
    {
      //right
      PlayerActor->act.x = 256-24;
    }
    else if(direction == 0x03)
    {
      //up
      PlayerActor->act.y = 8;
    }
    else if(direction == 0x04)
    {
      //down
      PlayerActor->act.y = 8;
    }
    else
    {
      //left & defualt direction
      PlayerActor->act.x = 8;
    }
  }

  if(worldScrolling == false)
  {
    PlayerActor->act.x += playerDeltaX;
    PlayerActor->act.y += playerDeltaY;
  }
  else
  {
    if(direction == 0x02)
    {
      //right
      deltaX = 8;
    }
    else if(direction == 0x03)
    {
      //up
      deltaY = -8;
    }
    else if(direction == 0x04)
    {
      //down
      deltaY = 8;
    }
    else
    {
      //left & defualt direction
      direction = 0x01;
      deltaX = -8;
    }

    world_x += deltaX;
    PlayerActor->act.x -= deltaX;

    world_y += deltaY;
    PlayerActor->act.y -= deltaY;

    //ppu_wait_nmi();
    scroll(world_x, world_y);
    //split(world_x, 0);

    if(direction == 0x01)
    {
      //left
      if(world_x <= -256)
      {
        world_x = 0;
        world_y = 0;
        old_worldScrolling = false;
        player.act.x = 256-24;
        worldScrolling = false;
        //world_x = 0;
      }
    }
    else if(direction == 0x02)
    {
      //right
      if(world_x >= 256)
      {
        world_x = 0;
        world_y = 0;
        old_worldScrolling = false;
        player.act.x = 8;
        worldScrolling = false;
        //world_x = 256;
      }

    }
    else if(direction == 0x03)
    {
      //up
      if(world_y <= -256)
      {
        world_x = 0;
        world_y = 0;
        old_worldScrolling = false;
        player.act.x = 8;
        worldScrolling = false;
        //world_y = 0;
      }
    }
    else if(direction == 0x04)
    {
      //down
      if(world_y >= 256)
      {
        world_x = 0;
        world_y = 0;
        old_worldScrolling = false;
        player.act.x = 8;
        worldScrolling = false;
        //world_y = 0;
      }
    }
  }

  //old_worldScrolling = worldScrolling;
}


char selectedPosition[3];
const char playerPlaceBlock = 0xC0;

void setSelectedPosition(int playerX, int playerY, int upOffset, byte lastFacingRight, int offset, byte findEmpty)
{
  byte breakBlock = searchPlayer(playerX, playerY + (upOffset*8), 0, lastFacingRight, offset);
  int itemX, itemY, collision;
  byte suitableOption = true;

  selectedPosition[2] = 1;

  if(lastFacingRight)
  {
    collision = -2;
  }
  else
  {
    collision = 1;
  }

  if((breakBlock & 0x02) != findEmpty)
  {
    itemX = ((playerX/8)+(lastFacingRight*3 - 1) + collision + offset);
    itemY = playerY/8 + 1;
  }
  else if((breakBlock & 0x01) != findEmpty)
  {
    itemX = ((playerX/8)+(lastFacingRight*3 - 1) + collision + offset);
    itemY = playerY/8;
  }
  else
  {
    if(lastFacingRight)
    {
      collision = -1;
    }
    else
    {
      collision = 2;
    }

    if((breakBlock & 0x08) != findEmpty)
    {
      itemX = ((playerX/8)+(lastFacingRight*3 - 1) + collision + offset);
      itemY = playerY/8 + 1;
    }
    else if((breakBlock & 0x04) != findEmpty)
    {
      itemX = ((playerX/8)+(lastFacingRight*3 - 1) + collision + offset);
      itemY = playerY/8;
    }
    else
    {
      suitableOption = false; 
    }
  }

  itemY += upOffset;

  if(suitableOption == false)
  {
    selectedPosition[2] = 0;

    if(lastFacingRight)
    {
      collision = -2;
    }
    else
    {
      collision = 1;
    }
    //lastFacingRight == true/1 -> 0, lastFacingRight == false/0 -> -1.. lastFacingRight-1 
    itemX = ((playerX/8)+(lastFacingRight*3 - 1) + collision + offset + (lastFacingRight - 1));
    itemY = playerY/8 + 1 + upOffset;
  }
  selectedPosition[0] = itemX;
  selectedPosition[1] = itemY;
}

char groundBlock[6];

// main function, run after console reset
void main(void) {

  char cur_oam;
  char stausX = 2;
  char stausY = 2;
  char statusVal = 0xA0;

  unsigned char floorLevel = 20;
  unsigned char floorTile = 0xc0;

  byte lastFacingRight = true;
  byte playerInAir = true; //since we start off the ground
  byte run = 0;
  byte swing = 0;
  byte timeBetweenFall = 2;
  byte fallTimer = 0;

  SpriteActor feet;

  byte doorsDirty = false;

  Particles singleBricks[NUM_BRICKS];
  unsigned char numActive = 0;

  short brickSpeed = 5;
  short brickLifetime = 10;
  short digTimer = 0;
  short digWait = 2;

  unsigned int i = 0, j = 0;

  char debugCheck = false;

  byte Up_Down = 0;
  byte lastTouch = 0;

  worldScrolling = false;
  scrollSwap = !worldScrolling;
  old_worldScrolling = worldScrolling;
  worldNumber = 0;
  transition = 0x01;

  pal_all(PALETTE);// generally before game loop (in main)

  feet.act.x = 0;
  feet.act.y = 0;
  feet.sprite = 0x0F;

  player.act.x = 8*1;
  player.act.y = 23* 8;

  player.act.dx = 0;
  player.act.dy = 0;
  player.act.attribute = 1 | (0 << 5) | (0 << 6) | (0 << 7);

  player.act.alive = true;
  player.act.moveSpeed = 1;
  player.act.jumpSpeed = 4;
  player.act.grounded = true;

  if(playerInAir)
  {
    player.act.dy = 1;
  }


  randomizeParticle(singleBricks, brickSpeed, 0, 0);
  // enable PPU rendeing (turn on screen)


  loadWorld();
  
  
  
  
  updatePlayerSprites();
  //debugDisplayShadow();

  ppu_on_all();

  //always updates @ 60fps
  while (1)
  {
    int noBlocksAbove;
    byte jumping;
    byte breaking;

    int res = 0;
    char breakBlock;
    char pad_result= pad_poll(0) | pad_poll(1);

    //oam_clear();
    cur_oam = oam_spr(stausX, stausY, 0xA0, 0, 0);
    //cur_oam = 4;


    player.act.dx = ((pad_result & 0x80) >> 7) + -1 * ((pad_result & 0x40) >> 6) ;
    res = searchPlayer(player.act.x, player.act.y, 1, lastFacingRight, 0);

    if(worldScrolling)
    {
      player.act.dx = 0;
      player.act.dy = 0;
    }
    else
    {
      if(((res & 0x0C) >> 2) != 0 && lastFacingRight || ((res & 0x03)) != 0 && !lastFacingRight )
      {
        /*
        int collision;


        {
          //1,2,4,8 bits 
          //1 & 2 = front 2
          //4 & 8 = back 2
          if(((res & 0x0C) >> 2) > 0)
          {
            //meaning we're moving right and the player's front 2 blocks are IN ground block...
            if(lastFacingRight)
            {
              //collision = -2;
              collision = -2;
            }
            else
            {
              collision = 1;
            }
            player.act.x = ((player.act.x/8)+(lastFacingRight*3 - 1) + collision)*8;
          }
        }
        */

        player.act.dx = 0;
      }

      res = 0;

      for(i = 0; i < 6; i++)
      {

        if(i >= 4 && player.act.dy == 0)
        {
          groundBlock[i] = 0;
        }
        else
        {
          if(!(player.act.dy > 0 && playerInAir) && (i == 2 || i == 3))
          {
            groundBlock[i] = 0;
          }
          else if(! (player.act.dy * player.act.jumpSpeed >= 8) && (i == 4 || i == 5))
          {
            groundBlock[i] = 0;
          }
          else
          {
            groundBlock[i] = checkGround((player.act.x/8) + (i%2), ((player.act.y)/8)+2 + (i/2), 1);
          }

        }
        //writeBinary(2, 8+i, groundBlock[i]);

        res = res | groundBlock[i];
      }


      if(res != 0)
      {
        player.act.grounded = true;
      }
      else
      {
        player.act.grounded = false;
        if(player.act.grounded == false && playerInAir == false)
        {
          //start falling! we walk off a block and dont jump
          player.act.jumpTimer = MAX_JUMP/2;
          playerInAir = true;
          fallTimer = 0;
        }
      }



      if((pad_result & 0x80)>>7 && lastFacingRight == false)
      {
        lastFacingRight = true;
      }
      else
      {
        if((pad_result & 0x40)>>6 && lastFacingRight)
        {
          lastFacingRight = false;
        }
      }
      {
        player.act.attribute = (player.act.attribute & 0xBF) | (!lastFacingRight  << 6);
        updatePlayerSprites();
      }


      {
        byte shift = pad_result & 0x02;
        jumping = pad_result & 0x10;
        breaking = pad_result & 0x04;
        noBlocksAbove = aboveOrBellowPlayer(player.act.x, player.act.y, 1, true, lastFacingRight, 0);

        if(breaking != 0 || swing != 0 || shift != 0)
        {
          jumping = 0;
          //cannot jump & break @ same time
        }



        //Up_Down = 0;
        if(shift)
        {
          if((pad_result & 0x20))
          {
            noBlocksAbove = aboveOrBellowPlayer(player.act.x, player.act.y, 1, false, lastFacingRight, 0);
            if(noBlocksAbove != 0)
            {
              //pressing down on keypad
              Up_Down = 0x01;
              lastTouch = 10;
            }
          }
          else if(((pad_result & 0x10)))
          {
            //pressing up but cant jump.... look up instead!
            Up_Down = 0x02; //0x01 = up, 0x02 = down, 0x00 = neither
            lastTouch = 10;
          }
          else if(lastTouch <= 0)
          {
            Up_Down = 0x00; 
          }

        }
        if(lastTouch > 0)
        {
          lastTouch--;
        }
        //writeBinary(2, 5, pad_result);
        //writeBinary(2, 2, Up_Down);
      }


      if(numActive)
        for(i = 0; i < NUM_BRICKS; i++)
        {
          if(singleBricks[i].lifetime > 0)
          {
            cur_oam = oam_spr(singleBricks[i].x, singleBricks[i].y, singleBricks[i].sprite ,singleBricks[i].attribute, cur_oam);
            //if(walk_wait == 0)
            {
              singleBricks[i].x = singleBricks[i].x + singleBricks[i].dx;
              singleBricks[i].y = singleBricks[i].y + singleBricks[i].dy;


              if(singleBricks[i].lifetime != 1)
              {
                singleBricks[i].lifetime--;
              }
              else
              {
                singleBricks[i].lifetime = 0;
                numActive--;
              }

            }
          }
        }



      if(!worldScrolling)
      {
        byte offset = 1;
        int upOffset = 0;
        if(Up_Down == 0x01)
        {
          // up
          upOffset = 1;
        }
        else if(Up_Down == 0x02)
        {
          // down
          upOffset = -1;
        }
        else
        {
          upOffset = 0;
        }
        setSelectedPosition(player.act.x, player.act.y, upOffset, lastFacingRight, offset, 0);

        if(swing == 0)
        {

          breakBlock = searchPlayer(player.act.x, player.act.y + (upOffset*8), 0, lastFacingRight, offset);
          //1101
          if(breaking)
          {
            swing = 1;
          }
          if((breakBlock != 0) && breaking)
          {

            int itemX, itemY, collision;
            byte suitableOption = true;
            //breakblock = option4 bit, option3 bit, option2 bit, option1 bit...
            outsideHelper = breakBlock;

            if(lastFacingRight)
            {
              collision = -2;
            }
            else
            {
              collision = 1;
            }

            if((breakBlock & 0x02) != 0)
            {
              itemX = ((player.act.x/8)+(lastFacingRight*3 - 1) + collision + offset);
              itemY = player.act.y/8 + 1;
            }
            else if((breakBlock & 0x01) != 0)
            {
              itemX = ((player.act.x/8)+(lastFacingRight*3 - 1) + collision + offset);
              itemY = player.act.y/8;
            }
            else
            {
              if(lastFacingRight)
              {
                collision = -1;
              }
              else
              {
                collision = 2;
              }

              if((breakBlock & 0x08) != 0)
              {
                itemX = ((player.act.x/8)+(lastFacingRight*3 - 1) + collision + offset);
                itemY = player.act.y/8 + 1;
              }
              else if((breakBlock & 0x04) != 0)
              {
                itemX = ((player.act.x/8)+(lastFacingRight*3 - 1) + collision + offset);
                itemY = player.act.y/8;
              }
              else
              {
                suitableOption = false; 
              }
            }

            if(suitableOption)
            {
              itemY += upOffset;

              outsideHelper = upOffset;

              setGround(itemX, itemY, 0);

              //ppu_wait_nmi();
              ppu_off();

              //check if it's a door (c4->c7)
              //if it is replace with stair blocks
              //else put background block in
              {
                char newBlock = 0x0D;
                char oldBlock = getchar_vram(itemX, itemY);

                outsideHelper = oldBlock;

                if(oldBlock >= 0xC4 && oldBlock <= 0xC7)
                {
                  int x = itemX, y = itemY, i;
                  //0xFc -> 0xFF
                  newBlock = 0xFc + (oldBlock -  0xC4);

                  if(oldBlock == 0xC5)
                  {
                    y--;
                  }
                  if( oldBlock == 0xc6)
                  {
                    x--;
                  }
                  if(oldBlock == 0xc7)
                  {
                    x--;
                    y--;
                  }

                  //outsideHelper2 = x;
                  //outsideHelper3 = y;

                  for(i = 0; i < numDoors; i++)
                  {
                    if(x == DoorPositions[i*2] && y == DoorPositions[i*2 + 1])
                    {
                      DoorInfo[i]--;
                      if(DoorInfo[i] == 0)
                      {
                        UpTo8DoorsOpen = UpTo8DoorsOpen | 1 << i;
                        doorsDirty = true;
                        //stairwellAccessable!

                      }
                      break;
                    }
                  }

                }
                setVRAMAddress(itemX, itemY, true);

                vram_put(newBlock);
              }

              ppu_on_all();


              numActive = NUM_BRICKS;
              for(i = 0; i < numActive; i++)
              {
                singleBricks[i].lifetime = brickLifetime;
                singleBricks[i].x = itemX*8-5;
                singleBricks[i].y = itemY*8;

              }
            }
          }
        }
      }

      {
        //makes stairs go to next level!
        //char dx[32];

        //writeBinary(UpTo8DoorsOpen, 2, 2);

        //StairsGoToWorld[i]
        for(i = 0; i < numDoors; i++)
        {

          //sprintf(dx, "door: %d   ", DoorInfo[i]);
          //updateScreen(2, 3+i, dx, 32);

          if(((UpTo8DoorsOpen >> i) & 0x01) == 1)
          {
            //then these stairs are open!
            if(jumping && player.act.x/8 >= DoorPositions[i*2]-1 && player.act.x/8 <= DoorPositions[i*2]+1
               && player.act.y/8 >= DoorPositions[i*2 + 1]-1 && player.act.y/8 <= DoorPositions[i*2 + 1]+1)
            {
              jumping = false;
              //instead go to new world
              worldNumber = StairsGoToWorld[i];
              worldScrolling = true;
              //transition = 0x01;
              transition = 0x00;
            }
          }
        }
      }


      if(player.act.grounded && playerInAir)
      {

        int itemY = 0;

        if((groundBlock[0] | groundBlock[1]) && ! (groundBlock[2] | groundBlock[3]))
        {
          itemY = 0;
        }
        else
        {
          itemY = 1;
        }

        player.act.y = ((player.act.y/8) + itemY)*8;

        player.act.dy = 0;

        playerInAir = false;
        //player.act.grounded = false;
        player.act.jumpTimer = 0;

      }

      //jumping debug
      /*
      if(debugCheck){

        byte above = true;
        int deltaY = 0;
        int deltaX = 0;
        if(above)
        {
          deltaY = -1;
        }
        else
        {
          deltaY = 2;
        }
        if(player.act.x % 8 != 0)
        {
          if(lastFacingRight)
          {
            deltaX = 1;
          }
        }
        else
        {
          if(!lastFacingRight)
          {
            deltaX = 1;
          }
        }
        feet.act.x = (player.act.x/8 + deltaX);
        feet.act.y = (player.act.y/8 + deltaY);
        cur_oam = oam_spr(feet.act.x * 8, feet.act.y * 8, feet.sprite , 1, cur_oam);
        feet.act.x += 1;
        cur_oam = oam_spr(feet.act.x * 8, feet.act.y * 8, feet.sprite , 2, cur_oam);


        {
          byte result = 0;
          char dx[32];
          result = checkGround((player.act.x/8) + deltaX, (player.act.y/8) + deltaY, 1);
          sprintf(dx, "ground_01: %d   ", result);
          updateScreen(2, 7, dx, 32);

          result = checkGround((player.act.x/8) + deltaX + 1, (player.act.y/8) + deltaY, 1);
          sprintf(dx, "ground_02: %d   ", result);
          updateScreen(2, 8, dx, 32);

          result = aboveOrBellowPlayer(player.act.x, player.act.y, 1, above, lastFacingRight, 0);

          sprintf(dx, "above: %d   ", result);
          updateScreen(2, 6, dx, 32);

        }
      }
      */

      if(player.act.jumpTimer != MAX_JUMP-1 && !playerInAir)
      {


        //if(player.act.grounded == true && ((pad_result & 0x10) >> 4))
        if(player.act.grounded == true && noBlocksAbove == 0 && jumping)
        {
          //if grounded and you try to jump...
          player.act.jumpTimer = 0;
          player.act.dy = jumpTable[player.act.jumpTimer];
          player.act.jumpTimer++;
          playerInAir = true;
          fallTimer = 0;
        }

      }


      if(playerInAir && !player.act.grounded)
      {

        if(fallTimer == 0)
        {

          //going up or down in the air
          player.act.dy = jumpTable[player.act.jumpTimer];
          if(player.act.jumpTimer != MAX_JUMP-1)
            player.act.jumpTimer++;
        }

        fallTimer++;
        fallTimer = fallTimer%timeBetweenFall;

      }


      //debug

      /*
        if(debugCheck)
        {
          char dx[32];
          sprintf(dx, "grounded: %d   ", player.act.grounded);
          updateScreen(2, 6, dx, 32);

          writeBinary(2, 5, pad_result);


          //sprintf(dx, "%d", player.act.dx);
          sprintf(dx, "break block: %d", breakBlock);
          updateScreen(2, 7, dx, 32);



          //sprintf(dx, "%d", player.act.dx);
          sprintf(dx, "air: %d        ", playerInAir);
          updateScreen(2, 8, dx, 32);



          //sprintf(dx, "%d", player.act.dx);
          sprintf(dx, "timer: %d      ", player.act.jumpTimer);
          updateScreen(2, 9, dx, 32);

        }
        */

      //byte scrollWorld(byte direction, MetaActor* PlayerActor, byte worldScrolling)
      //0x01 = left, 0x02 = right, 0x03 = up, 0x04 = down
      //transition = 0x02;


      {
        //animate the player!
        byte shoudlRun = true;
        if(swing != 0)
        {
          shoudlRun = false;
          if( swing < 4)
          {
            cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite_Attack_1);
            ppu_wait_frame();
            ppu_wait_frame();
            swing++;
          }
          else if(swing >= 4 && swing < 8) 
          {
            cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite_Attack_2);
            ppu_wait_frame();
            ppu_wait_frame();
            swing++; 
          }
          else
          {
            swing = 0;
            shoudlRun = true;
          }
        }

        if(shoudlRun)
        {
          if(playerInAir)
          {
            cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite_Jump);
          }
          else if(player.act.dx != 0 && player.act.grounded && !worldScrolling)
          {
            if(run < 4)
            {
              cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite_Run);
              run++;
            }
            else if (run >= 4 && run < 8)
            {
              cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite);
              run++;
            }
            else
            {
              cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite_Run);
              run = 0;
            }
          }
          else
          {
            cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite);
          }
        }

      }


      //minning debug
      /*
      if(debugCheck)
      {

        int collision = 0;
        if(lastFacingRight)
        {
          collision = -2;
        }
        else
        {
          collision = 1;
        }
        feet.act.x = ((player.act.x/8)+(lastFacingRight*3 - 1) + collision) * 8;
        feet.act.y = (player.act.y/8) *8;
        cur_oam = oam_spr(feet.act.x, feet.act.y, feet.sprite , 2, cur_oam);

        feet.act.x = ((player.act.x/8)+(lastFacingRight*3 - 1) + collision) * 8;
        feet.act.y = ((player.act.y/8) + 1) *8;
        cur_oam = oam_spr(feet.act.x, feet.act.y, feet.sprite , 1, cur_oam);

        if(lastFacingRight)
        {
          collision = -1;
        }
        else
        {
          collision = 2;
        }

        feet.act.x = ((player.act.x/8)+(lastFacingRight*3 - 1) + collision) * 8;
        feet.act.y = (player.act.y/8) *8;
        cur_oam = oam_spr(feet.act.x, feet.act.y, feet.sprite , 1, cur_oam);

        feet.act.x = ((player.act.x/8)+(lastFacingRight*3 - 1) + collision) * 8;
        feet.act.y = (player.act.y/8 + 1) *8;
        cur_oam = oam_spr(feet.act.x, feet.act.y, feet.sprite , 1, cur_oam);

      }
      */


      {

        cur_oam = oam_spr(selectedPosition[0]*8, selectedPosition[1]*8, SELECTED, selectedPosition[2], cur_oam);

        if((pad_result&0x08)>>3)
        {
          int deltaX = 0;
          int deltaY = 0;
          char current = checkGround(selectedPosition[0], selectedPosition[1], 0) | checkGround(selectedPosition[0], selectedPosition[1],1);



          if(current != 0)
          {
            if(Up_Down == 0x00)
            {
              deltaX = (1-lastFacingRight)*2 -1;
              current = checkGround(selectedPosition[0] + deltaX, selectedPosition[1], 0) | checkGround(selectedPosition[0] + deltaX, selectedPosition[1],1);
            }
            else
            {
              if(Up_Down == 0x01)
              {
                // up
                deltaY = -1;
              }
              else if(Up_Down == 0x02)
              {
                // down
                deltaY = 1;
              }
              current = checkGround(selectedPosition[0], selectedPosition[1]+deltaY, 0) | checkGround(selectedPosition[0], selectedPosition[1]+deltaY,1);
            }
          }
          if(current == 0)
          {
            if(digTimer <= 0)
            {
              ppu_off();
              //ppu_wait_nmi();
              setVRAMAddress(selectedPosition[0] + deltaX, selectedPosition[1]+deltaY, true);
              vram_put(playerPlaceBlock);
              ppu_on_all();
              setGround(selectedPosition[0] + deltaX, selectedPosition[1]+deltaY, 0x03);

              digTimer = digWait;
            }
            else
            {
              digTimer--;
            }

          }
        }
      }

    }
    scrollWorld(transition, &player);

    oam_hide_rest(cur_oam);
    //this makes it wait one frame in between updates
    ppu_wait_frame();
  }
}
