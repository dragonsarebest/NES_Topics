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
  //tracks, ISBLOCKED, isBlockable, is boss, is Player, isAttacking, lastFacingRight, playerInAir
  char isSprite;
  
  int animationTimer; //0-15
  char jumpTimer;
  short fallTimer;
  
  byte hurtFlash; //how long to flash for
  char currentAnimation;
  char startOfAnimations;
} Actor;
