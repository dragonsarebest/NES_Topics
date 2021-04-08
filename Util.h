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
  byte boolean; // 1111 1111, 0x01 = playerInAir,0x02 = lastFacingRight, 0x04 = isAttacking, 0x08 = is Player, 0x10 = is boss, 0x20 = isBlockable, 0x40 = ISBLOCKED
  int animationTimer; //0-15
  char jumpTimer;
  short fallTimer;
  
  byte hurtFlash; //how long to flash for
  char currentAnimation;
  char startOfAnimations;
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


