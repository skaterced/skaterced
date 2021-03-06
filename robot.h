#ifndef robot_h
#define robot_h

#include <Arduboy2.h>
//#include <stdlib.h>

#include "globals.h"
#include "weapon.h"
#include "function.h"
#include "tiles.h"

#define INVINCIBILITY 0x80

const unsigned char robots_bitmap[] PROGMEM = {
// width, height,
8, 8,
//0
//R1_0-3
0xff,0xff,0x83,0xd5,0xd6,0x9d,0x83,0xff,
0xff,0xff,0x83,0xd5,0x96,0xd5,0x83,0xff,
0xff,0xff,0x83,0x9d,0xd6,0xd5,0x83,0xff,
0xff,0xff,0x83,0xdd,0x9e,0xdd,0x83,0xff,
//exploding
0xff,0xd5,0xae,0xdb,0x9a,0xdf,0xae,0xd5,

//R_2_0-3
0xff,0xff,0x81,0xd5,0x9d,0xdd,0x81,0xff,
0xff,0xff,0x81,0x9d,0xd5,0x9d,0x81,0xff,
0xff,0xff,0x81,0xdd,0x9d,0xd5,0x81,0xff,
0xff,0xff,0x81,0x9d,0xdd,0x9d,0x81,0xff,
//exploding
0xff,0xd4,0xbe,0x9b,0xde,0x97,0xbe,0xd4,
//blasted
0xbf,0x9f,0xcf,0x97,0x87,0xcf,0x9f,0xbf
};

class Player {
  public :
    uint8_t x,y; //could this be uint8_t ?
    uint8_t weapons; //0 means normal Bombs
    uint8_t range;
    uint8_t dir; //if dir==DEAD : direction heaven 
    uint8_t lives=3;
    int score;
    bool moving=false;
    Bomb bombs[NB_BOMB_MAX];
    Player(uint8_t X, uint8_t Y) {
      this->x=X;
      this->y=Y;
      this->weapons=0;
      this->range=1;
      this->dir=0;      
      this->score=0;
    }
    void draw(bool player1){
      uint8_t frame=0; //WhiteOnBlack? 0:4;
      frame+=(dir&0x0F);
      if (!player1){
        frame+=BETWEEN_ROBOTS;
      }
      if (DEAD!=dir){
        if ((INVINCIBILITY!=(dir&INVINCIBILITY))||(0x02==(timer&0x02))){
          Sprites::drawOverwrite(x+leftBorder, y+upBorder, robots_bitmap, frame);          
        }
      }
      else {        
        if (timer<(HOLD_THRESHOLD+9)){
          Sprites::drawOverwrite(x+leftBorder, y+upBorder, robots_bitmap, (player1?  1:2)*BETWEEN_ROBOTS-1);
        }
        else {      
          Sprites::drawOverwrite(x+leftBorder, y+upBorder, robots_bitmap, 2*BETWEEN_ROBOTS);
        }
      }      
    }
    void move(uint8_t direction, uint8_t distance){
      switch (direction){
        case HAUT:
          y-=distance;
        break;
        case BAS:         
          y+=distance;
        break;
        case DROITE:         
          x+=distance;
        break;
       case GAUCHE:
          x-=distance;
        break;
      }
    }
    void drawBombs(void){
      for (uint8_t i=0; i<NB_BOMB_MAX; i++){
        if (bombs[i].counter!=0){
          bombs[i].draw();
        }
      }
    }
    bool placeBomb(void){  
      if ((tiles[getIndice(x,y)].walls&TILE_BOMB)==TILE_BOMB)
        return false;
      for (uint8_t i=0; i<NB_BOMB_MAX; i++){
        if (0==bombs[i].counter){
          bombs[i].x=x;
          bombs[i].y=y;
          bombs[i].counter=(twoPlayersMode? BOMB_DEFAULT_T : BOMB_DEFAULT_T-3);          
          if (WEAPON_DETO==weapons){
            bombs[i].counter=255;
          }
          tiles[getIndice(x,y)].walls|=TILE_BOMB;
          return true;
        }
      }
      return false;
    }
    void init(bool player1){
      if (player1){
        dir=0xD2;
        x=1;
      }
      else {
        dir=0xD0;
        x=71;
      }      
      y=31;
      //weapons=0;
      for (uint8_t i=0; i<NB_BOMB_MAX; i++){
        bombs[i].counter=0;
      }
    }
};

class Player p1(1,31);
class Player p2(71,31);

void explode(uint8_t ind, uint8_t range){
  tiles[ind].walls=(tiles[ind].walls&0xF0)|TILE_EXPLODING;
  int temp[BOMB_RANGE_MAX+1]={ind,-1,-1,-1,-1,-1,-1,-1,-1};
  if (range>BOMB_RANGE_MAX)
    range=BOMB_RANGE_MAX;
  for (uint8_t j=0; j<4; j++){ //explode in all directions
    for (uint8_t i=0; i<range; i++){
      if (canGoTo(temp[i],j,EXPLOSION)){
        temp[i+1]=voisin(temp[i],j);
        if (-1!=temp[i+1]){
          if (TILE_BOMB==(tiles[temp[i+1]].walls&TILE_BOMB)){
            uint8_t range2=0;
            Player* pp=&p1;
            for (uint8_t k=0; k<2; k++){
              for (uint8_t j=0; j<NB_BOMB_MAX; j++){
                if (getIndice(pp->bombs[j].x,pp->bombs[j].y)==ind){
                  range2=pp->range;
                  break;
                }
              }
              pp=&p2;
            }
            explode(temp[i+1],(0!=range2)?range2:range);
          }
          tiles[temp[i+1]].walls=((tiles[temp[i+1]].walls&0xF0)|TILE_EXPLODING);
        }
        else break;
      }
      else break;
    }
  }
}

bool checkBombs(void){
  bool boom=false;
  Player* pp=&p1;
  for (uint8_t j=0;j<2;j++){
    for (uint8_t i=0; i<NB_BOMB_MAX; i++){
      if ((pp->weapons!=WEAPON_DETO)&&(pp->bombs[i].counter>100)){
        pp->bombs[i].counter=3;
      }      
      if (0!=pp->bombs[i].counter){
        if (0==--pp->bombs[i].counter){
          explode(getIndice(pp->bombs[i].x,pp->bombs[i].y),pp->range);
          boom=true;
        }
      }
    }
    pp=&p2; 
  }
  for (uint8_t i=0;i<NBTILES;i++){ //checking if there is an external explosion. For ex. triggered by a nuke
    if (TILE_EXPLODING==(tiles[i].walls&TILE_EXPLODING)){
      boom=true;
    }
  }
  if (boom){ //check if there's a chain reaction
    for (uint8_t j=0;j<2;j++){
      for (uint8_t i=0; i<NB_BOMB_MAX; i++){        
        if (0!=pp->bombs[i].counter){
          if ((tiles[getIndice(pp->bombs[i].x,pp->bombs[i].y)].walls&TILE_EXPLODING)==TILE_EXPLODING){
            explode(getIndice(pp->bombs[i].x,pp->bombs[i].y),pp->range);
            pp->bombs[i].counter=0;
            i=0; // because it might have triggered another...
          }
        }
      }
      pp=&p1;
    }
  }
  return boom;
}

void controlRobot(Player * pp){ //check if arrow key is pressed, check if move is possible and then move
  uint8_t dir=99;
  bool stay=true;
  //uint8_t walls=0;
  uint8_t temp;
  // uint8_t movesBack=movesLeft;
  /*Player* pp=&p2;

  if (p1Playing){
    pp=&p1;
  }*/
  temp=getIndice(pp->x,pp->y);
  //walls=tiles[temp].walls;
  if (arduboy.justPressed(B_BUTTON)){ //B action (Bomb for now)    
    if (arduboy.pressed(A_BUTTON)){

    }
    else {
      if (pp->placeBomb()){
        hold=true;
        timer=HOLD_THRESHOLD-1;
      }
    }
  }
  else if (arduboy.justPressed(UP_BUTTON)){
    if (arduboy.pressed(A_BUTTON)){
      //secondary weapon (or switch weapon)
      if (WEAPON_DETO==pp->weapons){
        for (uint8_t i=0; i<NB_BOMB_MAX; i++){
          if (pp->bombs[i].counter>0){  
            pp->bombs[i].counter=1;
            hold=true;
            timer=HOLD_THRESHOLD-1;
          }
        }
      }
      else if ((WEAPON_MINE&pp->weapons)!=0){
        for (uint8_t i=0; i<NB_MINE_MAX; i++){
          if (!mines[i].active){
            uint8_t temp=(pp->weapons&WEAPON_MINE)>>6;
            temp--;
            pp->weapons&=~WEAPON_MINE;
            pp->weapons|=(temp<<6);
            tiles[getIndice(pp->x,pp->y)].walls|=TILE_TBD;
            mines[i].x=pp->x;
            mines[i].y=pp->y;
            mines[i].active=true;
            hold=true;
            timer=HOLD_THRESHOLD-1;
            break;
          }
        }
      }
      else if (WEAPON_SHUFFLER==pp->weapons){        
        for (uint8_t i=0;i<NBTILES;i++){
          if (((temp%casesCol)==i%casesCol)||((temp/casesCol)==i/casesCol)){
            for (uint8_t j=0;j<4;j++){
              if (random(100)<50){
                tiles[i].turn(true);
                imposeWall(i,true);
              }
            }
          }
        }
        hold=true;
        timer=HOLD_THRESHOLD-1;
      }
      else if ((pp->weapons&WEAPON_NUKE)!=0){        
        //arduboy.pollButtons(); //to avoid firing north. Usefull?
        hold=true;
        timer=1; //special case
      }
      else if (WEAPON_TELEPORT==pp->weapons){

        timer=3; //another special case
        hold=true;
        cursX=cursY=41;
      }  
    }
    else {
      dir=HAUT;      
      //if ((pp->y>=casesHeight)&&((walls&WALL_UP)!=WALL_UP)){
      if (canGoTo(temp,dir,0)){
        pp->move(dir, casesHeight);
        stay=false;
      }
    }
  }
  else if (arduboy.justPressed(DOWN_BUTTON)){
    if (arduboy.pressed(A_BUTTON)){
      //select weapon or just EoT...
      movesLeft=1; //because now "movesLeft-- is done outside this function
      hold=true;
      timer=HOLD_THRESHOLD-1;
    }
    else {
      dir=BAS;
      //if ((pp->y<((casesRow-1)*casesHeight))&&((walls&WALL_DOWN)!=WALL_DOWN)){
      if (canGoTo(temp,dir,0)){
        stay=false;
        pp->move(dir, casesHeight);
      }
    }
  }
  else if (arduboy.justPressed(RIGHT_BUTTON)){
    if (arduboy.pressed(A_BUTTON)){
      if (0!=tiles[temp].walls){
        tiles[temp].turn(false);
        imposeWall(temp, true);
        hold=true;      
        timer=HOLD_THRESHOLD-1;
      }
    }
    else {
      dir=DROITE;
      //if (((pp->x<((casesCol-1)*casesLength))&&((walls&WALL_RIGHT)!=WALL_RIGHT))||((pp->x<0)&&(pp->y==31))){
      if (canGoTo(temp,dir,0)){
        pp->move(dir, casesLength);
        stay=false;        
      }
    }
  }
  else if (arduboy.justPressed(LEFT_BUTTON)){
    if (arduboy.pressed(A_BUTTON)){
      if (0!=tiles[temp].walls){
        tiles[temp].turn(true);
        imposeWall(temp, true);
        hold=true;
        timer=HOLD_THRESHOLD-1;
      }
    }
    else {
      dir=GAUCHE;
      //if (((pp->x>=(casesLength))&&((walls&WALL_LEFT)!=WALL_LEFT))||((pp->x>70)&&(pp->y==31))){        
      if (canGoTo(temp,dir,0)){
        pp->move(dir, casesLength);
        stay=false;
      }
    }
  }
  if (99!=dir){ //
    //pp->dir=dir;
    pp->dir&=0xF0;
    pp->dir|=dir;
    if (false==stay){         
      if ((p1.x==p2.x)&&(p1.y==p2.y)){//||((tiles[getIndice(pp->x,pp->y)].walls&0x07)>0)){
        //return false;    
      }
      else { //valid move
        //movesLeft--; //inside checkMoving
        //pp->dir|=0x20; //adds a "moving timer"
        pp->moving=true;
        pp->move(pp->dir&0x0F,5);
        hold=true;
        timer=HOLD_THRESHOLD-2;
        //return true;
      }
      dir+=2; //move back
      if (dir>3)
        dir-=4;
      pp->move(dir, casesLength); //just because length=height      
    }
  }
  /*if (hold&&(INVINCIBILITY==(pp->dir&INVINCIBILITY))){ -> in testEOT()
    uint8_t temp=(pp->dir&0x70)>>4;
    pp->dir=pp->dir&0x0F;
    if (--temp>0){
      pp->dir|=((temp<<4)|INVINCIBILITY);
    }
  }*/
}

void checkMoving(void){
  Player* pp=&p2; //or monster?

  if (p1Playing){
    pp=&p1;
  }
  if (pp->moving){
    pp->move(pp->dir&0x0F,5);
    pp->moving=false;    
  }
}

bool testEOT(Player * pp){
  if (INVINCIBILITY==(pp->dir&INVINCIBILITY)){
      uint8_t temp=(pp->dir&0x70)>>4;
      pp->dir=pp->dir&0x0F;
      if (--temp>0){
        pp->dir|=((temp<<4)|INVINCIBILITY);
      }
    }
  if ((--movesLeft == 0) && (twoPlayersMode)) { //retest because of the bomb skip...
    movesLeft = movesInit;
    p1Playing = !p1Playing;    
    return true;
    //timer+=3;
  }
  return false;
}

#endif
