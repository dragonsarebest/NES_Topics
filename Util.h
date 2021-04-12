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
  unsigned char alive; //health value - or a boolean
  unsigned char moveSpeed;
  unsigned char jumpSpeed;
  
  char grounded;
  byte boolean;
  //0x80, 0x40, 0x20, 0x10...
  //tracks, ISBLOCKED, isBlockable, isboss, isPlayer, isAttacking, lastFacingRight, playerInAir
  char isSprite;
  
  int animationTimer; //0-15
  char jumpTimer;
  short fallTimer;
  
  byte hurtFlash; //how long to flash for
  char currentAnimation;
  char startOfAnimations;
} Actor;
