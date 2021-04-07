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
  byte boolean; // 1111 1111, 0x01 = playerInAir,0x02 = lastFacingRight, 0x03 = isWalking, 0x04 = is attacking
} MetaActor;


void updateMetaActor(MetaActor * actor)
{
  byte lastFacingRight = actor->boolean & 0x01;
}