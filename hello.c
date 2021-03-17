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
  int dx;
  int dy;
  unsigned char attribute;
  unsigned char alive;
  unsigned char moveSpeed;
  unsigned char jumpSpeed;
  char jumpTimer;
  char maxlife;
  char lifetime;
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

int doIHit(int x, int y, int x2, int y2, int hitXMin, int hitYMin)
{
  if(getAbs(x - x2) <= hitXMin && getAbs(y - y2) <= hitYMin)
  {
    return true;
  }
  return false;
}

void updateScreen(unsigned char column, unsigned char row, char * buffer, unsigned char num_bytes)
{
  vrambuf_clear();
  set_vram_update(updbuf);
  vrambuf_put(NTADR_A(column, row), buffer, num_bytes);
  vrambuf_flush();
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

//define variables here
DEF_METASPRITE_2x2(PlayerMetaSprite, 0xD8, 0);
MetaActor player;

DEF_METASPRITE_2x2(DoorMetaSprite, 0xC4, 0);
MetaActor Door;

SpriteActor brick;

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


#define MAX_JUMP 9
char jumpTable[MAX_JUMP] = 
{
  -8, -4, -2, -1, 0, 1, 2, 4, 8
};

// main function, run after console reset
void main(void) {
  
  unsigned char floorLevel = 20;
  unsigned char floorTile = 0xc0;
  byte lastFacingRight = true;
  byte playerInAir = false;
  
  SpriteActor singleBricks[32];
  unsigned char numActive = 0;
  unsigned char numBricks = 16;

  unsigned int i = 0, j = 0;
      
  pal_all(PALETTE);// generally before game loop (in main)
  
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
  Door.act.alive = true;
  
  
  player.act.x = 60;
  player.act.y = Door.act.y;
  player.act.dx = 1;
  player.act.dy = 0;
  player.act.attribute = 1 | (0 << 5) | (0 << 6) | (0 << 7);
  updateMetaSprite(player.act.attribute, PlayerMetaSprite);
  player.act.alive = true;
  player.act.moveSpeed = 5;
  player.act.jumpSpeed = 3;
  player.act.grounded = true;
  
  brick.sprite = 0x0F;
  brick.act.x = 30;
  brick.act.y = Door.act.y;
  brick.act.attribute = 1;
  brick.act.alive = true;
  
  //initalize brick spray
  for(i = 0; i < numBricks; i++)
  {
    SpriteActor bck;
    bck.sprite = 0x80;
    bck.act.x = brick.act.x + 2 * (i%4);
    bck.act.y = brick.act.y + i%8;
    bck.act.attribute = 1+i%2;
    bck.act.alive = true;
    bck.act.maxlife = 20;
    bck.act.lifetime = 0;
    bck.act.moveSpeed = 5;
    if(rand()*50 > 25)
    {
      bck.act.dx = -1 * bck.act.moveSpeed;
    }
    else
    {
      bck.act.dx = 1 * bck.act.moveSpeed;
    }
    
    if(rand()*50 > 25)
    {
      bck.act.dy = -1 * bck.act.moveSpeed;
    }
    else
    {
      bck.act.dy = 1 * bck.act.moveSpeed;
    }
    
    singleBricks[i] = bck;
  }
  
  
  
  // enable PPU rendeing (turn on screen)
  ppu_on_all();

  //always updates @ 60fps
  while (1)
  {
    char pad_result= pad_poll(0) | pad_poll(1);
    
    //game loop
    char cur_oam = 0; // max of 64 sprites on screen @ once w/out flickering
    char res;
    //char res2;
    int temp;
    player.act.dx = ((pad_result & 0x80) >> 7) + -1 * ((pad_result & 0x40) >> 6);
    //player.act.dy = ((pad_result & 0x20) >> 5) + -1 * ((pad_result & 0x10) >> 4);
    //player.act.dy = -1 * (player.act.grounded & ((pad_result & 0x10) >> 4));
    
    
    if((pad_result&0x08)>>3 && numActive == 0)
    {
      brick.act.alive = true;
    }
    
    //check if player is touching the ground
    //if there is a grounded block below player then the player is also grounded
    
    res = getchar_vram((player.act.x/8), ((player.act.y)/8)+2);
    //res2 = getchar_vram(((player.act.x + player.act.dx*player.act.moveSpeed)/8), ((player.act.y + player.act.dy*player.act.jumpSpeed)/8 +2));
    //res = (res & 0xF0) > (startOfground & 0xF0) || (res2 & 0xF0) > (startOfground & 0xF0);
    res = (res & 0xF0) > (startOfground & 0xF0);
      
    if( res)
    {
      player.act.grounded = true;
    }
    else
    {
      player.act.grounded = false;
    }
    
    
    {
    //right, left, down, up, select start, B, A
    //shows the player's input
      char dx[16];
      //sprintf(dx, "%d", player.act.dx);
      sprintf(dx, "%d %d %d %d %d %d %d %d", (pad_result&0x80)>>7, (pad_result&0x40)>>6, (pad_result&0x20) >>5, (pad_result&0x10)>>4, (pad_result&0x08)>>3, (pad_result&0x04)>>2, (pad_result&0x02)>>1, pad_result&0x01);
      updateScreen(2, 5, dx, 12);
    }
    
    
    {
      char dx[16];
      sprintf(dx, "grounded %d", player.act.grounded);
      updateScreen(2, 6, dx, 12);
    }
    
    //char temp = (player.act.dx == -1);
    temp = -1;
    if((pad_result & 0x80)>>7 && lastFacingRight == false)
    {
      temp = 0;
      lastFacingRight = true;
    }
    else
    {
      if((pad_result & 0x40)>>6 && lastFacingRight)
      {
        temp = 1;
        lastFacingRight = false;
      }
    }
    if(temp != -1)
    {
    	player.act.attribute = (player.act.attribute & 0xBF) | (temp  << 6);
    	updateMetaSprite(player.act.attribute, PlayerMetaSprite);
    }
    
    cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite);
    cur_oam = oam_meta_spr(Door.act.x, Door.act.y, cur_oam, DoorMetaSprite);

    
    for(i = 0; i < numActive; )
    {
      if(singleBricks[i].act.alive == true)
      {
        cur_oam = oam_spr(singleBricks[i].act.x, singleBricks[i].act.y, singleBricks[i].sprite ,singleBricks[i].act.attribute, cur_oam);
        //if(walk_wait == 0)
        {
          singleBricks[i].act.x = singleBricks[i].act.x + singleBricks[i].act.dx;
          singleBricks[i].act.y = singleBricks[i].act.y + singleBricks[i].act.dy;
          singleBricks[i].act.lifetime += 1;

          if(singleBricks[i].act.lifetime > singleBricks[i].act.maxlife)
          {
            SpriteActor trmp;
            singleBricks[i].act.alive = false;
            trmp = singleBricks[i];
            //swap this dead one with an alive one...
            singleBricks[i] = singleBricks[--numActive];
            singleBricks[numActive] = trmp;
          }
          else
          {
            i++;
          }

        }
      }
    }
    
    
    if(brick.act.alive == true)
    {
      cur_oam = oam_spr(brick.act.x, brick.act.y, brick.sprite ,brick.act.attribute, cur_oam);
      if(doIHit(player.act.x, player.act.y, brick.act.x+8, brick.act.y, 8, 8))
      {
        brick.act.alive = false;
	numActive = 16;
        for(i = 0; i < numActive; i++)
        {
          singleBricks[i].act.lifetime = 0;
          singleBricks[i].act.alive = true;
          singleBricks[i].act.x = brick.act.x+8;
          singleBricks[i].act.y = brick.act.y+8;
        }
        
      }
    }

    {
      //int x_pos = player.act.x;
      //int y_pos = 0;
      //max val is 512
      //scroll(x_pos, y_pos);
    }
    
    if(player.act.grounded && playerInAir)
    {
      playerInAir = false;
      player.act.dy = 0;
      player.act.jumpTimer = 0;
    }
    
    if(player.act.jumpTimer == 0)
    {
      if(player.act.grounded == true && ((pad_result & 0x10) >> 4))
      {
        //if grounded and you try to jump...
        player.act.jumpTimer = MAX_JUMP;
        player.act.dy = jumpTable[MAX_JUMP-player.act.jumpTimer];
        player.act.jumpTimer--;
	playerInAir = true;
      }

    }

    if(playerInAir)
    {
      //going up or down
      player.act.dy = jumpTable[MAX_JUMP-player.act.jumpTimer];
      if(player.act.jumpTimer != 0)
        player.act.jumpTimer--;

    }

    
    player.act.x += player.act.dx * player.act.moveSpeed;
    player.act.y += player.act.dy * player.act.jumpSpeed;
    
    //this makes it wait one frame in between updates
    ppu_wait_frame();
  }
}
