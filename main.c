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

#define NUM_SHADOW_ROW 30
#define NUM_SHADOW_COL 32

#define BYTE_PER_COL 32

#define SHADOW_SIZE (NUM_SHADOW_ROW * NUM_SHADOW_COL)/4
char shadow[SHADOW_SIZE];

byte outsideHelper; //used for debugging
byte outsideHelper2;
byte outsideHelper3;
byte outsideHelper4;
byte outsideHelper5;
byte outsideHelper6;

#define NumWorlds 2
#define LargestWorld NUM_SHADOW_ROW * NUM_SHADOW_COL
//960 is the largest map size...
const char worldData[NumWorlds][LargestWorld] = {
  {
    0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,
    0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,
    0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x00,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,
    0x0e,0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x00,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,
    0x0e,0x0e,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x03,0x00,0x00,0x00,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,
    0x0e,0x0e,0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x03,0x03,0x00,0x00,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,
    0x0e,0x0e,0x0e,0x0f,0x0f,0x00,0x00,0x00,0x03,0x03,0x03,0x03,0x03,0x00,0x00,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,
    0x0e,0x0e,0x0e,0x0e,0x0f,0x00,0x00,0x00,0x03,0x03,0x0f,0x0f,0x0f,0x00,0x00,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,
    0x0e,0x0e,0x0e,0x0e,0x0f,0x00,0x00,0x00,0x03,0x03,0x0f,0x0e,0x0f,0x0f,0x00,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,
    0x0e,0x0e,0x0e,0x0e,0x0f,0x00,0x00,0x03,0x03,0x02,0x0f,0x0e,0x0e,0x0f,0x0f,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,
    0x0f,0x0e,0x0e,0x0e,0x0f,0x00,0x03,0x03,0x02,0x03,0x0f,0x0e,0x0e,0x0e,0x0f,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x0f,0x0e,0x0e,0x0e,0x0f,0x03,0x03,0x03,0x02,0x02,0x0f,0x0e,0x0e,0x0e,0x0f,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x0f,0x0e,0x0e,0x0e,0x0f,0x02,0x03,0x02,0x02,0x02,0x0f,0x0e,0x0e,0x0e,0x0f,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x0f,0x0e,0x0e,0x0e,0x0f,0x02,0x02,0x02,0x02,0x02,0x0f,0x0e,0x0e,0x0e,0x0f,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,
    0x0f,0x0e,0x0e,0x0e,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0e,0x0e,0x0e,0x0f,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,
    0x0f,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0f,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,
    0x0f,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0f,0x0f,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,
    0x0f,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0f,0x02,0x7e,
    0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x02,0x03,
    0x0f,0x0e,0x0e,0x0e,0x0e,0x0e,0xc4,0xc6,0x0e,0x0e,0x0e,0x0e,0x0e,0x0f,0x02,0x7e,
    0x7e,0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x03,0x02,
    0x0f,0x0e,0x0e,0x0e,0x0e,0x0e,0xc5,0xc7,0x0e,0x0e,0x0e,0x0e,0x0e,0x0f,0x02,0x7e,
    0x7e,0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x02,0x02,
    0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0x0f,0x02,0x7e,
    0x7e,0x00,0x00,0x03,0x03,0x03,0x03,0x03,0x03,0x00,0x00,0x00,0x02,0x19,0x02,0xc1,
    0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0x02,0x7e,
    0x7e,0x00,0x00,0x03,0x02,0x02,0x03,0x03,0x03,0x03,0x00,0x00,0x02,0xc1,0xc1,0xc1,
    0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0x02,0x7e,
    0x7e,0x00,0x00,0x02,0x03,0x02,0x02,0x02,0x02,0x03,0x03,0x02,0x02,0xc1,0xc1,0xc1,
    0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0x7e,
    0x7e,0x00,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0xc1,0xc1,0xc1,0xc1,
    0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0x7e,
    0x7e,0x00,0x02,0x02,0x02,0x02,0x02,0x02,0x19,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,
    0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0x7e,
    0x7e,0x02,0x02,0x02,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,
    0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0xc1,0x7e,
    0x7e,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,
    0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0x7e,
    0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,
    0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,
    0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,
    0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e
    },

  {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x0a,0x0a,0x0a,0x0a,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x02,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x02,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x0a,0x0a,0x0a,0x02,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x02,0x0a,0x0a,0x0a,0x0a,0x0a,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x0a,0x0a,0x0a,0x02,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x02,0x02,0x0a,0x0a,0x0a,0x0a,0x0a,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x0a,0x0a,0x0a,0x0a,0x02,0x0a,0x02,0x0a,0x0a,0x02,0x02,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x0a,0x0a,0x0a,0x02,0x02,0x02,0x0a,0x02,0x02,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0a,0x0a,0x0a,0x0a,0x02,0x02,0x02,0x02,0x0a,0x0a,0x0a,0x0a,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,0x0f,0x0f,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,
    0x00,0x00,0x00,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
    0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    }
};
char worldNumber;
char transition = 0x80; //first bit = right, left, up, down

byte getchar_vram(byte x, byte y) {
  // compute VRAM read address
  word addr = NTADR_A(x,y);
  // result goes into rd
  byte rd;
  // wait for VBLANK to start
  ppu_wait_nmi();
  // set vram address and read byte into rd
  vram_adr(addr);
  vram_read(&rd, 1);
  // scroll registers are corrupt
  // fix by setting vram address
  vram_adr(0x0);
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

  //(orignalNumber & 0x??) | (byteToPlace  << remainder);
  //remainder is 0->8
  mask = mask ^ (0x03 << (remainder));
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

  //if(shadow[bytenum] != 0)
  {
    outsideHelper = x;
    outsideHelper2 = y;
    outsideHelper3 = remainder;
    outsideHelper4 = shadow[bytenum];
    outsideHelper5 = (shadow[bytenum] >> remainder);
    outsideHelper6 = (shadow[bytenum] >> remainder) & 0x01;
  }
  value = (shadow[bytenum] >> remainder) & 0x01;
  return value;
}

byte searchPlayer(unsigned char x, unsigned char y, byte groundOrBreak, byte facingRight, byte collision)
{
  //if checking for collision = 1 .. look forward 1 less when lookung right
  //char infront = checkGround((x/8)+ (facingRight*3 -1), ((y)/8), groundOrBreak);
  //char infront2 = checkGround((x/8)+ (facingRight*3 -1), ((y)/8)+1, groundOrBreak);
  char infront = checkGround((x/8)+facingRight, ((y)/8), groundOrBreak)*1;
  infront = infront + checkGround((x/8)+facingRight, ((y)/8)+1, groundOrBreak)*2;

  infront = infront + checkGround((x/8)+(facingRight*3 - 1 - collision*(!facingRight)), ((y)/8), groundOrBreak)*4;
  infront = infront + checkGround((x/8)+(facingRight*3 - 1 - collision*(!facingRight)), ((y)/8)+1, groundOrBreak)*8;

  //outsideHelper = (x/8)+ (facingRight*3 -1);
  //outsideHelper = (x/8) +facingRight;
  //outsideHelper2 = ((y)/8)+1;
  outsideHelper3 = shadow[(outsideHelper2*30*2 + outsideHelper*2) >> 3];
  return infront;
}

void loadWorld()
{ 
  int i;
  int tileNum;
  int rleInt = 0;

  vram_adr(NTADR_A(0,0));

  //maps are using RLE compression!
  for(rleInt = 0; rleInt < LargestWorld;)
  {
    //x, y
    int y = tileNum / NUM_SHADOW_COL; //tileNum / 32 = y value
    int x = tileNum % NUM_SHADOW_COL;
   

    char numberOfTimes;
    char currentTile;
    byte shadowBits = 0x00;
    
    //numberOfTimes = worldData[worldNumber][rleInt++];
    numberOfTimes = 1; // no rle compression atm
    //numberOfTimes = worldData[worldNumber][rleInt++]; //w/ rle compression on...
    currentTile = worldData[worldNumber][y*NUM_SHADOW_COL + x];

    if((currentTile & 0xF0) > 0x70)
    {
      shadowBits = 0x03; //breakable and ground
    }
    
    if(currentTile == 0xc0)
    {
      shadowBits = 0x01; //just ground
    }

    if(currentTile == 0xc4 || currentTile == 0xc5 || currentTile == 0xc6 || currentTile == 0xc7)
    {
      shadowBits = 0x02; // just breakable
    }

    for(i = 0; i < numberOfTimes; i++)
    {
      setGround(x,y, shadowBits);

      vram_put(currentTile);
    }

    tileNum += numberOfTimes;

    if(tileNum >= LargestWorld)
    {
      break;
    }
  }

  //outsideHelper = checkGround(15, 21, 1);

}


void debugDisplayShadow()
{

  int rleInt, x, y, ground, breakable;
  vram_adr(NTADR_A(0,0));

  for(rleInt = 0; rleInt < LargestWorld; rleInt++)
  {
    y = rleInt / NUM_SHADOW_COL; //tileNum / 32 = y value
    x = rleInt % NUM_SHADOW_COL;

    ground = checkGround(x, y, 1);
    breakable = checkGround(x, y, 0);

    if(ground && !breakable)
    {
      vram_put(0x01);
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

typedef struct Destructable
{
  unsigned char x;
  unsigned char y;
  unsigned char alive;
  unsigned char sprite;
} Destructable;

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

int modulo(int x, int y)
{
  //if y is a power of 2...
  return (x &(y-1));
}

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

  //Destructable destroybois[32];
  //Destructable brick;
  SpriteActor feet;

  Particles singleBricks[NUM_BRICKS];
  unsigned char numActive = 0;

  short brickSpeed = 5;
  short brickLifetime = 10;

  unsigned int i = 0, j = 0;

  char debugCheck = true;
  char worldScrolling = false;

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
  player.act.moveSpeed = 5;
  player.act.jumpSpeed = 3;
  player.act.grounded = true;
  
  if(playerInAir)
  {
    player.act.dy = 1;
  }


  randomizeParticle(singleBricks, brickSpeed, 0, 0);
  // enable PPU rendeing (turn on screen)

  worldNumber = 0;
  loadWorld();

  debugDisplayShadow();

  ppu_on_all();
  //while(1)
  //{};

  //always updates @ 60fps
  while (1)
  {

    int res = 0;
    int res2 = 0;
    int res3 = 0;
    char breakBlock;
    char pad_result= pad_poll(0) | pad_poll(1);

    //oam_clear();
    cur_oam = oam_spr(stausX, stausY, 0xA0, 0, 0);
    //cur_oam = 4;

    for(i = 0; i < 6; i++)
    {
      groundBlock[0] =  0x00;
    }

    res = searchPlayer(player.act.x, player.act.y, 1, lastFacingRight, -1);
    if(res == 0)
    {
      player.act.dx = ((pad_result & 0x80) >> 7) + -1 * ((pad_result & 0x40) >> 6) ;
    }
    else
    {
      player.act.dx = 0;
    }
    
    res = 0;
    
    /*
    if((pad_result&0x08)>>3 && numActive == 0)
    {
      char closestBrick = 0;
      int currentClosest = 1000;
      for(i = 0; i < 32; i++)
      {
        int dist = (destroybois[i].x-player.act.x)*(destroybois[i].x-player.act.x) + (destroybois[i].y-player.act.y)*(destroybois[i].y-player.act.y);
        if(dist < currentClosest)
        {
          currentClosest = dist;
          closestBrick = i;
        }
      }
      //pressing enter respawns brick...
      for(i = 0; i < 1; i++)
      {
        //SPAWN
        //SPAWN
        ppu_off();
        //ppu_wait_nmi();
        vram_adr(NTADR_A(destroybois[currentClosest].x>>3, destroybois[currentClosest].y>>3));
        vram_put(destroybois[currentClosest].sprite);
        ppu_on_all();

        destroybois[currentClosest].alive = true;
        setGround(destroybois[currentClosest].x>>3, destroybois[currentClosest].y>>3, 0x03);
      }

    }
    */

    {
      
      outsideHelper = (player.act.x/8);
      outsideHelper2 = ((player.act.y)/8)+2;
      outsideHelper3 = (player.act.x/8)+1;
      outsideHelper4 = ((player.act.y)/8)+2;
      
      res = checkGround((player.act.x/8), ((player.act.y)/8)+2, 1) | checkGround((player.act.x/8)+1, ((player.act.y)/8)+2, 1);

      groundBlock[0] = getchar_vram((player.act.x/8), ((player.act.y)/8)+2);
      groundBlock[1] = getchar_vram((player.act.x/8)+1, ((player.act.y)/8)+2);

      if(player.act.dy > 0 && playerInAir)
      {
        //look 1 block down... 
        res2 = checkGround((player.act.x/8), ((player.act.y)/8)+3, 1) | checkGround((player.act.x/8)+1, ((player.act.y)/8)+3, 1);
        
        outsideHelper = (player.act.x/8);
        outsideHelper2 = ((player.act.y)/8)+3;
        outsideHelper3 = (player.act.x/8)+1;
        outsideHelper4 = ((player.act.y)/8)+3;

        groundBlock[2] = getchar_vram((player.act.x/8), ((player.act.y)/8)+3);
        groundBlock[3] = getchar_vram((player.act.x/8)+1, ((player.act.y)/8)+3);

        if(player.act.dy * player.act.jumpSpeed >= 8)
        {
          //look 2 blocks down...
          res3 = checkGround((player.act.x/8), ((player.act.y)/8)+4, 1) | checkGround((player.act.x/8)+1, ((player.act.y)/8)+4, 1);
          groundBlock[4] = getchar_vram((player.act.x/8), ((player.act.y)/8)+4);
          groundBlock[5] = getchar_vram((player.act.x/8)+1, ((player.act.y)/8)+4);
          
          outsideHelper = (player.act.x/8);
          outsideHelper2 = ((player.act.y)/8)+4;
          outsideHelper3 = (player.act.x/8)+1;
          outsideHelper4 = ((player.act.y)/8)+4;

        }
      }

    }

    outsideHelper = res;
    outsideHelper2 = res2;
    outsideHelper3 = res3;

    res = res| res2 | res3;

    outsideHelper4 = res;
    /*
    outsideHelper = 55
    outsideHelper2 = 0A;
    outsideHelper3 = 00;
    outsideHelper4 = 5F;
    */

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
      }
    }



    //char temp = (player.act.dx == -1);
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
      updateMetaSprite(player.act.attribute, PlayerMetaSprite);
      updateMetaSprite(player.act.attribute, PlayerMetaSprite_Run);
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


    /*
    {
      char closestBrick = 0;
      int currentClosest = 1000;
      breakBlock = searchPlayer(player.act.x, player.act.y, 0, lastFacingRight, 0);

      if((breakBlock != 0) && pad_result&0x04)
      {

        int itemX, itemY;
        //1, 2, 4, 8
        if(breakBlock == 1)
        {
          itemX = (player.act.x/8)+lastFacingRight;
          itemY = ((player.act.y)/8);
        }
        if(breakBlock == 2)
        {
          itemX = (player.act.x/8)+lastFacingRight;
          itemY = ((player.act.y)/8)+1;
        }
        if(breakBlock == 4)
        {
          itemX = (player.act.x/8)+(lastFacingRight*3 - 1);
          itemY = ((player.act.y)/8);
        }
        if(breakBlock == 8)
        {
          itemX = (player.act.x/8)+(lastFacingRight*3 - 1);
          itemY = ((player.act.y)/8)+1;
        }

	for(i = 0; i < 32; i++)
        {
          int dist = (destroybois[i].x-itemX)*(destroybois[i].x-itemX) + (destroybois[i].y-itemY)*(destroybois[i].y-itemY);
          if(dist < currentClosest)
          {
            currentClosest = dist;
            closestBrick = i;
          }
        }
        setGround(itemX, itemY, 0);
        destroybois[closestBrick].alive = false;
        //brick.alive = false;

        //delete obj from background...

        ppu_off();
        //ppu_wait_nmi();
        vram_adr(NTADR_A(itemX, itemY));
        vram_put(0x00);
        ppu_on_all();


        numActive = NUM_BRICKS;
        for(i = 0; i < numActive; i++)
        {
          singleBricks[closestBrick].lifetime = brickLifetime;
          singleBricks[closestBrick].x = destroybois[i].x-5;
          singleBricks[closestBrick].y = destroybois[i].y;

        }

      }
    }
    */


    if(player.act.grounded && playerInAir)
    {
      int itemX, itemY;
      //1, 2, 4, 8
      if(res == 1)
      {
        itemX = (player.act.x/8)+lastFacingRight;
        itemY = ((player.act.y)/8);
      }
      if(res == 2)
      {
        itemX = (player.act.x/8)+lastFacingRight;
        itemY = ((player.act.y)/8)+1;
      }
      if(res == 4)
      {
        itemX = (player.act.x/8)+(lastFacingRight*3 - 1);
        itemY = ((player.act.y)/8);
      }
      if(res == 8)
      {
        itemX = (player.act.x/8)+(lastFacingRight*3 - 1);
        itemY = ((player.act.y)/8)+1;
      }
      player.act.jumpTimer = 0;
      if(res3 == false && res == true)
      {
        //zoomin... will pass through two blocks in next tick if unstopped
        player.act.y = (itemY+2)*8;
      }
      else
      {
        if(!res2)
        {
          //zoomin... will pass through one blocks in next tick if unstopped
          player.act.y = (itemY+1)*8;
        }
        else
        {
          player.act.y = (itemY)*8;
        }
      }
      player.act.dy = 0;

      playerInAir = false;
      player.act.grounded = false;

    }

    if(player.act.jumpTimer != MAX_JUMP-1 && !playerInAir)
    {
      if(player.act.grounded == true && ((pad_result & 0x10) >> 4))
      {
        //if grounded and you try to jump...
        player.act.jumpTimer = 0;
        player.act.dy = jumpTable[player.act.jumpTimer];
        player.act.jumpTimer++;
        playerInAir = true;
      }

    }

    if(playerInAir && !player.act.grounded)
    {
      //going up or down in the air
      player.act.dy = jumpTable[player.act.jumpTimer];
      if(player.act.jumpTimer != MAX_JUMP-1)
        player.act.jumpTimer++;
    }

    //debug

    {
      char dx[32];
      sprintf(dx, "grounded: %d", player.act.grounded);
      updateScreen(2, 6, dx, 32);
    }

    writeBinary(2, 5, pad_result);

    {
      char dx[32];
      //sprintf(dx, "%d", player.act.dx);
      sprintf(dx, "break block: %d", breakBlock);
      updateScreen(2, 7, dx, 32);
    }

    for(i = 0; i < 6; i++)
    {
      writeBinary(2, 8+i, groundBlock[i]);
    }




    //scrolling
    {
      byte useScrolling = true;

      int deltaX = 0;
      int deltaY = 0;
      deltaX = player.act.dx * player.act.moveSpeed;
      deltaY = player.act.dy * player.act.jumpSpeed;


      if(useScrolling)
      {
        if(player.act.x + deltaX >= 256-16)
        {
          player.act.x = 256-16;

          worldScrolling = true;
        }

        if(worldScrolling == false)
        {
          player.act.x += deltaX;
          player.act.y += deltaY;
        }

      }
      else
      {
        player.act.x += deltaX;
        player.act.y += deltaY;
      }

      if(worldScrolling)
      {
        //scroll one descrete chunk ata  time...

        //assuming the world is to the right
        world_x += 8;
        player.act.x -=8;

        scroll(world_x, world_y);
        //split(world_x, 0);

        if(world_x >= 256)
        {
          player.act.x = 8;
          worldScrolling = false;
        }
      }
    }



    if(player.act.dx != 0 && player.act.grounded && !worldScrolling)
    {
      if(run == 1)
      {
        cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite_Run);
        run = 0;
      }
      else
      {
        cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite);
        run++;
      }
    }
    else
    {
      cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite);
    }


    if(debugCheck)
    {
      feet.act.x = player.act.x;
      feet.act.y = player.act.y + 8*3;
      cur_oam = oam_spr(feet.act.x, feet.act.y, feet.sprite , 1, cur_oam);

      feet.act.x = player.act.x+8;
      feet.act.y = player.act.y + 8*3;
      cur_oam = oam_spr(feet.act.x, feet.act.y, feet.sprite , 2, cur_oam);
    }

    oam_hide_rest(cur_oam);
    //this makes it wait one frame in between updates
    ppu_wait_frame();
  }
}
