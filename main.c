#define NES_MIRRORING 1 //(1 = "vertical", 0 = "horizontal")
//Nametable A starts at 0x2000, Nametable B starts at 0x2400

#include "neslib.h"
#include "neslib.h"

#include "apu.h"

#include <stdio.h>
#include <stdlib.h>
#include "Util.h"
#include "Globals.h"

#include <nes.h>
#include "vrambuf.h"
#include "bcd.h"

// link the pattern table into CHR ROM
//#link "chr_generic.s"

//#link "vrambuf.c"

//#link "apu.c"

byte nextMusicByte()
{
  return *musicPtr++;
}

void play_music() {
  static byte chs = 0;
  if (musicPtr) {
    // run out duration timer yet?
    while (cur_duration == 0) {
      // fetch next byte in score
      byte note = nextMusicByte();
      // is this a note?
      if ((note & 0x80) == 0) {
        // pulse plays higher notes, triangle for lower if it's free
        if (note >= BASS_NOTE || (chs & 4)) {
          int period = note_table[note & 63];
          // see which pulse generator is free
          if (!(chs & 1)) {
            APU_PULSE_DECAY(0, period, DUTY_25, 2, 10);
            chs |= 1;
          } else if (!(chs & 2)) {
            APU_PULSE_DECAY(1, period, DUTY_25, 2, 10);
            chs |= 2;
          }
        } else {
          int period = triangle_note[note & 63];
          APU_TRIANGLE_LENGTH(period, 15);
          chs |= 4;
        }
      } else {
        // end of score marker
        if (note == 0xff)
          musicPtr = NULL;
        // set duration until next note
        cur_duration = note & 63;
        // reset channel used mask
        chs = 0;
      }
    }
    cur_duration--;
  }
}

void startMusic(const byte * music)
{
  musicPtr = music;
  cur_duration = 0;
}


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

byte loadWorld()
{ 
  int i = 0;
  int tileNum = 0;
  int index = 0;
  byte current[NUM_SHADOW_COL];
  byte doorCount = 0;
  word addr;

  scrollSwap = !scrollSwap;

  if(scrollSwap)
  {
    addr = NTADR_B(0,0);
  }
  else
  {
    addr = NTADR_A(0,0);
  }

  vram_adr(addr);


  ppu_off();

  vram_inc(0);

  vram_unrle(worldData[worldNumber]);
  vrambuf_flush();

  vram_adr(addr);


  {
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

        if(y <= 5)
        {
          shadowBits = 0;
        }

        setGround(x, y, shadowBits);

        tileNum++;
      }


    }
  }

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

  vram_adr(addr);
  ppu_on_all();

  return true;
}

//functions defined here
void updateMetaSprite(unsigned char attribute, unsigned char * meta, byte normal)
{
  //palette, is behind, flip_hor, flip_vert
  //1 | (0 << 5) | (1 << 6) | (0 << 7);
  //0x3, 0x20, 0x40, 0x80 ^
  if(normal)
  {
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
  else
  {
    //0x80
    if((attribute & 0x80) == 0)
    {
      meta[3] = attribute | 0x80;
      meta[7] = attribute;
      meta[11] = attribute | 0x80;
      meta[15] = attribute;
    }
    else
    {
      meta[3] = attribute & 0x7F;
      meta[7] = attribute;
      meta[11] = attribute & 0x7F;
      meta[15] = attribute;
    }


    //is a long metasprite
    if(((attribute&0x40) >> 6) == 0)
    {
      //flip horizontally
      meta[0] = 0;
      meta[4] = 8;
      meta[8] = 16;
      meta[12] = 24;
    }
    else
    {
      //flip horizontally the other way
      meta[0] = 24;
      meta[4] = 16;
      meta[8] = 8;
      meta[12] = 0;
    }

  }
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
      bck.sprite = dust;
    }
    else
    {
      bck.sprite = dust+1;
    }

    singleBricks[i] = bck;
  }
}


void pal_fade_to(unsigned to)
{
  //if(!to) music_stop();
  while(brightness!=to)
  {
    delay(4);

    if(brightness<to) ++brightness; 
    else --brightness;
    pal_bright(brightness);
  }
}



void setNewPlayerPos()
{
  int i;
  for(i = 0; i < 8; i++)
  {
    if(StairsGoToWorld[i] == old_worldNumber)
    {
      player.x = DoorPositions[i*2]*8;
      player.y = DoorPositions[i*2 + 1]*8;
      break;
    }
  }
  pal_fade_to(6);
}


void scrollWorld(byte direction, Actor* PlayerActor)
{
  int deltaX = 0, deltaY = 0;
  int playerDeltaX, playerDeltaY;
  byte returnVal;

  playerDeltaX = PlayerActor->dx * PlayerActor->moveSpeed;
  playerDeltaY = PlayerActor->dy * PlayerActor->jumpSpeed;


  if(old_worldScrolling == false && worldScrolling == false)
  {
    pal_fade_to(4);
  }

  if(old_worldScrolling == false && worldScrolling == true)
  {
    old_worldScrolling = true;

    setVRAMAddress(0,0,true);
    returnVal = loadWorld();
    pal_fade_to(2);

    if(direction == 0x02 && scrollSwap == 1)
    {
      direction = 0x01;
    }

    else if(direction == 0x01 && scrollSwap == 0)
    {
      direction = 0x02;
    }

    else if(direction == 0x04 && scrollSwap == 1)
    {
      direction = 0x03;
    }

    else if(direction == 0x03 && scrollSwap == 0)
    {
      direction = 0x04;
    }

    transition = direction;

    if(direction == 0x02)
    {
      //right
      PlayerActor->x = 256-24;
    }
    else if(direction == 0x03)
    {
      //up
      PlayerActor->y = 8;
    }
    else if(direction == 0x04)
    {
      //down
      PlayerActor->y = 8;
    }
    else
    {
      //left & defualt direction
      PlayerActor->x = 8;
    }
  }

  if(worldScrolling == false)
  {

    PlayerActor->x += playerDeltaX;
    PlayerActor->y += playerDeltaY;
  }
  else
  {

    ppu_wait_nmi();

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

    transition = direction;

    world_x += deltaX;
    PlayerActor->x -= deltaX;

    world_y += deltaY;
    PlayerActor->y -= deltaY;

    //ppu_wait_nmi();
    scroll(world_x, world_y);
    //split(world_x, world_y);

    if(direction == 0x01)
    {
      //left
      if(world_x <= -256)
      {
        //world_x = 0;
        //world_y = 0;
        old_worldScrolling = false;
        player.x = 256-24;
        worldScrolling = false;
        setNewPlayerPos();
      }
    }
    else if(direction == 0x02)
    {
      //right
      if(world_x >= 256 || world_x >= 0)
      {
        //world_x = 0;
        //world_y = 0;
        old_worldScrolling = false;
        player.x = 8;
        worldScrolling = false;
        setNewPlayerPos();
      }

    }
    else if(direction == 0x03)
    {
      //up
      if(world_y <= -256)
      {
        //world_x = 0;
        //world_y = 0;
        old_worldScrolling = false;
        player.x = 8;
        worldScrolling = false;
        setNewPlayerPos();
      }
    }
    else if(direction == 0x04)
    {
      //down
      if(world_y >= 256 || world_y >= 0)
      {
        //world_x = 0;
        //world_y = 0;
        old_worldScrolling = false;
        player.x = 8;
        worldScrolling = false;
        setNewPlayerPos();
      }
    }
  }
}

void playerSelect(int playerX, int playerY, int yOffset, int xOffset, byte findEmpty)
{
  byte inFront = checkGround((playerX/8 + xOffset), (playerY/8 + yOffset), findEmpty);
  selectedPosition[2] = 0;

  if(inFront != findEmpty)
  {
    selectedPosition[2] = 1;
  }

  selectedPosition[0] = (playerX/8 + xOffset);
  selectedPosition[1] = (playerY/8 + yOffset);
}

void setSelectedPosition(int playerX, int playerY, int upOffset, byte lastFacingRight, int offset, byte findEmpty)
{
  byte breakBlock = searchPlayer(playerX, playerY + (upOffset*8), 0, lastFacingRight, offset);
  int itemX, itemY, collision;
  byte suitableOption = true;
  int deltaX = (lastFacingRight*3 - 1);
  //deltaX = (lastFacingRight*2 - 1);

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
    itemX = ((playerX/8)+deltaX + collision + offset);
    itemY = playerY/8 + 1;
  }
  else if((breakBlock & 0x01) != findEmpty)
  {
    itemX = ((playerX/8)+deltaX + collision + offset);
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
      itemX = ((playerX/8)+deltaX + collision + offset);
      itemY = playerY/8 + 1;
    }
    else if((breakBlock & 0x04) != findEmpty)
    {
      itemX = ((playerX/8)+deltaX + collision + offset);
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

    itemX = ((playerX/8)+deltaX + collision + offset + (lastFacingRight - 1));
    itemY = playerY/8 + 1 + upOffset;
  }
  selectedPosition[0] = itemX;
  selectedPosition[1] = itemY;
}

void updatePlayerHealth(int health, Actor * player)
{
  int i;
  char hearts[10];

  health = player->alive - health;

  if(health <= 0)
  {
    player->alive = 0;
  }
  else
  {
    player->alive = health;
  }

  for(i = 1; i < 11; i++)
  {
    if(i <= player->alive)
    {
      //draw a heart = 0x15
      hearts[i-1] = 0x15;
      //x = 5+i
      //y = 2
    }
    else
    {
      //draw 0x16
      hearts[i-1] = 0x16;
    }
  }

  updateScreen(5, 2, hearts, 10);
}


void updateBombBlockLives(int amount, byte whichOne)
{
  char number[14];

  if(whichOne == 0x01)
  {
    numBombs += amount;
    if(numBombs > 99)
      numBombs = 99;
  }
  if(whichOne == 0x02)
  {
    numBlocks += amount;
    if(numBlocks > 99)
      numBlocks = 99;
  }
  if(whichOne == 0x03)
  {
    if(numLives == 0 && amount < 0)
    {
      playerLost = true;
    }
    else
    {
      numLives += amount;
      if(numLives > 99)
        numLives = 99;
    }
  }

  sprintf(number, "-x%d%d -x%d%d -x%d%d", (numBombs/10)%10, numBombs%10, 
          (numBlocks/10)%10, numBlocks%10, (numLives/10)%10, numLives%10);
  number[0] = 0x19;
  number[5] = 0xB2;
  number[10] = 0xB1;
  updateScreen(16, 2, number, 14);
}


void setWorldNumber(byte num)
{
  old_worldNumber = worldNumber;
  worldNumber = num;

  spawnBoss = true;
}


int absVal(int x)
{
  int mask = x >> 7;
  mask += x;
  mask = (mask + x) ^ mask;
  return mask;
}

int spriteCollision(Actor * actor1, Actor * actor2, byte amountX, byte amountY)
{
  int diffX = (actor1->x - actor2->x);
  int diffY = (actor1->y - actor2->y);
  diffX = absVal(diffX);
  diffY = absVal(diffY);
  return((diffX <= amountX) * 0x02) | (diffY <= amountY);
}

void updateMetaSprites(Actor * actor, byte normal)
{
  updateMetaSprite(actor->attribute, MetaTable[actor->currentAnimation], normal);
}

int detectAllCollisions(Actor * antagonist, int distX, int distY)
{
  //distY & distX = 16 if you want a PURE overlap
  int i = 0;
  for(i = 0; i < NumActors; i++)
  {
    Actor * act = allActors[i];
    if(act->alive > 0)
    {
      if((act->boolean & 0x20) && !(antagonist->boolean & 0x40) )
      {
        //if this actor is blockable, and antagonist is not currently blocked
        int returnValue = spriteCollision(antagonist, act, distX, distY); //stictly anyoverlapping
        if(returnValue >= 0x30)
        {
          //means we found a blocking hit...
          antagonist->boolean |= 0x40;
          act->boolean |= 0x40;
          return i;
        }
      }
    }
  }
  return -1;
}

void breakThisBlock(int itemX, int itemY, byte isPlayer)
{
  char block[1];
  char oldBlock;
  int i;

  ppu_wait_nmi();
  oldBlock = getchar_vram(itemX, itemY);

  if(oldBlock >= door && oldBlock <= door+4)
  {
    int x = itemX, y = itemY, i;
    //0xFc -> 0xFF
    block[0] = stairs + (oldBlock -  door);


    if(oldBlock == door+1)
    {
      y--;
    }
    if( oldBlock == door+2)
    {
      x--;
    }
    if(oldBlock == door+3)
    {
      x--;
      y--;
    }

    for(i = 0; i < numDoors; i++)
    {
      if(x == DoorPositions[i*2] && y == DoorPositions[i*2 + 1])
      {
        DoorInfo[i]--;
        //only need to break one pannel to progress..
        if(DoorInfo[i] != 4)
        {
          UpTo8DoorsOpen = UpTo8DoorsOpen | 1 << i;
          //stairwellAccessable!

        }
        break;
      }
    }
  }
  else
  {
    block[0] = 0x0D;
  }

  setGround(itemX, itemY, 0);

  updateScreen(itemX, itemY, block, 1);

  if(isPlayer)
  {
    change |= 0x02;
  }
  numActive = NUM_BRICKS;

  for(i = 0; i < numActive; i++)
  {
    singleBricks[i].lifetime = brickLifetime;
    singleBricks[i].x = itemX*8-5;
    singleBricks[i].y = itemY*8;

  }
}

char updateActor(Actor * actor, char cur_oam, int index)
{
  int i = 0;
  int res = 0;
  char breakBlock;

  char pad_result = pad_poll(0) | pad_poll(1);


  byte playerInAir = (actor->boolean & 0x01);
  byte lastFacingRight = (actor->boolean & 0x02) >> 1;
  byte attacking = (actor->boolean & 0x04) >> 2;
  byte isPlayer = (actor->boolean & 0x08) >> 3;
  byte isBoss = (actor->boolean & 0x10) >> 4;
  byte isBlockable = (actor->boolean & 0x20) >> 5;
  byte isBlocked = (actor->boolean & 0x40) >> 6;
  byte istracking = (actor->boolean & 0x80) >> 7;

  static byte lastX = 0;
  static byte lastY = 0;


  int noBlocksAbove;
  byte jumping;
  byte breaking;


  if(!isPlayer)
  {
    pad_result = 0;
  }

  /*
  if(isBoss)
  {
    writeBinary(2, 5, boss.boolean);
    {
      char dx[2];
      sprintf(dx, "%d", boss.animationTimer);
      updateScreen(2, 6, dx, 2);
    }
  }
  */


  if(actor->isSprite && isBlockable == false || isBlockable == false && istracking == false && isPlayer == false && isBoss == false)
  {
    //dont update these as hard

    index = 0;

    if(checkGround(actor->x/8,actor->y/8 + 1, 1) == 0)
    {
      actor->dy = 1;
    }
    else
    {
      actor->dy = -1;
    }

    actor->x += actor->dx * actor->moveSpeed;
    actor->y += actor->dy * actor->jumpSpeed;

    actor->currentAnimation = actor->startOfAnimations;

    if(actor->startOfAnimations == 10)
    {
      //is the chain
      //actor->attribute = boss.attribute;

      actor->attribute |= boss.attribute & 0x04;
      actor->hurtFlash = boss.hurtFlash;

      if(boss.alive <= 0)
      {
        actor->alive = 0;
      }

      if(actor->hurtFlash != 0)
      {
        actor->hurtFlash--;
        if(actor->hurtFlash%2)
          actor->attribute = !(actor->attribute & 0x03) | actor->attribute & 0xFC;
      }
      else
      {
        actor->attribute = !(0 & 0x03) | actor->attribute & 0xFC;
      }

      if(actor->attribute & 0x04)
      {
        actor->boolean |= 0x02;
      }
      else
      {
        actor->boolean &= 0xFD;
      }

      lastFacingRight = (boss.boolean & 0x02) >> 1;

      //if(index == 0)
      {
        //actor->y != boss.y ||
        if(boss.dx != 0)
        {
          if( actor->animationTimer >= 10 && (actor->attribute & 0x80) == 0)
          {
            actor->animationTimer = 0;
            //actor->attribute = 0x80 *( !(actor->attribute&0x80 >> 7)) | (actor->attribute&0x7f) ;
            actor->attribute |= 0x80;
          }
          else if(actor->animationTimer >= 10 && (actor->attribute & 0x80) != 0)
          {
            actor->animationTimer = 0;
            actor->attribute &= 0x7F;
          }
        }

        actor->y = boss.y + 8;

        if(lastFacingRight)
        {
          actor->x = boss.x - 31;
          if(actor->attribute | 0x80)
          {
            actor->x++;
          }

        }
        else
        {
          actor->x = boss.x + 15;
          if(actor->attribute | 0x80)
          {
            actor->x--;
          }
        }

        if(actor->attribute | 0x80)
        {
          actor->y -= 3;
        }

        actor->animationTimer ++;

      }
    }
    return cur_oam;
  }

  if(!isPlayer)
  {
    pad_result = 0;
  }

  if(actor->alive > 0 || isPlayer)
  {
    res = searchPlayer(actor->x, actor->y, 1, lastFacingRight, 0);

    if(!isPlayer && actor->hurtFlash == 0 && (actor->boolean & 0x80) != 0)
    {
      int dist = 16;
      //set pad_result here to simulate input for other entities
      pad_result = 0;

      if(isBoss)
      {
        dist = 256;
      }


      if(absVal(actor->x - player.x) > dist || actor->dx == 0) // && player.act.hurtFlash <= 0
      {

        if(actor->x < player.x)
        {
          pad_result |= 0x80;
        }
        else if(actor->x > player.x)
        {
          pad_result |= 0x40;
        }
      }
      else
      {
        if(isBoss && chargeTimer >= MAX_Charge -1)
        {
          chargeTimer = 0;
        }
      }

      //go toward player basic script

      if(isBoss)
      {
        if(bossNumber == 0)
        {
          //chain chomp
          if(isBlocked)
          {
            isBlocked = false;
            attacking = true;
            boss.animationTimer = 0;
            //chargeTimer = 0;
            boss.moveSpeed = 0;
            boss.boolean |= 0x04;
            //pad_result &= 0x3F;
            boss.boolean &= 0xBF; //dont break again
            pad_result |= attackButton;

          }


          //continue input
          if(attacking == false)
          {
            if(boss.dx < 0)
            {
              pad_result |= 0x40;

            }
            else if(boss.dx > 0)
            {
              pad_result |= 0x80;
            }
          }
          else
          {
            boss.dx = 0;
          }




          if(actor->grounded == true && actor->dx == 0)
          {

            int offset = 0;
            if(lastFacingRight == false)
            {
              offset = 0; //fixes bug where you couldnt break blocks on your left if you were touching them
            }

            setSelectedPosition(actor->x, actor->y, 0, lastFacingRight, offset, 0);

            if(selectedPosition[2] != 0)
            {
              pad_result |= 0x04; //breaking
            }
            else if(attacking == false)
            {
              pad_result |= 0x10; // jump
            }

          }

          if(attacking == false)
          {
            boss.moveSpeed = chargeTable[chargeTimer];
            if(chargeTimer < MAX_Charge-1)
            {
              chargeTimer++;
            }
            else
            {
              //chargeTimer = 0;
            }
          }
          else
          {
            chargeTimer = 0;
            boss.moveSpeed = 2;
          }

          if(boss.hurtFlash != 0)
          {
            boss.dx *= -1;
          }
        }
      }
    }

    //input from trackpad!

    actor->dx = ((pad_result & 0x80) >> 7) + -1 * ((pad_result & 0x40) >> 6);

    {
      byte shift = pad_result & 0x02;
      if(!shift)
      {
        if((pad_result & 0x80)>>7)
        {
          //actor->boolean |= 0x02;
          //lastFacingRight = (actor->boolean & 0x02) >> 1;
          lastFacingRight = true;
        }
        else
        {
          if((pad_result & 0x40)>>6)
          {
            //actor->boolean = actor->boolean & 0xFD; //1111 1101
            //lastFacingRight = (actor->boolean & 0x02) >> 1;

            lastFacingRight = false;
          }
        }
      }

      jumping = pad_result & 0x10;
      breaking = pad_result & attackButton;
      noBlocksAbove = aboveOrBellowPlayer(actor->x, actor->y, 1, true, lastFacingRight, 0);

      //if(!shift && resetTouch == 0)
      {
        byte old = actor->attribute & 0x40;
        byte new;

        actor->attribute = (actor->attribute & 0xBF) | (!lastFacingRight  << 6);
        actor->boolean = (actor->boolean & 0xFB) | (!lastFacingRight  << 2);

        new = actor->attribute & 0x40;

        if(new != old && leftRight != 1)
        {
          leftRight *= -1;
          leftRight ++;
          //mirror's leftRight when the player turns around
        }

      }

      if((res & 0x0C) != 0 && actor->dx > 0)
      {
        //player.act.dx = -1;
        actor->dx = 0;
      }
      if((res & 0x03) != 0 && actor->dx < 0)
      {
        //player.act.dx = 1;
        actor->dx = 0;
        actor->x -= (actor->x % 8) > 0;

      }

      res = 0;

      for(i = 0; i < 6; i++)
      {

        if(i >= 4 && actor->dy == 0)
        {
          groundBlock[i] = 0;
        }
        else
        {
          if(!(actor->dy > 0 && playerInAir) && (i == 2 || i == 3))
          {
            groundBlock[i] = 0;
          }
          else if(! (actor->dy * actor->jumpSpeed >= 8) && (i == 4 || i == 5))
          {
            groundBlock[i] = 0;
          }
          else
          {
            groundBlock[i] = checkGround((actor->x/8) + (i%2), ((actor->y)/8)+2-actor->isSprite + (i/2), 1);
          }

        }
        res = res | groundBlock[i];
      }

      if(res != 0)
      {
        actor->grounded = true;
      }
      else
      {
        actor->grounded = false;
        if(actor->grounded == false && playerInAir == false)
        {
          //start falling! we walk off a block and dont jump
          actor->jumpTimer = MAX_JUMP/2;
          //actor->boolean |= 0x01;
          //playerInAir = actor->boolean&0x01;
          playerInAir = true;
          actor->fallTimer = 0;
        }
      }

      if(breaking != 0 || player.animationTimer != 0 && attacking && isPlayer || shift != 0)
      {
        jumping = 0;
        //cannot jump & break @ same time
      }

      if(shift)
      {
        actor->dx = 0;
        if(resetTouch > 0)
        {
          resetTouch--;
        }
      }

      {

        if(lastTouch == 0)
        {
          if((pad_result & 0x20) && shift)
          {
            noBlocksAbove = aboveOrBellowPlayer(player.x, player.y, 1, false, lastFacingRight, 0);
            {
              //pressing down on keypad
              //Up_Down = 0x01;
              Up_Down++;
              if(Up_Down > 3)
              {
                Up_Down = 3;
              }
              lastTouch = waitTouch;
              resetTouch = waitTouch*3;
            }
          }
          else if(((pad_result & 0x10)) && shift)
          {
            //pressing up but cant jump.... look up instead!
            //Up_Down = 0x02; //0x01 = up, 0x02 = down, 0x00 = neither
            Up_Down--;
            if(Up_Down < -2)
            {
              Up_Down = -2;
            }
            lastTouch = waitTouch;
            resetTouch = waitTouch*3;
          }
          else
          {
            if(((pad_result & 0x40)) && shift)
            {
              //pressing up but cant jump.... look up instead!
              //Up_Down = 0x02; //0x01 = up, 0x02 = down, 0x00 = neither
              leftRight--;
              if(leftRight < -2)
              {
                leftRight = -2;
              }
              lastTouch = waitTouch;
              resetTouch = waitTouch*3;
            }
            else if(((pad_result & 0x80)) && shift)
            {
              //pressing up but cant jump.... look up instead!
              //Up_Down = 0x02; //0x01 = up, 0x02 = down, 0x00 = neither
              leftRight++;
              if(leftRight > 4)
              {
                leftRight = 4;
              }
              lastTouch = waitTouch;
              resetTouch = waitTouch*3;
            }
            else if(resetTouch <= 0 && shift)
            {
              //Up_Down = 0x00;
              Up_Down = 1;
              leftRight = 1;
              resetTouch = 0;
              lastTouch = waitTouch*3;
            }
          }

        }

      }
      if(lastTouch > 0)
      {
        lastTouch--;
      }


    }




    //player only behavior
    if(actor->hurtFlash == 0)
    {
      int offset = 1;
      int upOffset = 0;

      if(lastFacingRight == false)
      {
        offset = 0; //fixes bug where you couldnt break blocks on your left if you were touching them
      }
      if(isPlayer)
      {
        upOffset = Up_Down;
        offset = leftRight;

      }


      if(!isPlayer)
      {
        setSelectedPosition(actor->x, actor->y, upOffset, lastFacingRight, offset, 0);;
      }
      else
      {
        playerSelect(actor->x, actor->y, upOffset, offset, 0);
      }

      //only players & boses can break blocks
      if((attacking == false && isPlayer || isBoss))
      {
        char newBlock = 0x0D;
        int itemX, itemY, collision;
        byte suitableOption = false;

        if(breaking && attacking == false)
        {
          attacking = true;
          //actor->boolean |= 0x04;
          actor->animationTimer = 0;
        }

        if(!isPlayer)
        {
          breakBlock = searchPlayer(actor->x, actor->y + (upOffset*8), 0, lastFacingRight, offset);


          if((breakBlock != 0) && (breaking) || selectedPosition[2] != 0 && (breaking) || isBoss && selectedPosition[2] != 0)
          {
            suitableOption = true;
            breaking = false;
            //breakblock = option4 bit, option3 bit, option2 bit, option1 bit...
            //outsideHelper = breakBlock;

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
              itemX = ((actor->x/8)+(lastFacingRight*3 - 1) + collision + offset);
              itemY = actor->y/8 + 1;
            }
            else if((breakBlock & 0x01) != 0)
            {
              itemX = ((actor->x/8)+(lastFacingRight*3 - 1) + collision + offset);
              itemY = actor->y/8;
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
                itemX = ((actor->x/8)+(lastFacingRight*3 - 1) + collision + offset);
                itemY = actor->y/8 + 1;
              }
              else if((breakBlock & 0x04) != 0)
              {
                itemX = ((actor->x/8)+(lastFacingRight*3 - 1) + collision + offset);
                itemY = actor->y/8;
              }
              else
              {
                if(selectedPosition[2] != 0)
                {
                  itemX = selectedPosition[0];
                  itemY = selectedPosition[1];

                }
                else
                {
                  suitableOption = false;
                }


              }
            }
          }
          itemY += upOffset;
        }
        else if(breaking)
        {
          //is player
          if(selectedPosition[2] != 0)
          {
            itemX = selectedPosition[0];
            itemY = selectedPosition[1];
            suitableOption = true;

          }
          else
          {
            suitableOption = false;
          }
        }

        if(suitableOption)
        {
          if(isBoss)
          {
            chargeTimer = 0;
            boss.dx = 0;
          }
          breakThisBlock(itemX, itemY, isPlayer);
        }
      }

      //placing blocks


      if((pad_result&0x08)>>3 && numBlocks > 0 && actor->hurtFlash == 0)
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
            char block[1];
            block[0] = playerPlaceBlock;
            if(isPlayer)
            {
              change |= 0x01;
            }
            updateScreen(selectedPosition[0] + deltaX, selectedPosition[1]+deltaY, block, 1);

            setGround(selectedPosition[0] + deltaX, selectedPosition[1]+deltaY, 0x03);

            digTimer = digWait;
          }
          else
          {
            digTimer--;
          }
        }
      }


      if(boss.alive <= 0 && isPlayer)
      {
        //makes stairs go to next level!
        for(i = 0; i < numDoors; i++)
        {
          if(((UpTo8DoorsOpen >> i) & 0x01) == 1)
          {
            //then these stairs are open!
            if(jumping && actor->x/8 >= DoorPositions[i*2]-1 && actor->x/8 <= DoorPositions[i*2]+1
               && actor->y/8 >= DoorPositions[i*2 + 1]-1 && actor->y/8 <= DoorPositions[i*2 + 1]+1)
            {
              jumping = false;

              setWorldNumber(StairsGoToWorld[i]);
              worldScrolling = true;

              ppu_off(); //makes scrolling work!!!!!!
            }
          }
        }
      }

    }



    //dont let boss & player collide
    if(boss.hurtFlash == 0 && isPlayer)
    {
      //int returnVal = metaSpriteCollision(&player, &boss, 24, 24);
      int returnVal = spriteCollision(&player, &boss, 30, 24);
      if(returnVal >= 0x03  && boss.alive)
      {
        if(player.dx > 0 && boss.x > player.x || player.dx < 0 && boss.x < player.x)
        {
          player.dx = 0;
        }

        //allows player to stand on boss
        if(returnVal == 0x03)
        {
          player.dy = 0;
          player.boolean = player.boolean & 0xFE;
          playerInAir = player.boolean&0x01;
          player.jumpTimer = 0;
          player.grounded = true;
        }
        if(returnVal >= 0x03)
        {
          //boss.boolean |= 0x40; //breaking whatever is infront of boss
          //boss.boolean &= 0xFB; //stop attacking bc youre doing it again

          //if the chain chomp is facing player
          if((boss.boolean & 0x04) == 0 && boss.hurtFlash == 0 && ( ((boss.attribute & 0x40) && player.x <= boss.x ) || (!(boss.attribute & 0x40) && player.x >= boss.x )))
          {
            boss.boolean |= 0x40; //breaking whatever is infront of boss
            boss.dx = 0;
            player.hurtFlash += 20;

            change |= 0x30;

            player.dy = -5;

            if(boss.x > player.x)
            {
              player.dx = -30;
            }
            else
            {
              player.dx = 30;
            }
          }
        }
        else
        {
          boss.boolean &= 0xBF; // do not attack, do not break, thers nothing in front of you
        }
      }

      //extended reach for player to hit
      //returnVal = returnVal | spriteCollision(&player, &boss, 30, 24);
      returnVal = returnVal | spriteCollision(&player, &chain, 8, 8);
      if(returnVal >= 0x03 && breaking)
      {
        //if(breaking)
        {
          //only hurt boss when swinging sword
          boss.hurtFlash += 5;

          //player only does one "hit" at a time
          if(boss.alive > 0)
            boss.alive --;

          if(boss.alive <= 0)
          {
            justKilledBoss = true;
          }

          boss.dy = -2;

          if(boss.x < player.x)
          {
            boss.x -= 10;
          }
          else
          {
            boss.x += 10;
          }
        }

      }
    }
    else
    {
      boss.boolean &= 0xBF; // do not attack, do not break, you cannot since you are hurt
    }

    if(actor->grounded && playerInAir)
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

      actor->y = ((actor->y/8) + itemY)*8;

      actor->dy = 0;

      actor->boolean = actor->boolean & 0xFE;
      playerInAir = actor->boolean&0x01;
      //player.act.grounded = false;
      actor->jumpTimer = 0;

    }


    if(actor->jumpTimer != MAX_JUMP-1 && !playerInAir)
    {
      if(actor->grounded == true && noBlocksAbove == 0 && jumping)
      {
        //if grounded and you try to jump...
        actor->jumpTimer = 0;
        actor->dy = jumpTable[actor->jumpTimer];
        actor->jumpTimer++;
        //actor->boolean |= 0x01;
        //playerInAir = actor->boolean&0x01;
        playerInAir = true;
        actor->fallTimer = 0;
      }

    }


    if(playerInAir && !actor->grounded)
    {

      if(actor->fallTimer == 0)
      {

        //going up or down in the air
        actor->dy = jumpTable[actor->jumpTimer];
        if(actor->jumpTimer != MAX_JUMP-1)
          actor->jumpTimer++;
      }

      actor->fallTimer++;
      actor->fallTimer = actor->fallTimer % timeBetweenFall;

    }

  }

  //make them flash when hurt
  if(actor->hurtFlash != 0)
  {
    actor->hurtFlash--;
    if(actor->hurtFlash%2)
      actor->attribute = !(actor->attribute & 0x03) | actor->attribute & 0xFC;
  }
  else
  {
    actor->attribute = !(0 & 0x03) | actor->attribute & 0xFC;
  }



  {
    //animate the player!
    byte shoudlRun = true;

    if(attacking)
    {

      shoudlRun = false;
      if( actor->animationTimer < 8 - 4*isPlayer )
      {
        actor->currentAnimation = actor->startOfAnimations+1;

        //cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite_Attack_1);
        //ppu_wait_frame();
        //ppu_wait_frame();
        actor->animationTimer++;
      }
      else if(actor->animationTimer >= 8 - 4*isPlayer && actor->animationTimer < 24 - 12*isPlayer) 
      {
        actor->currentAnimation = actor->startOfAnimations+2;
        //cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite_Attack_2);
        //ppu_wait_frame();
        //ppu_wait_frame();
        actor->animationTimer++; 
      }
      else
      {
        actor->animationTimer = 0;
        attacking = false;
        shoudlRun = true;
      }

    }

    if(shoudlRun)
    {
      if(playerInAir)
      {
        //cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite_Jump);
        actor->currentAnimation = actor->startOfAnimations+3;
      }
      else if(actor->dx != 0)
      {
        if(actor->animationTimer < 4)
        {
          //cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite_Run);
          actor->currentAnimation = actor->startOfAnimations+4;
          actor->animationTimer++;
        }
        else if (actor->animationTimer >= 4 && actor->animationTimer < 8)
        {
          actor->currentAnimation = actor->startOfAnimations;
          //cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite);
          actor->animationTimer++;
        }
        else
        {
          actor->currentAnimation = actor->startOfAnimations+4;
          //cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite_Run);
          actor->animationTimer = 0;
        }
      }
      else
      {
        actor->currentAnimation = actor->startOfAnimations;
        //cur_oam = oam_meta_spr(player.act.x, player.act.y, cur_oam, PlayerMetaSprite);
      }
    }

  }


  /*
  {
    char dist = 16;
    int index;
    if(actor->isSprite)
    {
      dist = 8;
    }
    index = detectAllCollisions(actor, dist, dist);
    if(index != -1)
    {
      //check to see if you are going toward them or away
      Actor * antagonist = allActors[index];
      if(actor->dx < 0 && antagonist->x <= actor->x || actor->dx > 0 && antagonist->x >= actor->x)
      {
        actor->dx = 0;
        isBlocked = true;
      }
      else if(actor->dy < 0 && antagonist->y <= actor->y || actor->dy > 0 && antagonist->y >= actor->y)
      {
        actor->dy = 0;
        isBlocked = true;
      }
      else
      {
        isBlocked = false;
      }
    }
    else
    {
       isBlocked = false;;
    }
  }
  */


  if(!isPlayer)
  {
    //player is updated in scroll
    byte temp = actor->x;

    actor->x += actor->dx * actor->moveSpeed;
    actor->y += actor->dy * actor->jumpSpeed;

    if(temp == actor->x)
    {
      actor->dx = 0;
    }

    //actor->dx = 0;
  }

  lastX = actor->x;
  lastY = actor->y;


  actor->boolean = 0;

  if(playerInAir)
  {
    actor->boolean |= 0x01;
  }
  if(lastFacingRight)
  {
    actor->boolean |= 0x02;
  }
  if(attacking)
  {
    actor->boolean |= 0x04;
  }
  if(isPlayer)
  {
    actor->boolean |= 0x08;
  }
  if(isBoss)
  {
    actor->boolean |= 0x10;
  }
  if(isBlockable)
  {
    actor->boolean |= 0x20;
  }
  if(isBlocked)
  {
    actor->boolean |= 0x40;
  }
  if(istracking)
  {
    actor->boolean |= 0x80;
  }

  return cur_oam;
}

char updateSpriteActors(Actor * actor, char cur_oam, int index)
{
  cur_oam = updateActor(actor, cur_oam, index);
  //updateSprites(actor);
  return cur_oam;
}

char updateMetaActor(Actor * actor, char cur_oam, int index, byte normal)
{
  cur_oam = updateActor(actor, cur_oam, index);
  updateMetaSprites(actor, normal);
  return cur_oam;
  //return cur_oam = oam_meta_spr(actor->act.x, actor->act.y, cur_oam, PlayerMetaSprite);
}

void main(void) {

  char cur_oam;
  unsigned int i = 0, j = 0;
  byte debug = false;
  byte deathTimer = 0xF;

  worldScrolling = false;
  scrollSwap = !worldScrolling;
  old_worldScrolling = worldScrolling;
  worldNumber = 0;
  transition = 0x01;
  //default world parameters

  bank_spr(1);
  bank_bg(0);

  MetaTable[0] = PlayerMetaSprite;		//idle
  MetaTable[1] = PlayerMetaSprite_Attack_1;	//attack 1
  MetaTable[2] = PlayerMetaSprite_Attack_2;	//attack 2
  MetaTable[3] = PlayerMetaSprite_Jump;		//jump
  MetaTable[4] = PlayerMetaSprite_Run;		//run

  //ChainChomp_stand
  //ChainChomp_Jump
  //ChainChomp_Attack_1
  //ChainChomp_Attack_2
  //ChainChomp_Run
  MetaTable[5] = ChainChomp_stand;		//idle
  MetaTable[6] = ChainChomp_Attack_1;		//attack 1
  MetaTable[7] = ChainChomp_Attack_2;		//attack 2
  MetaTable[8] = ChainChomp_Jump;		//jump
  MetaTable[9] = ChainChomp_Run;		//run

  MetaTable[10] = longChain;
  //MetaTable[0] = longChain;

  //put all actors here!
  allActors[0] = &player;
  allActors[1] = &boss;


  pal_all(PALETTE);// generally before game loop (in main)


  //chain.currentAnimation = 0x30; //if it is a sprite
  chain.currentAnimation = 10;
  chain.startOfAnimations = 10;
  chain.x = 8*8;
  chain.y = 8*8;
  chain.dx = 0;
  chain.dy = 0;
  chain.attribute = 1 | (0 << 5) | (0 << 6) | (0 << 7);
  chain.alive = 0; // hit points
  chain.moveSpeed = 1;
  chain.jumpSpeed = 2;
  chain.grounded = true;
  //0x01 = playerInAir,0x02 = lastFacingRight, 0x04 = isAttacking, 0x08 = is Player, 0x10 = is boss, 0x20 = isBlockable, 0x40 = ISBLOCKED, 0x80 = tracks
  chain.boolean = 0x01 | 0x02;
  //chain.isSprite = false; //if it is a sprite
  chain.hurtFlash = 0;

  allActors[2] = &chain;


  player.x = 8*1;
  player.y = 23* 8;

  player.dx = 0;
  player.dy = 0;
  player.attribute = 1 | (0 << 5) | (0 << 6) | (0 << 7);
  player.alive = 10; // hit points
  player.moveSpeed = 1;
  player.jumpSpeed = 4;
  player.grounded = true;
  //0x01 = playerInAir,0x02 = lastFacingRight, 0x04 = isAttacking, 0x08 = is Player, 0x10 = is boss, 0x20 = isBlockable, 0x40 = ISBLOCKED, 0x80 = tracks
  player.boolean = 0x01 | 0x02 | 0x08 | 0x20 | 0x80;
  player.currentAnimation = 0;
  player.startOfAnimations = 0;
  player.isSprite = false;
  player.hurtFlash = 0;

  boss.alive = 0;
  boss.moveSpeed = 12;
  boss.jumpSpeed = 2;
  boss.grounded = true;
  boss.dx = -1;
  boss.dy = 0;
  boss.attribute = 1 | (0 << 5) | (0 << 6) | (0 << 7);
  //0x01 = playerInAir,0x02 = lastFacingRight, 0x04 = isAttacking, 0x08 = is Player, 0x10 = is boss, 0x20 = isBlockable, 0x40 = ISBLOCKED, 0x80 = tracks
  boss.boolean = 0x01 | 0x10 | 0x20 | 0x80;
  boss.currentAnimation = 0;
  boss.startOfAnimations = 5;
  boss.isSprite = false;
  boss.hurtFlash = 0;

  randomizeParticle(singleBricks, brickSpeed, 0, 0);
  // enable PPU rendeing (turn on screen)

  
  apu_init();
  musicPtr = 0;

  loadWorld();

  ppu_on_all();

  //always updates @ 60fps
  while (1)
  {
    change = 0;
    cur_oam = 4; //idk why this is necessary

    //MainTheme, bossFight
    if (!musicPtr) startMusic(bossFight);
    //waitvsync();
    play_music();
    
    if(worldScrolling)
    {
      player.dx = 0;
      player.dy = 0;

      boss.dx = 0;
      boss.dy = 0;

      //dont allow any actors to move or update!
    }
    else
    {

      if(spawnBoss)
      {
        //always first thing to run once a new world is loaded

        updatePlayerHealth(0, &player);
        updateBombBlockLives(0, 0x01);
        updateBombBlockLives(0, 0x02);
        updateBombBlockLives(0, 0x03);

        justKilledBoss = false;

        spawnBoss = false;
        //get spawn location & which boss here
        if(worldNumber == 1 && (bossSpawnedTracker & 0x01) == 0)
        {
          bossSpawnedTracker |= 0x01; //only spawn boss once!
          bossNumber = 0;
          boss.attribute = 1;
          boss.x = 28*8;
          boss.y =  25*8;
          boss.alive = 20;
          boss.moveSpeed = 2;
          boss.jumpSpeed = 6;
          boss.currentAnimation = 0;
          boss.startOfAnimations = 5;
          //seettings for world 1 boss

          chain.alive = 2;

        }
      }

      //update every actor...
      for(i = 0; i < NumActors; i++)
      {
        if(allActors[i]->isSprite == false)
        {
          //since i = 0 is alwyas player...
          if(allActors[i]->alive > 0 || i == 0)
          {
            byte normal = true;
            if(allActors[i]->startOfAnimations == 10)
            {
              normal = false; //the chain
            }
            cur_oam = updateMetaActor(allActors[i], cur_oam, i, normal);

            if(i == 0 || i == 1)
            {
              //display reticles
              if(debug == true || i == 0)
              {
                cur_oam = oam_spr(selectedPosition[0]*8, selectedPosition[1]*8, SELECTED, selectedPosition[2], cur_oam);
              }
            }

          }
        }
        else
        {
          if(allActors[i]->alive > 0)
          {
            cur_oam = updateActor(allActors[i], cur_oam, i);
          }
        }
      }




      //update particles!
      if(numActive)
      {
        for(i = 0; i < NUM_BRICKS; i++)
        {
          if(singleBricks[i].lifetime > 0)
          {


            if(boss.alive <= 0 && justKilledBoss && deathTimer > 0)
            {
              singleBricks[i].x = singleBricks[i].x + singleBricks[i].dx/deathTimer;
              singleBricks[i].y = singleBricks[i].y + singleBricks[i].dy/deathTimer;
            }
            else
            {
              singleBricks[i].x = singleBricks[i].x + singleBricks[i].dx;
              singleBricks[i].y = singleBricks[i].y + singleBricks[i].dy;
            }


            if(singleBricks[i].lifetime != 1)
            {
              singleBricks[i].lifetime--;
            }
            else
            {
              singleBricks[i].lifetime = 0;
              numActive--;
            }
            cur_oam = oam_spr(singleBricks[i].x, singleBricks[i].y, singleBricks[i].sprite ,singleBricks[i].attribute, cur_oam);
          }
        }
      }

    }


    //render all actors
    for(i = 0; i < NumActors; i++)
    {
      //first obj is always player
      if(allActors[i]->alive > 0 || i == 0)
      {
        if(allActors[i]->isSprite == false)
        {
          cur_oam = oam_meta_spr(allActors[i]->x, allActors[i]->y, cur_oam, MetaTable[allActors[i]->currentAnimation]);
        }
        else
        {
          cur_oam = oam_spr(allActors[i]->x, allActors[i]->y, allActors[i]->currentAnimation , allActors[i]->attribute, cur_oam);
        }
      }
    }



    //boss death flash and stuff
    if(boss.alive <= 0 && deathTimer > 0 && deathTimer < 0xF)
    {
      boss.attribute = !(boss.attribute & 0x03) | boss.attribute & 0xFC;
      boss.x = 15*8;
      boss.dx = 0;
      boss.dy = 0;
      cur_oam = oam_meta_spr(boss.x, boss.y, cur_oam, MetaTable[boss.startOfAnimations]);
    }

    if(boss.alive <= 0 && justKilledBoss && deathTimer > 0)
    {
      if(deathTimer == 0xF)
      {
        //pal_fade_to(2);
        pal_bright(2);
      }
      deathTimer--;
      //oam_hide_rest(cur_oam);
    }
    else if(boss.alive <= 0 && justKilledBoss == false && deathTimer == 0)
    {
      numActive = NUM_BRICKS;

      for(i = 0; i < numActive; i++)
      {
        singleBricks[i].lifetime = brickLifetime;
        singleBricks[i].x = boss.x-5;
        singleBricks[i].y = boss.y;

      }

      pal_fade_to(6);
      deathTimer = 0xF;
      //oam_hide_rest(cur_oam);
    }

    if(deathTimer == 0 && justKilledBoss)
    {
      justKilledBoss = false;
      //oam_hide_rest(cur_oam);
    }


    //scroll world
    scrollWorld(transition, &player);

    oam_hide_rest(cur_oam);


    if((change&0x03) != 0)
    {
      //updates num blocks
      updateBombBlockLives(((change&0x02) >> 1) - (change&0x01), 0x02);
      change = change & (0xFF ^ 0x03);
    }

    if((change & 0x30) != 0)
    {
      updatePlayerHealth(1, &player);
      change = change & (0xFF ^ 0x30);
      if(player.alive <= 0)
      {
        player.alive = 10;
        updateBombBlockLives(-1, 0x03);
      }
    }
    
    

    ppu_wait_frame();
  }
}