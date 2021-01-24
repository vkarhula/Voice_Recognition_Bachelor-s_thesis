/*
 * laiva.h
 *
 *  Created on: 20.3.2009
 *      Author: Administrator
 */

#ifndef LAIVA_H_
#define LAIVA_H_

struct SHIP
{
   u_short size;
   u_short shipStatus;        //0=no hit, 1 = hit to one or more coordinates, not sunk, 2 = sunk
   u_short coordStatus[5];    //0=no hit, 1 = hit
   u_short coordinates[10];   //coordinates(x,y), x first, y next
   u_short nroCoordReady;     //how many coordinatepairs are ready
   u_short mark;              //mark of a ship
};

struct PLAYER {
   u_short playerCode;        // 1 = player1, 2 = opponent
   u_short shipsSetStatus;    // 0 = not ready, 1 = ready
   u_short playerStatus;      // 0 = free, 1 = reserved
   u_short nroSunkenShips;    // how many ships have been sunken
   u_short sunkenShips[5];    // {2, 3, 3, 4, 5} when all sunken
                          // {0, 3, 0, 0, 5} when cru3/sub3 and air5 sunken
                          // obs! (nroSunkenShips needs to be updated) when changes here
   u_short nroHits;           // no double hits -> needs to be read from the sea
                          // not from the announcements
   u_short allShipsSunken;    // true = 1, false = 0
};

enum playerStatus {FREE, RESERVED};
enum gameStatus {NOT_STARTED, PLAY_GOING_ON, CANCELLED};

struct GAME
{
   u_short turn;
   u_short playerAnnounced;       // 0 = no, 1 = player replied to our GET_REQ_OID1()
   u_long playerAddress;	//??
   u_short selectedOpponentHasAnswered; // 0 = no, 1 = yes
   u_short opponentAnswer;        // 0 = error, 1 = ok

   u_short coordGetOk;        // 0 = not ready, 1 = message received
   u_short coordSend;       // 0 = not ready, 1 = message received
   u_short resultsSend;       // 0 = not ready, 1 = results[5] send
   u_short resultsAsked;      // 0 = not asked, 1 = results asked

   // lippu tulosten laskemisen valmistumista varten
   u_short resultReady;
   // lippu kierrosten synkronointiin
   u_short startTurn;
   // ledien sytyttämistä varten, edellisen kierroksen ammusten lkm
   u_short previousTurn_SunkenShips;

   u_short gameStatus;            // 0 = not started, 1 = play going on, 1 = cancelled
   u_short speechMode;			//0 = off, 1 = on
   //u_short verboseMode;           // 0 = off, 1 = on
   struct PLAYER player[2];
};

struct MESSAGE
{
   u_short messageType;          // set_request, get_request, get_response, trap ->enum
   u_short OID;                  // values 1-6
   u_short turn;
   u_short address;              // broadcast address (several recipients), not broadcast address (only one recipient)
   u_short coordinates[10];      // 1st x first, 1st y second, 2nd x third ...
   u_short shotEffects[5];       // shot effects
   char string[40];          // if written
   u_short error;               // if error (=1)
   u_short ok;                  // if "ok" -> or string "ok" ? (=1)
   u_short messageStatus;       // 0 = not existing, 1 = received, (waiting?), 2 = handled
};

// Globaalit muuttujat
u_short sea[10][10]; // = {0};     // own sea
u_short oppSea[10][10]; // = {0};  // opponent's sea
u_short targetSea[10][10]; // = {0};  // opponent's sea, our guess, where we shoot
u_short oppTargetSea[10][10]; // = {0};  //sea, where opponent shoots
struct PLAYER player1;
struct PLAYER player2;
struct GAME game;
// Own sea
struct SHIP air5;
struct SHIP bat4;
struct SHIP cru3;
struct SHIP sub3;
struct SHIP des2;
// Opponent's sea
struct SHIP oppAir5;
struct SHIP oppBat4;
struct SHIP oppCru3;
struct SHIP oppSub3;
struct SHIP oppDes2;
//
u_short temp[10];  //for ship coordinates while searching the place
u_short targetSeaShotX[5];    // our sea
u_short targetSeaShotY[5];
u_short targetOppSeaShotX[5];    // opponent's sea, two player game()
u_short targetOppSeaShotY[5];
u_short targetOppShotX[5];    // opponent's game, OnePlayerGame()
u_short targetOppShotY[5];

// kaksinpeli
//TODO muuta u_short intiksi
u_short oneValue;
u_short length;
u_short ourCoord[10];			// our coordinates, sent to opponent sea
u_short oppCoord[10];			// coordinates send by opponent
//int ourCoord[10];
//int oppCoord[10];

//u_short oppSeaShotEffects[5]; //effects we caused to opponent's sea = results
u_short seaShotEffects[5];    // result how opponent's shots affected, will be returned to opp
u_short oppShotEffects[5];	//yksinpeli TODO poista oppSea -> opp...

//vastustajien lista
u_long vastustajaLista[20];
u_short u_shortLista[20];	//deb.

// lippu tulosten laskemisen valmistumista varten
//u_short resultReady;
// lippu kierrosten synkronointiin
//u_short startTurn;
// ledien sytyttämistä varten, edellisen kierroksen ammusten lkm
//u_short previousTurn_SunkenShips;

// verkkoliikenteen debuggausta varten (entinen player->nroHits)
//u_short old_nroHits;



//u_long opponentAddress;

void showSea(u_short*);
void showShip(struct SHIP * boat);
u_short findLocationToShip(struct SHIP * boat, u_short * sea, u_short * temp);
u_short seekRight(u_short * sea, u_short * i, u_short x, u_short y, u_short * temp);
u_short seekDown(u_short * sea, u_short * i, u_short x, u_short y, u_short * temp);
u_short seekLeft(u_short * sea, u_short * i, u_short x, u_short y, u_short * temp);
u_short seekUp(u_short * sea, u_short * i, u_short x, u_short y, u_short * temp);
u_short findRightX(u_short size, u_short * temp);
u_short findLeftX(u_short size, u_short * temp);
u_short findDownY(u_short size, u_short * temp);
u_short findUpY(u_short size, u_short * temp);
u_short findSecondCoordinate(struct SHIP * boat, u_short * sea, u_short * temp, u_short * i, u_short x, u_short y);
u_short findNextCoordinate(struct SHIP * boat, u_short * sea, u_short * temp, u_short * i, u_short x, u_short y);
u_short setShipToSea(struct SHIP * boat, u_short * temp, u_short * sea);
void askTarget(u_short * inX, u_short * inY);
u_short returnSeaValue(u_short x, u_short y, u_short * sea);
u_short markShipHit(struct SHIP * boat, u_short x, u_short y);
u_short didShipSink(struct SHIP * boat, u_short x, u_short y);
u_short checkShotEffects(struct SHIP * air5, struct SHIP * bat4, struct SHIP * cru3, struct SHIP * sub3, struct SHIP * des2, u_short x, u_short y, u_short * sea, u_short * targetSea, struct PLAYER * player);
void findLocationToAllShips(struct SHIP * air5, struct SHIP * bat4, struct SHIP * cru3, struct SHIP * sub3, struct SHIP * des2, u_short * sea, u_short * temp);
void showTwoSeaParallel(u_short * sea1, u_short * sea2);
void giveMaxFiveShots(u_short shotCount, u_short * shotX, u_short * shotY, u_short * sea);
void askMaxFiveShots(u_short shotCount, u_short * shotX, u_short * shotY);
void showGameStatus(void); //struct GAME * game);
void showPlayer(struct PLAYER * player);
void setShotsToTargetSea(u_short * sea, u_short * targetSea, u_short targetShotX, u_short targetShotY);
void showGameEndResult(void);    //struct GAME * game);
u_short returnShipHitCount(struct SHIP * boat);
u_short returnAllShipHits(struct SHIP * boat1, struct SHIP * boat2, struct SHIP * boat3, struct SHIP * boat4, struct SHIP * boat5);
void askIfUserWantsToCancel(void);   //struct GAME * game);
//u_short askPlayToBeStarted(void);
void startOnePlayerGame(void); //struct GAME * game);
void setupShip(struct SHIP * boat, u_short size, u_short mark);
//
void startTwoPlayerGame(void); //struct GAME * game);   //, u_short * oppCoord, u_short * ourCoord, u_short * seaShotEffects, u_short * oppSeaShotEffects);
void markShotEffectsToPlayerStruct(struct PLAYER * player, u_short result);
void resetShip(struct SHIP * boat, u_short size, u_short mark);
void resetWholeGame(void);  //struct GAME * game);
int findOpponent(void);
void setShotsToOurTargetSea(u_short * sea, u_short x, u_short y, u_short shotResult);
u_short countHitsFromSea(u_short * sea, u_short lowestHitValue);

//debuggaus
void tempTul(u_short * temp);


#endif /* LAIVA_H_ */
