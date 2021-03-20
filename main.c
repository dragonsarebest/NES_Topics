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
#define SHADOW_SIZE 240 // 30 columns, 32 rows
char shadow[SHADOW_SIZE];

//newDataColumn has 32 bytes
char newDataColumn[32];
//newDataRow has 30 bytes
char newDataRow[30];

int shadowWorldX, shadowWorldY;
  

byte outsideHelper; //used for debugging
byte outsideHelper2;
byte outsideHelper3;

//define variables here


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

char startOfground = 0x7f;

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


void scrollShadow(int deltaX, int deltaY, char * newDataColumn, char * newDataRow)
{
  //deltaX and deltaY = change in pixels
  //newDataColumn has 32 bytes
  //newDataRow has 30 bytes
  
  //can only scroll up to 4 tile in x or y dir at a time. otherwise we would need to rework how we input large potions of data..
  //this works with less than 1 byte. w/ 2 bits per tile = 4 tiles per byte
  
  //#define SHADOW_SIZE 240 //30 columns, 32 rows
  //char shadow[SHADOW_SIZE];
  char * pos = shadow;
  int i = 0;
  int j = 0;
  char leftover = 0;
  char old_leftover = 0;
  int numTiles_1;
  int numBytes;
  int remainder;
  
  //numTiles_1 = deltaX >> 3; //number of tiles we need to move!
  numTiles_1 = deltaX/8;
  //numBytes = deltaX >> 2; //divide number by 4, or shift twice since we have 4 tiles per byte
  numBytes = deltaX/4;
  //remainder = numTiles_1 & 0x07; // get the last 3 bits
  remainder = numTiles_1 % 4; //4 tiles = 1 byte, how many do we have left
  
  shadowWorldX += remainder & 0x01;
  remainder = (remainder & 0x0E) + shadowWorldX/2;
  shadowWorldX = shadowWorldX % 2;
  //only want remainder to be powers of 2
  
  numBytes += remainder%8;
  remainder = remainder/8;
  //in case we have >= bits of information... 
  
  if(deltaY != 0)
  {
    
    if(deltaY > 0)
    {
      //move down, shift up
      for(i = 31; i >= 0; i--)
      {
        //bottom = bottom-numtiles
        for(j = 0; j < 30; j++)
        {
          if(i <= numTiles_1)
          {
            shadow[i*30 + j] = newDataRow[j];
          }
          else
          {
            shadow[i*30 + j] = shadow[(i-numTiles_1)*30 + j ];
          }
        }
      }
    }
    else
    {
      //move up, shift down
      for(i = 0; i < 32; i++)
      {
        //bottom = bottom-numtiles
        for(j = 0; j < 30; j++)
        {
          if(i >= 32-numTiles_1)
          {
            shadow[i*30 + j] = newDataRow[j];
          }
          else
          {
            shadow[i*30 + j] = shadow[(i+numTiles_1)*30 + j];
          }
        }
      }
    }
    
  }
  
  
  if(deltaX != 0)
  {
    
    outsideHelper = numTiles_1;
    outsideHelper3 = numBytes;
    outsideHelper2 = remainder;

    
    for(i = 0; i < 32; i++)
    {
      int startOfRow = i*30;

        //SHIFTING X FIRST
        if(deltaX > 0)
        {
          //shift left move right
          for(j = 0; j < 30; j++)
          {
            if(j >= 30-numBytes)
            {
              shadow[startOfRow + j] = 0;
            }
            else
            {
              shadow[startOfRow + j] = shadow[startOfRow + j + numBytes];
            }
          }
          //we've moved the porpper num of bytes... now we need to move bits!


          //old_leftover = 0;
          //old_leftover = would add new map data here...
          //datacolumn holds up to 6 bits of data... 1 byte
          old_leftover = newDataColumn[i] >> (8-remainder);

          for(j = 29; j >= 0; j--)
          {
            //xy11 1111
            //shadow[startOfRow + j] = 1111 1100
            //leftover = 0000 numTiles
            leftover = shadow[startOfRow + j] >> (8-remainder);
            //xy11 1111
            //0000 00xy
            shadow[startOfRow + j] = (shadow[startOfRow + j] << remainder) | old_leftover;
            //xy11 1111
            //1111 1100

            old_leftover = leftover;
          }

        }
        else
        {
          //shift right move left
          for(j = 29; j >= 0; j--)
          {
            if(j <= numBytes)
            {
              shadow[startOfRow + j] = 0;
            }
            else
            {
              shadow[startOfRow + j] = shadow[startOfRow + j - numBytes];
            }
          }

          //we've moved the porpper num of bytes... now we need to move bits!

          //old_leftover = 0;
          //old_leftover = would add new map data here...
          old_leftover = newDataColumn[i] << (remainder);

          for(j = 0; j < 30; j++)
          {
            //xy11 11ab
            //shadow[startOfRow + j] = 00xy 1111
            //leftover = ab00 0000
            leftover = shadow[startOfRow + j] << (remainder);
            //xy11 11ab
            //00xy 1111
            shadow[startOfRow + j] = (shadow[startOfRow + j] >> (8-remainder)) | old_leftover;
            //xy11 11ab
            //00xy 1111

            old_leftover = leftover;
          }
        }
      

    }
  }
}

void setGround(unsigned char x, unsigned char y, byte placeMe)
{
  //placeMe is the replacement 2 bits of data
  int bytenum;
  int remainder;
  byte mask = 0xFF;
  int bitNum = (y*30*2 + x*2);
  bytenum = bitNum >> 3;//same as dividing by 8...
  remainder = (bitNum & 0x07); //the lost 3 bits from divison
  //go remainder number of bits into shadow
  
  //(orignalNumber & 0x??) | (byteToPlace  << remainder);
  //remainder is 0->8
  mask = mask ^ (0x03 << (remainder));
  
  //writeBinary(2, 8, shadow[bytenum]);
  //outsideHelper = shadow[bytenum];
  shadow[bytenum] = (shadow[bytenum] & mask | placeMe << (remainder));
  //outsideHelper = shadow[bytenum];
  //writeBinary(2, 9, shadow[bytenum]);
}

short checkGround(unsigned char x, unsigned char y, byte val)
{
  //val = 0 if you want the 1st value and  = 1 if you want the 2nd
  // 1 = ground, 0 = breakable
  
  //32 columns, 30 rows
  int bytenum;
  int remainder;
  int value;
  int bitNum = (y*30*2 + x*2);
  bytenum = bitNum >> 3;//same as dividing by 8...
  remainder = (bitNum & 0x07) + val; //the lost 3 bits from divison
  //go remainder number of bits into shadow
  value = (shadow[bytenum] >> remainder) & 0x01;
  //outsideHelper3 = value;
  //outsideHelper2 = 0x01;
  //outsideHelper = (shadow[bytenum] >> remainder);
  //writeBinary(2, 10, shadow[bytenum]);
  return value;
}


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


//sprite char + position...


// main function, run after console reset
void main(void) {
  
  unsigned char floorLevel = 20;
  unsigned char floorTile = 0xc0;
  
  byte lastFacingRight = true;
  byte playerInAir = false;
  byte run = 0;
  
  //Destructable destroybois[32];
  Destructable brick;
  SpriteActor feet;
  
  Particles singleBricks[NUM_BRICKS];
  unsigned char numActive = 0;
  
  short brickSpeed = 5;
  short brickLifetime = 10;

  unsigned int i = 0, j = 0;
  
  char debugCheck = false;
      
  pal_all(PALETTE);// generally before game loop (in main)
  
  /*
  writeText("This is\nJoshua Byron's\nfirst NES 'Game'!", 2, 2);
  */
  for(i = 1; i < 31; i++)
  {
    vram_adr(NTADR_A(i,floorLevel));
    vram_put(floorTile);
  }
  
  feet.act.x = 0;
  feet.act.y = 0;
  feet.sprite = 0x0F;
  
  player.act.x = 60;
  player.act.y = 18*8;
  
  player.act.x = 8*12;
  player.act.y = 18*7 + 2;
  //on top of brick block...
  
  player.act.dx = 1;
  player.act.dy = 0;
  player.act.attribute = 1 | (0 << 5) | (0 << 6) | (0 << 7);
  
  player.act.alive = true;
  player.act.moveSpeed = 2;
  player.act.jumpSpeed = 3;
  player.act.grounded = true;
  
  
  brick.sprite = 0x0F;
  brick.x = 100;
  brick.y = 18*8;
  //brick.attribute = 1;
  brick.alive = true;
  vram_adr(NTADR_A(brick.x/8, brick.y/8));
  vram_put(brick.sprite);
  
  
  
  
  for( i = 0; i < SHADOW_SIZE; i++)
  {
    shadow[i] = 0x00;
  }
  
  //set shadow
  for(i = 0; i < 30; i++)
  {
    //32 columns, 30 rows
    for(j = 0; j < 32; j++)
    {
      //0000 0010 just breakable 0x02
      //0000 0011 ground and breakable 0x03
      //0000 0001 just ground 0x01
      if(j == floorLevel)
      {
      	setGround(i, j, 0x02);
      }
    }
  }
  
  shadowWorldX = 0;
  shadowWorldY = 0;
  
  for(i = 0; i < 32; i++)
  {
    newDataColumn[i] = 0;
  }
  
  for(i = 0; i < 30; i++)
  {
    newDataRow[i] = 0;
  }
  
  
  setGround(brick.x/8, brick.y/8, 0x03);
  
  randomizeParticle(singleBricks, brickSpeed, brick.x, brick.y);
  // enable PPU rendeing (turn on screen)
  ppu_on_all();

  //always updates @ 60fps
  while (1)
  {
    
    char pad_result= pad_poll(0) | pad_poll(1);
    
    //game loop
    char cur_oam = 0; // max of 64 sprites on screen @ once w/out flickering
    char res;
    char res2;
    char res3;
    char breakBlock;
    
    res = searchPlayer(player.act.x, player.act.y, 1, lastFacingRight, -1);
    if(res == 0)
    {
    	player.act.dx = ((pad_result & 0x80) >> 7) + -1 * ((pad_result & 0x40) >> 6) ;
    }
    else
    {
      player.act.dx = 0;
    }
    
    if((pad_result&0x08)>>3 && numActive == 0)
    {
      //pressing enter respawns brick...
      for(i = 0; i < 1; i++)
      {
        ppu_off();
        vram_adr(NTADR_A(brick.x/8, brick.y/8));
        vram_put(brick.sprite);
        ppu_on_all();
        
        brick.alive = true;
        setGround(brick.x/8, brick.y/8, 0x03);
      }
    }
    
    {
      res = checkGround((player.act.x/8), ((player.act.y)/8)+2, 1) | checkGround((player.act.x/8)+1, ((player.act.y)/8)+2, 1);
      res2 = res;
      
      if(player.act.dy > 0 && playerInAir)
      {
        //look 1 block down... 
        res = res | checkGround((player.act.x/8), ((player.act.y)/8)+3, 1) | checkGround((player.act.x/8)+1, ((player.act.y)/8)+3, 1);
        res3 = res;
        
        if(player.act.dy * player.act.jumpSpeed >= 8)
        {
          //look 2 blocks down...
          res = res | checkGround((player.act.x/8), ((player.act.y)/8)+4, 1) | checkGround((player.act.x/8)+1, ((player.act.y)/8)+4, 1);
        }
      }
      
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
        }
      }
    
    
    //outsideHelper = res;
    //outsideHelper3 = res3;
    //outsideHelper2 = player.act.grounded;


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
    
    
    {
      //cur_oam = oam_spr(brick.x, brick.y, brick.sprite , brick.attribute, cur_oam);

      //res = checkGround((player.act.x/8), ((player.act.y)/8)+2, 0) | searchPlayer(player.act.x, player.act.y, 0, lastFacingRight, 0);
      //res checks bellow and in the facing dir of the player...
      breakBlock = searchPlayer(player.act.x, player.act.y, 0, lastFacingRight, 0);
      
      //if((res == 1 || rectanglesHit(player.act.x, player.act.y, 16, 16, brick.act.x, brick.act.y, 8, 8)) && pad_result & 0x04)
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
        
        
        setGround(itemX, itemY, 0);
        //destroybois[i].alive = false;
        brick.alive = false;
        
        //delete obj from background...
        
        ppu_off();
        vram_adr(NTADR_A(itemX, itemY));
        vram_put(0x00);
        ppu_on_all();
        
        
	numActive = NUM_BRICKS;
        for(i = 0; i < numActive; i++)
        {
          singleBricks[i].lifetime = brickLifetime;
          singleBricks[i].x = brick.x-5;
          singleBricks[i].y = brick.y;
          //randomizeParticle(singleBricks, brickSpeed, brick.x, brick.y);
          
        }
        
      }
    }
    
    
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
    {
    //right, left, down, up, select start, B, A
    //shows the player's input
      char dx[16];
      //sprintf(dx, "%d", player.act.dx);
      sprintf(dx, "%d %d %d %d %d %d %d %d", (pad_result&0x80)>>7, (pad_result&0x40)>>6, (pad_result&0x20) >>5, (pad_result&0x10)>>4, (pad_result&0x08)>>3, (pad_result&0x04)>>2, (pad_result&0x02)>>1, pad_result&0x01);
      updateScreen(2, 5, dx, 16);
    }
    {
      char dx[32];
      sprintf(dx, "speed: %d :: jump %d", player.act.dy, player.act.jumpTimer);
      updateScreen(2, 11, dx, 32);
      //for(i = 0; i < 30; i++){ppu_wait_frame();}
    }
    
    {
      char dx[32];
      //sprintf(dx, "%d", player.act.dx);
      sprintf(dx, "bricked: %d, %d", brick.x, brick.y);
      updateScreen(2, 10, dx, 32);
      sprintf(dx, "player: %d, %d   ", player.act.x, player.act.y);
      updateScreen(2, 9, dx, 32);
    }
    
    {
      char dx[32];
      //sprintf(dx, "%d", player.act.dx);
      sprintf(dx, "break block: %d", breakBlock);
      updateScreen(2, 7, dx, 32);
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
        if(player.act.x + deltaX > 256/2)
        {
          world_x += deltaX;

          //we need to shift the shadow!
          
          
          if(deltaX != 0)
          {
            outsideHelper = -1;
          }
          
          if(abs(shadowWorldX)/8 >= 4)
          {
            scrollShadow(deltaX, 0, newDataColumn, newDataRow);
          }
          else
          {
            shadowWorldX += deltaX;
          }
          

        }
        else
        {
          player.act.x += deltaX;
        }
        player.act.y += deltaY;
      }
      else
      {
        player.act.x += deltaX;
        player.act.y += deltaY;
      }
      

      //max val is 512
      scroll(world_x, world_y);
      
      
    }
    
    if(player.act.dx != 0 && player.act.grounded)
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
    //for(i = 0; i  < 10; i++)
    //{
    //  ppu_wait_frame();
    //}
    
    //this makes it wait one frame in between updates
    ppu_wait_frame();
  }
}