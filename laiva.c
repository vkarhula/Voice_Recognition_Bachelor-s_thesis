/*
 * laiva.c
 *
 *  Created on: 20.3.2009
 *      Author: Administrator
 */

// vk 2003 klo 1320
// siirto sulautettuun alkaa
// 1.4 muokkaukset 2503 klo 1230 versiosta, k‰‰ntyy, mutta peli‰ ei voi k‰ynnist‰‰ uudelleen
// t‰ss‰ versiossa ei sekoilua oppSea -muutosten kanssa

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "verkko.h"
#include "laiva.h"
#include "main.h"

/*****
Rajapintakuvaus
Verkkoliikennett‰ k‰sittelee kootusti kaksi funktiota: x1 ja x2.
Funktio x1 vastaanottaa tulevat viestit ja purkaa auki SMNP-paketit. SNMP-paketti ohjataan edelleen
funkitolle x3, joka ohjaa viestit tunnistetietojen (int OID, yms.) mukaan eteenp‰in.
Funktio x2 k‰sittelee ohjelmasta tulevat l‰hetett‰v‰t viestit ja ohjaa ne funktiolle x4,
joka muodostaa viestidatasta SNMP-protokollan mukaisen paketin.
Globaalilla muistialueella sijaitsevat pelin pelitapahtumatiedot (struct GAME * game),
jotka ohjaavat pelilogiikan kulkua yksin- ja kaksinpelin aikana.
Kaksinpeliss‰ v‰litett‰v‰t arvot sijoitetaan globaalilla muistialueella sijaitseviin
muuttujiin: int oneValue,
//TODO u_short -> int
int length;
u_short ourCoord[10];			// our coordinates, sent to opponent sea
u_short oppCoord[10];			// coordinates send by opponent

u_short oppSeaShotEffects[5]; //effects we caused to opponent's sea = results
u_short seaShotEffects[5];    // result how opponent's shots affected, will be returned to opp
u_short oppShotEffects[5];	//yksinpeli TODO poista oppSea -> opp...

*/


// laske osumat merelt‰,
// lowestHitValue = 3, kun osumat merkitty 3, uppoama isommalla=(2,3,4,5)+2
/*! \fn u_short countHitsFromSea(u_short * sea, u_short lowestHitValue)
 *  \brief Funktio, joka laskee osumat merelt‰.
 *  \param *sea Meri, josta osumia lasketaan.
 *  \param lowestHitValue ###########################################
 *  \return .
 */
u_short countHitsFromSea(u_short * sea, u_short lowestHitValue){
	u_short i = 0, j = 0;
	u_short count = 0;

	for(j=0; j<10; j++){
		for(i=0; i<10; i++){
			if(*(sea + i + 10*j) >= lowestHitValue){
				count++;
			}
		}
	}
	return count;
}

/*! \fn int findOpponent(void)
 *  \brief Funktio pelikaverin etsint‰‰n verkon yli.
 *  \return 0, jos palataan p‰‰valikkoon, muulloin 1.
 */
int findOpponent(void){
	int i = 0, add = 0;
	char ch;

	// PELIKAVERIN ETSIMINEN

	   game.selectedOpponentHasAnswered = 0;
	   do {
	      //sendGetRequestForSeveralPossibleOpponents_GR_OID1();
	      oneValue = 0;
	      length = 0;

	      parseOutboundMessage(1, &oneValue, length);

	      //obs! jos vastaanottaja l‰hett‰‰ GET_RESPONSE_OID1(ok)
	      //     => asetetaan game->playerAnnounced == 1 ja game->playerAddress = ip
	      printf("\nOdotetaan vastaanottajien vastauksia. Anna 1, jos haluat lopettaa odottelun.\n\n");
	      do{

	    	 // Toistetaan l‰hetyst‰ aina kun tulostetaan lista
	    	 //parseOutboundMessage(1, &oneValue, length);

	         // Kuunnellaaan vastaanottajien vastauksia pyyntˆˆmme
	         //printf("\ngame->playerAnnounced = %d", game.playerAnnounced); //
	         //showGameStatus(); //

	         //if (game->playerAnnounced == 1){
	         if (game.playerAnnounced == 1){
	        	 //printf("\nif (game->playerAnnounced == 1){");
	            // movePlayerToList(char/int playerCode, int address);  //listan voisi ker‰t‰ eri s‰ikeess‰
	        	for(i = 0; i < 20; i++){
	        		if(vastustajaLista[i] == game.playerAddress){
	        			add = 0;
	        			i = 20;
	        		} else {
	        			add = 1;
	        		}
	        	}

	        	 for(i = 0; i < 20; i++){
	        	   if(add == 1 && vastustajaLista[i]==0){
	        		  //printf("\nkirjoitetaan vastustaja listaan");
	        		  vastustajaLista[i] = game.playerAddress;
	        	      i = 20;	//exit
	        	   }
	        	}
	            // tulostaa kaikki listan vastustajat
	        	printf("\nLista vapaista vastapelaajista:");
	        	for(i = 0; i < 20; i++){
	        	   if(vastustajaLista[i]!=0){
	        		  printf("\n%d. %s", i, inet_ntoa(vastustajaLista[i]));
	        		  //printf("\n\t u_shortLista[%d] = %d", i, u_shortLista[i]);
	        	   }
	        	}
	        	printf("\n");
	            // ip-address: printf("\n%s", inet_ntoa(vv.osoite));

	        	game.playerAnnounced = 0;
	         }
	         // Vastataan vastustajien l‰hett‰miin pelipyyntˆihin
	         //if (game->playerAsked_XXX == 1){

	         NutSleep(500);
	         printf("1 lopettaa vastustajien etsimisen...\n");
	         printf("0 palaa p‰‰valikkoon.\n");

	         //printf("while:ssa: vastustajaLista[0]: %s\n", inet_ntoa(vastustajaLista[0]));
	         ch = getchar();

	      } while ( ch!='1' && ch!='0' );//!getchar());  //getchar() == 1 );   //onko toimiva lopetusehto?

	      if ( ch == '0')
	    	  return 0;		//Palataan p‰‰valikkoon

	      //printf("while:n j‰lkeen: vastustajaLista[0]: %s\n", inet_ntoa(vastustajaLista[0]));
	      //showList();  //listassa nimi/nro ja ip-osoite
	      //selectOpponentFromList(); -> ota talteen ip -> vIP
	      printf("\nValitse sopiva vastustaja listasta. Anna indeksinumero: ");
	      i = (u_short)getc(stdin);
	      i = i - 48;

	      game.playerAddress = vastustajaLista[i];
	      //opponentAddress = vastustajaLista[i];

	      printf("Valittu pelaaja: %s\n", inet_ntoa(vastustajaLista[i]));
	      printf("game.playerAddress: %s\n", inet_ntoa(game.playerAddress));
	      //printf("opponentAddress: %s\n", inet_ntoa(opponentAddress));

	      //send_request_OID2_yhdelle();
	      oneValue = 0;
	      length = 0;
	      parseOutboundMessage(2, &oneValue, length);

	      while (game.selectedOpponentHasAnswered == 0){
	         NutSleep(50);
	         printf("\nOdotetaan vastaanottajan vastausta");
	      }
	      NutSleep(50);
	      if (game.opponentAnswer == 1){           //ok
	         printf("\nPeli valitun vastustajan kanssa voi alkaa.\n\n");
	         game.player[1].playerStatus = RESERVED;
	      }

	   } while (game.opponentAnswer == 0);

	   //default values back
	   game.opponentAnswer = 0;

	    // changes playerStatus to reserved state
	   // game.player[1].playerStatus = RESERVED;
	   // -> funktio, joka l‰hett‰‰ sautom errorviestit, jos tulee kyselyit‰, kun tila RESERVED

	   return 1;

}


// meid‰n meri: sea
// vastustaja ampuu meid‰n (sea) merelle: targetSea
// oppCoord[10], targetSeaShotX[5], targetSeaShotY[5], seaShotEffects[5]
// me ammumme vastustajan merelle: oppTargetSea
//targetOppShotX[5] ja ...Y[5], ourCoord[5], oppShotEffects[5] = results
/*! \fn void startTwoPlayerGame(void)
 *  \brief Funktio kaksinpeliin.
 *  \return void.
 */
void startTwoPlayerGame(void) { //struct GAME * game){  //, u_short * oppCoord, u_short * ourCoord, u_short * seaShotEffects, u_short * oppShotEffects){
	u_short i = 0;
	u_short turn = 0;
	//u_short add = 0;

	// moved to main:
	//int seaShotEffects[5] = {0};    // result how opponent's shots affected, will be returned to opp
	//int oppShotEffects[5] = {0};      //effects we caused to opponent's sea

	u_short endGame = 0;
	u_short choice = 0;
	u_short ok1 = 0, ok2 = 0, ok3 = 0;
	u_short sendResultOk = 0;
	u_short x = 0, y = 0, z = 0;
	u_short j = 0;
	u_short viive = 0;
	//char ch;


	turn = 1;	// set_requestia varten: laivat valmiit -ilmoitus
	//game.turn = 1;

	// LAIVOJEN ASETTAMINEN, player1 and player2
	// Own boats
	findLocationToAllShips(&air5, &bat4, &cru3, &sub3, &des2, sea, temp);
	// Opponent's boats on opponent's sea
	//TODO tarkista, voiko t‰m‰n poistaa
	//findLocationToAllShips(&oppAir5, &oppBat4, &oppCru3, &oppSub3, &oppDes2, oppSea, temp);

	// HAS OPPONENT GOT HIS/HER BOATS READY?
	// VASTAANOTA SET-REQUEST, OID = 2, int game.turn

	// Ensin tarkistetaan, onko vastustaja ehtinyt jo kysy‰ meilt‰
	// ovatko laivamme valmiit, jos niin vastataan viestiin
	// Ellei ole, l‰hetet‰‰n kysely ja odotetaan vastausta

	if (game.player[0].shipsSetStatus == 1) { // opponent has set his/her boats
		// LƒHETƒ VASTAUS
		//sendBoatsReady_SR_OID2();  //obs!onko sama vastausksena kuin kysymyksen‰?

		//TODO: Mauni siirt‰‰ seuraavat rivit verkon puolelle
		//TODO: , muuten systeemi ei ole en‰‰ synkassa, kun SNMP-viestej‰ tulee paljon...
		//TODO: Maunille: T‰st‰ l‰htee ylim‰‰r‰inen GET-RESPONSE
		//TODO:
		// siirretty verkon puolelle seuraavat rivit:
		oneValue = 0; //TODO: tarkista onko nolla? -> yksi //p.o. 0 //maunin kuutiointi...('Laivat valmiina' kuittaus, kuittaukset aina 0 (paitsi jos error :) ))

		//////////
		//TODO x t‰h‰n muutos, viittaus ko.kyselyviestiin, kun l‰hetet‰‰n vastaus
		//parseSnmpResponse(&SR_2, 2, &oneValue);

		parseSnmpResponse(2, &oneValue);
		printf("\nVastataan vastustajan l‰hett‰m‰‰‰n Laivat valmiit? -kyselyyn");

	} else {
		// send your announcement
		// LƒHETƒ SET-REQUEST, OID = 2, int turn
		//sendBoatsReady_SR_OID2();  //obs!onko sama vastausksena kuin kysymyksen‰?
		oneValue = turn;	//turn = 1
		length = 1;

		parseOutboundMessage(2, &oneValue, length);
		printf("\nL‰hetet‰‰n vastustajalle Laivat valmiit? -kysely");

		// WAIT FOR RESPONSE
		while (game.player[0].shipsSetStatus == 1) {
			NutSleep(125);
		}
	}
	game.startTurn = 0;	// nollataan, jotta autetaan 'erottamaan' Get_response(Ok)-vastausviestit toisistaan

	//deb.
	printf("\nMolemmat pelaajat ovat saaneet asetettua laivat paikoilleen.");
	printf("\nja l‰hett‰neet ja kuitanneet viestit.");
	// tila-arvot takaisin oletusarvoihin
	game.player[0].shipsSetStatus = 0; // jos per‰kk‰isi‰ pelej‰ (?)

	// obs! -> message interface
	//obs! kun vastustaja l‰hett‰‰ (SET-REQUEST, OID = 2, int turn) -viestin
	//t‰ytyy asettaa:
	//   game.player[0].shipsSetStatus == 1

	printf("\nKuinka haluat pelata?"
		"\n1 = tietokone pelaa puolestani arpoen satunnaisesti ammusten sijainnit"
		"\n2 = annan itse kohdekoordinaatit \nAnna valintasi: ");
	choice = (u_short) getc(stdin);

	// PELIN ALOITTAMINEN
	while (!endGame) {

		//turn++;
		//game.turn = turn;
		game.resultReady = 0;

		//showTwoSeaParallel(sea, oppSea);

		// vastustajan kohdemeri, oikeat laivat ja vastustajan ammunnat  -????
		printf("\nVastustajan kohdemeri, vas. oikeat laivat, oik. vastustajan ammunnat");
		showTwoSeaParallel(sea, oppTargetSea);
		// meid‰n ammuntameri
		printf("\nMeid‰n ammunnat vastustajan merelle");
		showSea(targetSea);

		// kohdekoordinaattien asettaminen (player1 automaattinen, player2 k‰sin)

		// Our sea, opponent's target
		/*
		 giveMaxFiveShots(5-game.player[0].nroSunkenShips, targetSeaShotX, targetSeaShotY, targetSea);
		 for(i = 0; i < 5; i++){
		 //printf("\n%d, %d", targetSeaShotX[i]+1, targetSeaShotY[i]+1);        //-1 korjaus (0,9)->(1,10)
		 }*/
		//printf("\n");  //deb.

		// Opponent's sea, our target
		if (choice == '1') {
			// random shots
			giveMaxFiveShots(5 - game.player[1].nroSunkenShips, targetOppShotX,
					targetOppShotY, oppTargetSea);
		} else { // works with all other values than 1
			// shots will be asked from the user
			// using keyboard, when verbose mode is off
			askMaxFiveShots(5 - game.player[1].nroSunkenShips, targetOppShotX, targetOppShotY);
		}

		// Yhdistet‰‰n erilliset x ja y koordinaattitaulukot yhdeksi
		// ourCoord[] sis‰lt‰‰ l‰hett‰m‰mme koordinaatit vihollismerelle
		j = 0;
		printf("\nmeid‰n l‰hett‰m‰t koordinaatit");  //
		for (i = 0; i < 5; i++) {

			///
			printf("\t%d, %d", targetOppShotX[i], targetOppShotY[i]);

			*(ourCoord + j++) = targetOppShotX[i];
			*(ourCoord + j++) = targetOppShotY[i];
		}

		// TARGET COORDINATES
		// Odotetaan,
		// kunnes vastustaja on l‰hett‰nyt 10 koordinaattia ja
		// vastustaja on vastaanottanut 10 koordinaattia

		// tilamuuttujat oletusarvoihin
		game.coordGetOk == 0; // for next turn (default value)
		//game.coordSend == 0; // for next turn (default value)
		ok1 = ok2 = ok3 = viive = 0;

		do {
			// 1) tarkista, onko vastustaja jo l‰hett‰nyt meille 10 koordinaattiaan
			if (game.coordSend == 1 && ok1 == 0) {
				// l‰het‰ ok viesti ett‰ koordinaatit[10] saapuneet
				//sendTargetCoordinatesGot_Ok_GetResp_OID3(); -> verkkopuolen koodissa
				printf(	"\n###Vastustaja on l‰hett‰nyt koordinaattinsa.");
				printf("\n-> autom. vastaus verkon puolelta.");
				ok1 = 1;

				// siirretty verkon puolelta t‰nne
				oneValue = 0x00;
				parseSnmpResponse(3, &oneValue);
			}
			//NutSleep(125);

			// omien koordinaattien l‰hetys ja kuittauksen odotus
			// uudelleen l‰hetys n. 10 s kuluttua
			if (ok2 == 0 || viive > 20) {
				// sending target coordinates to opponent's sea
				//sendTargetCoordinates_SR_OID3(int ourCoord[10]);
				length = 10;
				parseOutboundMessage(3, ourCoord, length);
				ok2 = 1;
				viive = 0;
				printf("\n###Omat koordinaatit l‰hetetty");
				printf("\nok1 = %d, ok2 = %d, ok3 = %d", ok1, ok2, ok3);
			}
			//NutSleep(125);	klo1230

			// 2) onko vastustaja l‰hett‰nyt kuittauksen l‰hetetyist‰ 10 koorinaatista
			if (game.coordGetOk == 1 && ok3 == 0) {
				ok3 = 1;
				printf("\n###Vastustaja on kuitannut l‰hett‰m‰mme koordinaatit");
			}
			//NutSleep(125);
			NutSleep(50);
			viive++;

			printf("\nok1 = %d, ok2 = %d, ok3 = %d", ok1, ok2, ok3);

		} while (ok1 == 0 || ok2 == 0 || ok3 == 0);
		printf("\n#####SALVO ohi.");

		game.coordSend = 0; // for next turn (default value)

		// Read coordinates, which opponent send
		// read coordinates from oppCoord[10]
		// separate to targetSeaShotX[5], targetSeaShotY[5]
		j = 0;
		printf("\nVastustajan l‰hett‰m‰t koordinaatit");	//
		for (i = 0; i < 5; i++) {
			targetSeaShotX[i] = oppCoord[j++];
			targetSeaShotY[i] = oppCoord[j++];
			printf("\n\t(%d, %d), ", targetSeaShotX[i], targetSeaShotY[i]);  //
		}

		// obs! -> message interface
		// obs! when opponent has got our coordinates, and send message 'ok'
		// has to be set: game.coordGetOk == 1
		// obs! kun olemme vastaanottaneet vastustajan 10 koordinaattia
		// lippu game.coordSend == 1 ylˆs

		// ANALYZING SHOT EFFECTS
		// kohde- ja osumakoordinaattien analysointi (player1 ja player2, meri1 ja meri2)
		// Our sea, opponent't target
		printf("\nVastustajalle l‰hetett‰v‰t tulokset: ");
		for (i = 0; i < 5 - game.player[0].nroSunkenShips; i++) {
			// seaShotEffects[5] sis‰lt‰‰ viisi vastustajalle palautettavaa tulosarvoa
			seaShotEffects[i] = checkShotEffects(&air5, &bat4, &cru3, &sub3,
					&des2, targetSeaShotX[i], targetSeaShotY[i], sea,
					targetSea, &game.player[0]);
			setShotsToTargetSea(sea, targetSea, *(targetSeaShotX + i),
					*(targetSeaShotY + i));

			////
			//markShotEffectsToPlayerStruct(&game.player[0], seaShotEffects[i]);

			//printf("\n seaShotEffects[%d] = %d", i, seaShotEffects[i]);
			printf("%d ",  seaShotEffects[i]);
		}

		// Odotetaan, ett‰ tulokset analysoitu ennen kuin l‰hetet‰‰n verkon
		// puolella kysyj‰lle
		game.resultReady = 1;

		// HIT RESULTS
		ok1 = 0;
		sendResultOk = 0;
		//game.resultsAsked = 0;
		game.resultsSend = 0;
		do {
			if (ok1 == 0) {
				// Ask for hit results
				//askHitResultsFromOpponent_GetReq_OID4();
				oneValue = 0;
				length = 0;
				parseOutboundMessage(4, &oneValue, length);
				ok1 = 1;
			}
			//NutSleep(125);
			NutSleep(50);

			// Send shot results to opponent when asked, will be send only once
			//T‰m‰ verkon puolelle 14.4.2009
			  if (game.resultsAsked == 1 && sendResultOk == 0) {
				//sendOpponentHitResults_GR_OID4_5integers(seaShotEffects[5]);  //mainissa seaShotEffects[5]

				//////
				parseSnmpResponse_ok(&GR_4, 4, seaShotEffects);

				//length = 5;
				//parseOutboundMessage(4, seaShotEffects, length);
				sendResultOk = 1;
			}
			//NutSleep(125);

printf("\n########game.resultsAsked = %d, game.resultsSend = %d", game.resultsAsked, game.resultsSend);

			// Odota,
			// kunnes vastustaja on pyyt‰nyt tuloksia
			// ja kunnes vastustaja on l‰hett‰nyt osumavastaukset (game.resultsSend == 1)
		} while (game.resultsAsked == 0 || game.resultsSend == 0);
		game.resultsAsked = 0;
		//game.resultsSend = 0;

		//obs!
		// game.resultsAsked == 1, kun vastustaja kysyy tuloksia
		// game.resultsSend == 1, kun vastustaja on l‰hett‰nyt osumavastaukset

		// ANALYZING SHOT EFFECTS
		// Opponent's sea, our target
		printf("\nMeid‰n ammustemme aih.tulokset: ");
		for (i = 0; i < 5 - game.player[1].nroSunkenShips; i++) {
			//vastustajan antamien tulosten sijoittaminen kohdemerelle, oppTargetSea ?
			x = targetOppShotX[i];
			y = targetOppShotY[i];
			z = *(oppShotEffects + i);

			printf("\n%d ", z);

			//TODO tarkista miss‰ vektoreissa kulkee meid‰n ja vast. kohteet ja resultit!!!
			// meid‰n meri: sea
			// vastustaja ampuu meid‰n (sea) merelle: targetSea
			// oppCoord[10], targetSeaShotX[5], targetSeaShotY[5], seaShotEffects[5]
			// me ammumme vastustajan merelle: oppTargetSea
			//targetOppShotX[5] ja ...Y[5], ourCoord[5], oppShotEffects[5] = results

			//
			//oppTargetSea[x][y] = z; //asettaa tulokset targetSea:lle
			//targetSea[x][y] = z; //asettaa tulokset targetSea:lle

			/////
			//setShotsToTargetSea(sea, targetSea, *(targetSeaShotX + i),
			//		*(targetSeaShotY + i));
			setShotsToOurTargetSea(oppTargetSea, *(targetOppShotX + i),
					*(targetOppShotY + i), z);

			markShotEffectsToPlayerStruct(&game.player[1], z); //mik‰ on result???
			//printf("\n oppShotEffects[%d] = %d", i, *(oppShotEffects + i));

		}

		// TODO
		// Merkitse turva-alue uponneiden laivojen ymp‰rille
		//void setSafetyAreaAroundSunkenShip(){

		// meid‰n peliosumat, lasketaan merelt‰
		game.player[1].nroHits = countHitsFromSea(oppTargetSea, 3);

		// vastustajan kohdemeri, oikeat laivat ja vastustajan ammunnat
		printf("\nVastustajan kohdemeri, vas. oikeat laivat, oik. vastustajan ammunnat");
		showTwoSeaParallel(sea, targetSea);
		// meid‰n ammuntameri
		printf("\nMeid‰n ammunnat vastustajan merelle");
		showSea(oppTargetSea);

		//showShip(&air5);	//deb.
		//showShip(&bat4);
		//showShip(&cru3);
		//showShip(&sub3);
		//showShip(&des2);
		//showShip(&oppAir5);
		//showShip(&oppBat4);
		//showShip(&oppCru3);
		//showShip(&oppSub3);
		//showShip(&oppDes2);

		showGameStatus();

		// Sytytet‰‰n ledit hetkeksi, kun laiva uppoaa
		for (i = 0; i < 5 - game.previousTurn_SunkenShips; i++) {
			z = *(oppShotEffects + i);
			/*
			if (z > 1) { // laiva uppoaa: 2,3,3,4,5
				led1On();
				led2On();
				led3On();
				led4On();
				NutSleep(500);
				led1Off();
				led2Off();
				led3Off();
				led4Off();
				NutSleep(200);
			}
			*/
		}

		// Tarkistus,
		// onko peli loppunut tai haluaako k‰ytt‰j‰ keskeytt‰‰
		if (game.player[0].nroSunkenShips == 5 || game.player[1].nroSunkenShips
				== 5 || game.gameStatus == CANCELLED) { //cancel deb.
			// end of the game
			endGame = 1;
			showGameEndResult();

			// send TRAP message
			// sendGameEndedMessage_TRAP_OID6(string);
			oneValue = 0; // TODO tarkista
			length = 0; // TODO tarkista

			parseOutboundMessage(6, &oneValue, length);

			//obs! kun vastustaja keskeytt‰‰, asetettava: game->gameStatus == CANCELLED

		} else {
			askIfUserWantsToCancel();
			if (game.gameStatus == CANCELLED) {
				endGame = 1;
				showGameEndResult();
				// send TRAP message
				// sendGameEndedMessage_TRAP_OID6(string);
				oneValue = 0; // TODO tarkista
				length = 0; // TODO tarkista
				parseOutboundMessage(6, &oneValue, length);

			// siirryt‰‰n seuraavalle kierrokselle
			} else {
				turn++;
				game.turn = turn;
				// set_request(oid=2, turn = game.turn)
				// ota huomioon, jos vastapuoli jo odottaa vastaavaa kuittausta
				// odotetaan kuittaus tai haluaako vastapuoli keskeytt‰‰
					// jos haluaa lopettaa, lopetetaan
					// jos ok, siiryt‰‰n seuraavalle kierrokselle

				// VASTAANOTA SET-REQUEST, OID = 2, int game.turn

				// Ensin tarkistetaan, onko vastustaja ehtinyt jo kysy‰ meilt‰
				// ovatko laivamme valmiit, jos niin vastataan viestiin
				// Ellei ole, l‰hetet‰‰n kysely ja odotetaan vastausta

				// kun vastapuoli l‰hett‰‰ set_requestin(oid=2, turn > 1)
				// lippu: game.startTurn = 1

				//do{

				if (game.startTurn == 1) { // opponent wants to next turn
					// LƒHETƒ Ok-VASTAUS
					//sendBoatsReady_SR_OID2();

					//TODO: Mauni siirt‰‰ seuraavat rivit verkon puolelle
					//TODO: , muuten systeemi ei ole en‰‰ synkassa, kun SNMP-viestej‰ tulee paljon...
					//siirretty verkon puolelle seur. rivit:
					//oneValue = 0;
					parseSnmpResponse(2, &oneValue);
					printf("\nVastataan vastustajan l‰hett‰m‰‰‰n uuden kierroksen Set_requestiin");
					//game.startTurn = 0;	//oletusarvo seuraavalle kierrokselle

				} else {
					// send your announcement
					// LƒHETƒ SET-REQUEST, OID = 2, int turn
					//sendBoatsReady_SR_OID2();  //obs!onko sama vastausksena kuin kysymyksen‰?
					oneValue = game.turn;
					length = 1;
					parseOutboundMessage(2, &oneValue, length);
					printf("\nL‰hetet‰‰n vastustajalle uuden kierroksen merkiksi set_request()");

					// WAIT FOR RESPONSE
					while (game.startTurn == 0) {
						printf("\nOdotetaan whilessa, ett‰ startTurn nousisi ->1");
						//NutSleep(125);
						NutSleep(50);
					}
					//game.startTurn = 0;	//oletusarvo seuraavalle kierrokselle

				}
				//} while (game.startTurn == 0);

				game.startTurn = 0;	//oletusarvo seuraavalle kierrokselle

			}
		}

	} //while
	game.player[1].playerStatus = FREE;
}

/*! \fn void resetWholeGame(void)
 *  \brief Funktio, joka alustaa pelin.
 *  \return void.
 */
void resetWholeGame(void) {    //struct GAME * game){
   u_short i = 0, j = 0;
   for( j = 0; j < 10; j++){
      temp[j] = 0;
      ourCoord[j] = 0;			// our coordinates, sent to opponent sea
      oppCoord[j] = 0;		// coordinates send by opponent
      for(i = 0; i < 10; i++){
         sea[j][i] = 0;
         oppSea[j][i] = 0;
         targetSea[j][i] = 0;
         oppTargetSea[j][i] = 0;
      }
      for(i = 0; i < 5; i++){
         targetSeaShotX[i] = 0;    // our sea
         targetSeaShotY[i] = 0;
         targetOppSeaShotX[i] = 0;    // opponent's sea, two player game()
         targetOppSeaShotY[i] = 0;
         targetOppShotX[i] = 0;    // opponent's game, OnePlayerGame()
         targetOppShotY[i] = 0;
         //oppSeaShotEffects[i] = 0; //effects we caused to opponent's sea = results
         seaShotEffects[i] = 0;    // result how opponent's shots affected, will be returned to opp
         oppShotEffects[i] = 0;
      }
   }

   // vastustajien listan nollaaminen
   for(i = 0; i < 20; i++){
      vastustajaLista[i] = 0;
      u_shortLista[i] = 0;	//
   }

	   // Alkuasetukset
	   game.gameStatus = 0;
	   game.turn = 1;
	   game.playerAnnounced = 0;       // 0 = no, 1 = player replied to our GET_REQ_OID1()
	   game.playerAddress = 0;	//??
	   game.selectedOpponentHasAnswered = 0; // 0 = no, 1 = yes
	   game.opponentAnswer = 0;        // 0 = error, 1 = ok
	   game.coordGetOk = 0;        // 0 = not ready, 1 = message received
	   game.coordSend = 0;       	// 0 = not ready, 1 = message received
	   game.resultsSend = 0;       // 0 = not ready, 1 = results[5] send
	   game.resultsAsked = 0;      // 0 = not asked, 1 = results asked
	   game.resultReady = 0;
	   game.startTurn = 0;
	   // ledin sytytt‰mist‰ varten, otetaan muistiin ed.kierroksen ammusten=resulttien lkm
	   game.previousTurn_SunkenShips = 0;
	   game.speechMode = 0;

	   game.player[0].playerCode = 1;  		// 1 = player1, 2 = opponent
	   game.player[0].shipsSetStatus = 0;    // 0 = not ready, 1 = ready
	   game.player[0].allShipsSunken = 0;
	   game.player[0].nroHits = 0;
	   game.player[0].nroSunkenShips = 0;	// how many ships have been sunken
	   game.player[0].playerStatus = FREE;	 // 0 = free, 1 = reserved
	   game.player[0].sunkenShips[0] = 0;
	   game.player[0].sunkenShips[1] = 0;
	   game.player[0].sunkenShips[2] = 0;
	   game.player[0].sunkenShips[3] = 0;
	   game.player[0].sunkenShips[4] = 0;
	   game.player[1].playerCode = 2;  		// 1 = player1, 2 = opponent
	   game.player[1].shipsSetStatus = 0;    // 0 = not ready, 1 = ready
	   game.player[1].allShipsSunken = 0;
	   game.player[1].nroHits = 0;
	   game.player[1].nroSunkenShips = 0;
	   game.player[1].playerStatus = FREE;
	   game.player[1].sunkenShips[0] = 0;
	   game.player[1].sunkenShips[1] = 0;
	   game.player[1].sunkenShips[2] = 0;
	   game.player[1].sunkenShips[3] = 0;
	   game.player[1].sunkenShips[4] = 0;

	   resetShip(&air5, 5, 6);
	   resetShip(&bat4, 4, 5);
	   resetShip(&cru3, 3, 4);
	   resetShip(&sub3, 3, 3);
	   resetShip(&des2, 2, 2);
	   resetShip(&oppAir5, 5, 6);
	   resetShip(&oppBat4, 4, 5);
	   resetShip(&oppCru3, 3, 4);
	   resetShip(&oppSub3, 3, 3);
	   resetShip(&oppDes2, 2, 2);

	   // vanha player->nroHits, k‰ytet‰‰n verkkoliikenteen debuggaukseen
	   //old_nroHits = 0;


	   /************************************************************************/
	   /*Mauni: seuraava puuttui alustuksista ja aiheutti ylim‰‰r‰isen viestin */
	   // rivill‰ ~200 (parseSnmpResponse() )
	   game.player[0].shipsSetStatus = 0;
	   /************************************************************************/
}

/*! \fn void resetShip(struct SHIP * boat, u_short size, u_short mark)
 *  \brief Funktio, joka alustaa laivat.
 *  \param *boat Laivan nimi.
 *  \param size Laivan koko.
 *  \param mark Pelikartan koodi laivalle.
 *  \return void.
 */
void resetShip(struct SHIP * boat, u_short size, u_short mark){
    boat->size = size;
	boat->shipStatus = 0;        //0=no hit, 1 = hit to one or more coordinates, not sunk, 2 = sunk
	boat->coordStatus[0] = 0;    //0=no hit, 1 = hit
	boat->coordStatus[1] = 0;
	boat->coordStatus[2] = 0;
	boat->coordStatus[3] = 0;
	boat->coordStatus[4] = 0;
	boat->coordinates[0] = 0;   //coordinates(x,y), x first, y next
	boat->coordinates[1] = 0;
	boat->coordinates[2] = 0;
	boat->coordinates[3] = 0;
	boat->coordinates[4] = 0;
	boat->coordinates[5] = 0;
	boat->coordinates[6] = 0;
	boat->coordinates[7] = 0;
	boat->coordinates[8] = 0;
	boat->coordinates[9] = 0;
	boat->nroCoordReady = 0;     //how many coordinatepairs are ready
	boat->mark = mark;               //mark of a ship
}

/*! \fn void startOnePlayerGame(void)
 *  \brief Yksinpelifunktio.
 *  \return void.
 */
void startOnePlayerGame(void) { //struct GAME * game){
   u_short i = 0;
   u_short turn = 0;
   u_short endGame = 0;
   u_short choice = 0;

   game.player[1].playerStatus = RESERVED;

   //printf("\t\tNutHeapAvailable() = %d", NutHeapAvailable());


   // laivojen asettaminen, player1 and player2
   // Own boats
   findLocationToAllShips(&air5, &bat4, &cru3, &sub3, &des2, sea, temp);
   // Opponent's boats on opponent's sea
   findLocationToAllShips(&oppAir5, &oppBat4, &oppCru3, &oppSub3, &oppDes2, oppSea, temp);

   printf("\n\tKuinka haluat pelata?" \
          "\n\t1 = tietokone pelaa puolestani arpoen satunnaisesti ammusten sijainnit" \
          "\n\t2 = annan itse kohdekoordinaatit \n\n\tAnna valintasi: ");
   choice = (u_short)getc(stdin);
   //printf("\nvalinta = d=%d, c=%c", choice, choice);

   while (!endGame){

	  system("cls");
	  fflush(stdin);


	  turn++;
      game.turn = turn;

      showTwoSeaParallel(sea, oppSea);

      // eka kierros
      // kohdekoordinaattien asettaminen (player1 automaattinen, player2 k‰sin)

      // Our sea, opponent's target
      giveMaxFiveShots(5-game.player[0].nroSunkenShips, targetSeaShotX, targetSeaShotY, targetSea);
      /*
      for(i = 0; i < 5; i++){
         printf("\n%d, %d", targetSeaShotX[i]+1, targetSeaShotY[i]+1);        //-1 korjaus (0,9)->(1,10)
      }*/
      //printf("\n");  //deb.

      // Opponent's sea, our target
      if (choice == '1'){
         // random shots
         giveMaxFiveShots(5-game.player[1].nroSunkenShips, targetOppShotX, targetOppShotY, oppTargetSea);
      } else {    //works with all other values than 1
         // shots will be asked from the user
         askMaxFiveShots(5-game.player[1].nroSunkenShips, targetOppShotX, targetOppShotY);
      }
      //for(i = 0; i < 5; i++){
      //   printf("\t%d, %d", targetOppShotX[i], targetOppShotY[i]); }

      // Target coordinates
      // sending target coordinates targetSeaShotX[5], targetSeaShotY[5]  //???
      // receiving opponent target coordinates: targetOppShotX[5], targetOppShotY[5]

      // Hit results
      // asking the hit results from opponent
      // announcing opponent the hit results when asked

      // Analysing shot effects
      // kohde- ja osumakoordinaattien analysou_shorti (player1 ja player2, meri1 ja meri2)
      // Our sea, opponent't target
      for (i = 0; i < 5-game.player[0].nroSunkenShips; i++){
          seaShotEffects[i] = checkShotEffects(&air5, &bat4, &cru3, &sub3, &des2, targetSeaShotX[i], targetSeaShotY[i], sea, targetSea, &game.player[0]);
          setShotsToTargetSea(sea, targetSea, *(targetSeaShotX+i), *(targetSeaShotY+i));
          //printf("\n seaShotEffects[%d] = %d", i, seaShotEffects[i]);
      }
      // Opponent's sea, our target
      for (i = 0; i < 5-game.player[1].nroSunkenShips; i++){
          oppShotEffects[i] = checkShotEffects(&oppAir5, &oppBat4, &oppCru3, &oppSub3, &oppDes2, targetOppShotX[i], targetOppShotY[i], oppSea, oppTargetSea, &game.player[1]);
          setShotsToTargetSea(oppSea, oppTargetSea, *(targetOppShotX+i), *(targetOppShotY+i));
          //printf("\n oppShotEffects[%d] = %d", i, oppShotEffects[i]);
      }

      showTwoSeaParallel(targetSea, oppTargetSea);

      /*// show ships   //deb.
      showShip(&air5);
      showShip(&bat4);
      showShip(&cru3);
      showShip(&sub3);
      showShip(&des2);
      showShip(&oppAir5);
      showShip(&oppBat4);
      showShip(&oppCru3);
      showShip(&oppSub3);
      showShip(&oppDes2);
	  */

      showGameStatus();

	  // Sytytet‰‰n ledit hetkeksi, kun laiva uppoaa
	  for (i = 0; i < 5 - game.previousTurn_SunkenShips; i++) {
		  if (*(oppShotEffects + i) > 1) { // laiva uppoaa: 2,3,3,4,5
				led1On();
				led2On();
				led3On();
				led4On();
				NutSleep(500);
				led1Off();
				led2Off();
				led3Off();
				led4Off();
				NutSleep(200);
			}
	  }

      fflush(stdin);

      // Tarkistus,
      // onko peli loppunut tai haluaako k‰ytt‰j‰ keskeytt‰‰
      if (game.player[0].nroSunkenShips == 5 || game.player[1].nroSunkenShips == 5 || game.gameStatus == CANCELLED){       //cancel deb.
         // end of the game
         endGame = 1;
         showGameEndResult(); //game);
         fflush(stdin);
      } else {
         askIfUserWantsToCancel();   //game);
         if(game.gameStatus == CANCELLED){
            endGame = 1;
            showGameEndResult();   //game);
         }
         fflush(stdin);
      }

   } //while

}
/*
// ei t‰m‰ taida olla k‰ytˆss‰ miss‰‰n?
u_short askPlayToBeStarted(void){
   u_short x;
   printf("\nP‰‰valikko\n\n1 = yksinpeli\n2 = kaksinpeli");
   x = (u_short)getc(stdin);
   fflush(stdin);
   return x;
}
*/

// asks user if user wants to cancel the play
// if cancelled, will be marked to game.gameStatus = CANCELLED
/*! \fn void askIfUserWantsToCancel(void)
 *  \brief Funktio, joka tarkistaa, haluaako k‰ytt‰j‰ keskeytt‰‰ pelin.
 *  \return void.
 */
void askIfUserWantsToCancel(void) {    //struct GAME * game){
   u_short x;
   printf("\nJos haluat keskeytt‰‰, anna 1 ja paina Enter. Jatkaaksesi anna muu numero. ");
   x = (u_short)getc(stdin);

   if (x == '1'){
      game.gameStatus = CANCELLED;
   }
   fflush(stdin);
}

/*! \fn u_short returnAllShipHits(struct SHIP * boat1, struct SHIP * boat2, struct SHIP * boat3, struct SHIP * boat4, struct SHIP * boat5)
 *  \brief Funktio, joka laskee kaikkien alusten osumat. K‰ytet‰‰n yksinpeliss‰.
 *  \param *boat1 Laivan nimi
 *  \param *boat2 Laivan nimi
 *  \param *boat3 Laivan nimi
 *  \param *boat4 Laivan nimi
 *  \param *boat5 Laivan nimi
 *  \return Osumien lukum‰‰r‰.
 */
u_short returnAllShipHits(struct SHIP * boat1, struct SHIP * boat2, struct SHIP * boat3, struct SHIP * boat4, struct SHIP * boat5){
    u_short hits = 0;
    u_short boatHit = 0;

    boatHit = returnShipHitCount(boat1);
    hits = hits + boatHit;
    boatHit = returnShipHitCount(boat2);
    hits = hits + boatHit;
    boatHit = returnShipHitCount(boat3);
    hits = hits + boatHit;
    boatHit = returnShipHitCount(boat4);
    hits = hits + boatHit;
    boatHit = returnShipHitCount(boat5);
    hits = hits + boatHit;

    return hits;
}

/*! \fn u_short returnShipHitCount(struct SHIP * boat)
 *  \brief Funktio, joka palauttaa yhteen laivaan osuneiden osumien lukum‰‰r‰n.
 *  \param *boat Laivan nimi.
  *  \return Osumien lukum‰‰r‰.
 */
u_short returnShipHitCount(struct SHIP * boat){
    u_short i = 0;
    u_short hits = 0;
    for (i = 0; i < boat->size; i++){
        if (boat->coordStatus[i] == 1){
           hits++;
        }
    }
    return hits;
}

/*! \fn void showGameEndResult(void)
 *  \brief Funktio, joka tulostaa pelin lopputuloksen.
  *  \return void.
 */
void showGameEndResult(void) { //struct GAME * game){
   // TODO: should players be identified with player.playerCode or name!!!

   printf("\n\n\tPELI LOPPUI\n\nTulokset: ");
   if (game.player[0].allShipsSunken == 1){         // player[0] = opponent, player[1] = we
      if(game.player[1].allShipsSunken == 1){
          printf("\tTASAPELI");
      } else {
         printf("\tVASTUSTAJA VOITTI!");
      }
   }
   if ((game.player[1].allShipsSunken == 1) && (game.player[0].allShipsSunken != 1)){
      printf("\tME VOITIMME!");
      //Sytytet‰‰n ledit kahteen kertaan
      led1On();
      led2On();
      led3On();
      led4On();
      NutSleep(500);
      led1Off();
      led2Off();
      led3Off();
      led4Off();
      NutSleep(200);
      led1On();
      led2On();
      led3On();
      led4On();
      NutSleep(500);
      led1Off();
      led2Off();
      led3Off();
      led4Off();
      NutSleep(200);
   }

   if ((game.player[0].allShipsSunken != 1) && (game.player[1].allShipsSunken != 1 && game.gameStatus == CANCELLED)){
      printf("\tPeli keskeytettiin.");
   }

   printf("\n\n\tMe saimme %d osumaa ja upotimme %d laivaa.", game.player[1].nroHits, game.player[1].nroSunkenShips);
   printf("\n\tVastustaja sai %d osumaa ja upotti %d laivaa.\n\n", game.player[0].nroHits, game.player[0].nroSunkenShips);

   fflush(stdin);

}

// used to mark shotResults got from opponent to our targetSea
// may be confusing because also sunken (2,3,...) will be marked with +2
/*! \fn void setShotsToOurTargetSea(u_short * sea, u_short x, u_short y, u_short shotResult)
 *  \brief Funktio, joka asettaa osumat meid‰n kohdemerelle.
 *  \param *sea Meri, johon osumat asetetaan
 *  \param x Vaakakoordinaatti.
 *  \param y Pystykoordinaatti.
 *  \param shotResult Ammunnan tulos x ja y koordinaatissa.
  *  \return void.
 */
void setShotsToOurTargetSea(u_short * sea, u_short x, u_short y, u_short shotResult){
   u_short error = 0;

   // checking x values
      if (x < 0 || x > 9){
         error = 1;
      }
      // checking y values
      if (y < 0 || y > 9){
         error = 1;
      }

	  // shots will be marked to target sea
      if (!error){
		  *(sea + x + 10*y) = shotResult + 2;   //deb +2
      }
      fflush(stdin);
}

// used only in the 1-player play
// shot effects will be set to target sea
// 0 = shot not hit,
//set shots to targetSea x = empty shot, 1 = hit // NOT THESE:(, 2 ... 5 = sunken ship)
/*! \fn void setShotsToTargetSea(u_short * sea, u_short * targetSea, u_short targetShotX, u_short targetShotY)
 *  \brief Funktio, joka asettaa ammusten vaikutuksen kohdemerelle.
 *  		K‰ytet‰‰n vain yhden pelaajan peliss‰.
 *  \param *sea Meri, johon laivat on alunperin sijoitettu.
 *  \param *targetSea Meri, johon osumat merkit‰‰n.
 *  \param targetShotX Vaakakoordinaatti.
 *  \param targetShotY Pystykoordinaatti.
 *  \return void.
 */
void setShotsToTargetSea(u_short * sea, u_short * targetSea, u_short targetShotX, u_short targetShotY){
   u_short value = 0;
   u_short error = 0;

      // coordinates (0,9) are supposed to be used (instead of (1,10)

      // checking x values
      if (targetShotX < 0 || targetShotX > 9){
         value = 0;
         error = 1;
      }
      // checking y values
      if (targetShotY < 0 || targetShotY > 9){
         value = 0;
         error = 1;
      }

      if (!error){
         value = returnSeaValue( targetShotX, targetShotY, sea);
         if (value == 0 || value == 1){        //no hit
            value = 0;
         } else {   //hit
           value = 1;
           // if sunken, another value will be returned, or will it??? // NO
         }
      }
      //printf("\n at (%d, %d) value marked to target sea = %d", targetShotX+1, targetShotY+1, value);  //deb +1

      // shots will be marked to target sea
      *(targetSea + targetShotX + 10*targetShotY) = value+2;   //deb +2

      fflush(stdin);
}

/*! \fn void showGameStatus(void)
 *  \brief Funktio, joka tulostaa pelitilanteen.
 *  \return void.
 */
void showGameStatus(void) { //struct GAME * game){
	if(verbose==1){
		printf("\n\n\GAME \tturn = %d, gameStatus = %d", game.turn, game.gameStatus);
		printf("\nplayerAnnounced = %d, playerAddress = %s", game.playerAnnounced, inet_ntoa(game.playerAddress));
		printf("\nselectedOpponentHasAnswered = %d, opponentAnswer = %d", game.selectedOpponentHasAnswered, game.opponentAnswer);
	} else {
		printf("\n\n\tGAME \tturn = %d", game.gameStatus);
	}
   //printf("\nverbose mode = %d", game.verboseMode);
   showPlayer(&game.player[0]);
   showPlayer(&game.player[1]);
   //printf("\nVerkon kautta luetut player[1].old_nroHits = %d", old_nroHits);  //deb
   printf("\n");
   fflush(stdin);
}


/*! \fn void showPlayer(struct PLAYER * player)
 *  \brief Funktio, joka tulostaa pelaajan tietueen tiedot.
 *  \param *player Pelaaja.
  *  \return void.
 */
void showPlayer(struct PLAYER * player){
	if(player->playerCode == 1){
		//printf("\nPLAYER\tplayerCode = %d", player->playerCode);        // 1 = player1, 2 = opponent
		printf("\n\tVASTUSTAJA");        // 1 = player1, 2 = opponent
	} else {
		printf("\n\tME");
	}
	if(verbose == 1){
		printf("\nnroSunkenShips = %d, sunkenShips[] = {%d, %d, %d, %d, %d}",  \
			player->nroSunkenShips, player->sunkenShips[0], player->sunkenShips[1], player->sunkenShips[2], \
			player->sunkenShips[3], player->sunkenShips[4]);
		printf("\n\nroHits = %d, allShipsSunken = %d", player->nroHits, player->allShipsSunken);
	} else {
		printf("\n\tosumien lkm = %d, ", player->nroHits);
		printf("uponneiden laivojen lkm = %d \n\tuponneetLaivat[] = {%d, %d, %d, %d, %d}",  \
			player->nroSunkenShips, player->sunkenShips[0], player->sunkenShips[1], player->sunkenShips[2], \
			player->sunkenShips[3], player->sunkenShips[4]);
	}
   fflush(stdin);
}

// in calling function u_short shotX[5] and u_short shotY[5]
/*! \fn void giveMaxFiveShots(u_short shotCount, u_short * shotX, u_short * shotY, u_short * sea)
 *  \brief Funktio, joka palauttaa maksimissaan viisi koordinaattiparia. Funktio tarkistaa,
 *  		ettei samaa koordinaattia palauteta useaan kertaan. Funktio tarkistaa, ettei
 *  		koordinaattiin ole jo ammuttu aikaisemmin.
 *  \param shotCount Ammusten lukum‰‰r‰.
 *  \param *shotX Taulukko vaakakoordinaateille.
 *  \param *shotY Taulukko pystykoordinaateille.
 *  \param *sea Meri, johon osumat on merkitty.
 *  \return void.
 */
void giveMaxFiveShots(u_short shotCount, u_short * shotX, u_short * shotY, u_short * sea){
   u_short i = 0, j = 0;
   u_short value = 0;
   u_short ok = 0;
   u_short x = 0, y = 0;
   u_short sama = 0;
   u_short virhe = 0;

   // me ammumme vastustajan merelle: oppTargetSea
   //targetOppShotX[5] ja ...Y[5], ourCoord[5], oppShotEffects[5] = results

   // if returned sunken ship, find borders = 1 -> no shots at the area
/*   for(i = 0; i < shotCount; i++){
	   if(oppShotEffects[5]){

	   }
   }
*/

   //if returned hit, not sunk
   // one shot next to hit in random direction



   // make a guess of five ship locations, shots to 2nd or 3rd coordinate?

   do {
	   virhe = 0;
		// five random shots
		for (i = 0; i < shotCount; i++) {
			ok = 0;
			do {
				x = rand() % 10;
				y = rand() % 10;
				//printf("\narvotut (x,y) = (%d,%d)", x, y);  //deb
				// check that shot is not already shot
				value = returnSeaValue(x, y, sea);
				if (value == 0) {
					*(shotX + i) = x;
					*(shotY + i) = y;
					//printf("\thyv‰ksytyt (x,y) = (%d,%d)", x, y);  //deb
					ok = 1;
				}
			} while (ok == 0);
		}


		//Tarkistetaan, ettei arvota samaa koordinaattia
		for (i = 0; i < shotCount; i++) {
			x = *(shotX + i);
			y = *(shotY + i);
			//printf("\nArvotut %d:(%d, %d)",i, x,y);
			sama = 0;
			for (j = 0; j < shotCount; j++) {
				if (x == *(shotX + j) && y == *(shotY + j)){
					sama++;
					//printf("\nSAMAT: %d:(%d, %d) = %d:(%d, %d)", i, x, y, j, *(shotX + j), *(shotY + j));
				}
				if (sama > 1) { // sama = 1 aina, kun verrataan myˆs itseen = ok
					virhe = 1;
					j = shotCount;	// ulos silmukoista
					i = shotCount;
					//printf("\nVIRHE");
				}
			}
		}

	} while (virhe == 1);

   fflush(stdin);
}

//pit‰isi olla return u_short, jos k‰ytt‰j‰ keskeytt‰‰ eik‰ anna kaikkia arvoja(?)
/*! \fn void askMaxFiveShots(u_short shotCount, u_short * shotX, u_short * shotY)
 *  \brief Funktio, joka kysyy k‰ytt‰j‰lt‰ korkeintaan viisi ammuskoordinaattiparia.
 *  \param shotCount Ammusten lukum‰‰r‰.
 *  \param *shotX Taulukko vaakakoordinaateille.
 *  \param *shotY Taulukko pystykoordinaateille.
 *  \return void.
 */
void askMaxFiveShots(u_short shotCount, u_short * shotX, u_short * shotY){
    u_short i = 0;
    for ( i = 0; i < shotCount; i++){
        askTarget(shotX+i, shotY+i);
    }
    //TODO: check that not same coordinate twice and not hit,sunken or safety area

}

// Function handles all shot effects and calls subroutines needed
// struct ship status information will be handled in subroutines and is up-to-date
// struct PLAYER status will be updated (nroHits, sunkenShips[], nroShipsSunken and allShipsSunken)
// returns value which will be forwarded to opponent
/*! \fn u_short checkShotEffects(struct SHIP * air5, struct SHIP * bat4, struct SHIP * cru3, struct SHIP * sub3, struct SHIP * des2, u_short x, u_short y, u_short * sea, u_short * targetSea, struct PLAYER * player)
 *  \brief Funktio, joka tarkistaa, osuiko ammuskoordinaatti laivaan.
 *  \param *air5 Laivan nimi.
 *  \param *bat4 Laivan nimi.
 *  \param *cru3 Laivan nimi.
 *  \param *sub3 Laivan nimi.
 *  \param *des2 Laivan nimi.
 *  \param x Vaakakoordinaatti
 *  \param y Pystykoordinaatti
 *  \param *sea Meri, johon laivasto on sijoitettu.
 *  \param *targetSea Meri, johon osumatiedot merkit‰‰n.
 *  \param *player Pelaaja.
  *  \return Tulosarvo, joka v‰litet‰‰n vastapuolelle. 0 = ei osumaa, 1 = osuma, 2 = laiva *des2 upposi,
                   3 = laiva *sub3 tai *cru3 upposi, 4 = laiva *bat4 upposi, 5 = laiva *air5 upposi

 */
u_short checkShotEffects(struct SHIP * air5, struct SHIP * bat4, struct SHIP * cru3, struct SHIP * sub3, struct SHIP * des2, u_short x, u_short y, u_short * sea, u_short * targetSea, struct PLAYER * player){
    u_short value = 0;
    u_short error = 0;
    u_short result = 0;
    u_short hit = 0;
    u_short sunken = 0;
    u_short sunkenInd = 0;      //to which index sunken ship will be saved in player->sunkenShips[]
    u_short i = 0;

    // checking x values
    if (x < 0 || x > 9){
       value = 0;
       error = 1;
    }
    // checking y values
    if (y < 0 || y > 9){
       value = 0;
       error = 1;
    }

    // What is situation at that coordinate on the sea
    if(!error){
    	value = returnSeaValue(x, y, sea);
    }

    switch(value){
       case 0: // case 0 or 1 = no hit
       case 1: break;
       case 2: //destroyer hit
               // markShipHit() marks boat.shipStatus = 1 (hit)
               hit = markShipHit(des2, x, y);
               // didShipSink() marks boat.shipStatus = 2 (sunken)
               sunken = didShipSink(des2, x, y);
               sunkenInd = 0;
               result = 2;
               break;
       case 3:
               hit = markShipHit(sub3, x, y);
               sunken = didShipSink(sub3, x, y);
               sunkenInd = 1;
               result = 3;     // obs! submarine and cuiser have same return values 3
               break;
       case 4:
               hit = markShipHit(cru3, x, y);
               sunken = didShipSink(cru3, x, y);
               sunkenInd = 2;
               result = 3;    // obs! submarine and cuiser have same return values 3
               break;
       case 5:
               hit = markShipHit(bat4, x, y);
               sunken = didShipSink(bat4, x, y);
               sunkenInd = 3;
               result = 4;
               break;
       case 6:
               hit = markShipHit(air5, x, y);
               sunken = didShipSink(air5, x, y);
               sunkenInd = 4;
               result = 5;
               break;
       default:
               break;
    }

    // otetaan talteen ammusten lkm ledien sytytt‰mist‰ varten
    game.previousTurn_SunkenShips = player->nroSunkenShips;

    // hit
    if(hit == 1){
       if(sunken == 1){
          // result is as set in cases

          // Update the struct PLAYER information
          // each ship has it's own index in player->sunkenShips
          // therefore re-sunk of a ship should not be possible
          player->sunkenShips[sunkenInd] = result;
          //calculate amount of sunken ships
          player->nroSunkenShips = 0;
          for(i = 0; i < 5; i++){
             if(player->sunkenShips[i] != 0){
                player->nroSunkenShips++;
             }
          }
          // mark all ships sunken in the case
          if (player->nroSunkenShips == 5){
             player->allShipsSunken = 1;
          }

       } else {
          result = 1;      // hit = 1
       }
    // no hit
    } else {
       result = 0;         // no hit = 0
    }

    // calculate total number of hits from boat->coordStatus[boat->size]
    // counting all boats
    player->nroHits = returnAllShipHits(air5, bat4, cru3, sub3, des2);

    return result; // 0 = no hit, 1 = hit, 2 = destroyer sunken,
                   // 3 = submarine or cruiser sunken, 4 = battleship sunken
                   // 5 = aircraft carrien sunken
    fflush(stdin);
}

// used only for twoPlayerGame, rest of function checkShotEffects()
// which will be used for onePlayerGame
/*! \fn void markShotEffectsToPlayerStruct(struct PLAYER * player, u_short result)
 *  \brief Funktio, joka merkitsee osumatulokset pelaaja-tietueen tietoihin.
 *  		K‰ytet‰‰n kahden pelaajan peliss‰.
 *  \param *player Pelaaja.
 *  \param result Osumatulos.
 *  \return void.
 */
void markShotEffectsToPlayerStruct(struct PLAYER * player, u_short result){
    u_short i = 0;

    // k‰ytet‰‰n laskemaan verkon kautta tulleita osumia
    //if(result == 1) old_nroHits++;   //hit

    // varsinaisesti nroHits lasketaan k‰ym‰ll‰ meri l‰pi
    //countHitsFromSea() funkitiolla, jota kutsutaan ylemm‰ll‰ tasolla

    if (result > 1) { // sunken 2,3,4,5
		//old_nroHits++;
		// sijoitetaan uponneet laivat omille paikoilleen, player->sunkenShips[i]
		switch (result) {
		case 2:
			if (player->sunkenShips[0] == 0) { //destroyerin paikka, i = 0
				player->sunkenShips[0] = 2;
			}
			break;
		case 3: // t‰ytet‰‰n ensin i=2, sitten i=3, mahdoton erottaa laivoja toisistaan
			if (player->sunkenShips[1] == 0) { // sub3 ja cru3, i = 1 ja i = 2
				player->sunkenShips[1] = 3;
			} else if (player->sunkenShips[2] == 0) {
				player->sunkenShips[2] = 3;
			}
			break;
		case 4:
			if (player->sunkenShips[3] == 0) { //bat4, i = 3
				player->sunkenShips[3] = 4;
			}
			break;
		case 5:
			if (player->sunkenShips[4] == 0) { //destroyerin paikka, i = 0
				player->sunkenShips[4] = 5;
			}
			break;
		default:
			break;

		}
    }

    //calculate amount of sunken ships
    player->nroSunkenShips = 0;
    for(i = 0; i < 5; i++){
       if(player->sunkenShips[i] != 0){
          player->nroSunkenShips++;
       }
    }
    // mark all ships sunken in the case
    if (player->nroSunkenShips == 5){
       player->allShipsSunken = 1;
    }

}



// Finds location to all five ships on the sea
/*! \fn void findLocationToAllShips(struct SHIP * air5, struct SHIP * bat4, struct SHIP * cru3, struct SHIP * sub3, struct SHIP * des2, u_short * sea, u_short * temp)
 *  \brief Funktio, joka sijoittaa kaikki laivat kohdemerelle.
 *  \param *sea Meri, johon laivat sijoitetaan.
 *  \param *air5 Laivan nimi.
 *  \param *bat4 Laivan nimi.
 *  \param *cru3 Laivan nimi.
 *  \param *sub3 Laivan nimi.
 *  \param *des2 Laivan nimi.
 *  \param *sea Kohdemeri.
 *  \param *temp V‰liaikainen muuttuja, johon laivan sijaintikoordinaatit sijoitetaan.
 *  			Sijoitetaan merelle vain jos riitt‰v‰sti tilaa lˆytyy.
 *  \return void.
 */
void findLocationToAllShips(struct SHIP * air5, struct SHIP * bat4, struct SHIP * cru3, struct SHIP * sub3, struct SHIP * des2, u_short * sea, u_short * temp){
   u_short setOk = 0;
   u_short coordSet = 0;    //the return value of setting the coordinates to ship structure

    // Setting ships to sea
   // If the starting pou_short was impossible, start again with do-while
   do{ setOk = findLocationToShip(air5, sea, temp); } while (setOk == 0);
   if (setOk == 1) coordSet = setShipToSea(air5, temp, sea);
      //sitten ampuminen, ilmoitukset
      //oman target-alueen muodostaminen
   //showShip(air5);  //deb.

   setOk = 0;
   do { setOk = findLocationToShip(bat4, sea, temp); } while (setOk == 0);
   if(setOk == 1) coordSet = setShipToSea(bat4, temp, sea);
   //showShip(bat4);

   setOk = 0;
   do { setOk = findLocationToShip(cru3, sea, temp); } while (setOk == 0);
   if(setOk == 1) coordSet = setShipToSea(cru3, temp, sea);
   //showShip(cru3);

   setOk = 0;
   do { setOk = findLocationToShip(sub3, sea, temp); } while (setOk == 0);
   if(setOk == 1) coordSet = setShipToSea(sub3, temp, sea);
   //showShip(sub3);

   setOk = 0;
   do { setOk = findLocationToShip(des2, sea, temp); } while (setOk == 0);
   if(setOk == 1) coordSet = setShipToSea(des2, temp, sea);
   //showShip(des2);

   fflush(stdin);
}


// checking if the ship was hit
// marking data to boat.coordStatus[] in hit case
// and in boat.shipStatus = 1 (hit) in hit case
// returns 1 = hit, 0 = no hit
/*! \fn u_short markShipHit(struct SHIP * boat, u_short x, u_short y)
 *  \brief Funktio, joka tarkistaa, osuiko ammus laivaan. Merkitsee osuman tietueisiin.
 *  \param *boat Laivan nimi.
 *  \param x Vaakakoordinaatti.
 *  \param y Pystykoordinaatti.
 *  \return Osumatulos: 0 = ei osumaa, 1 = osuma.
 */
u_short markShipHit(struct SHIP * boat, u_short x, u_short y){
   u_short i = 0, j = 0;
   u_short hit = 0;
   // using separate i and j for different size of tables
   // Shot coordinate is marked as hit ( = 1) in hit case
   // The boat.coordinates[] are not in order of magnitude,
   // instead the coordinates[] are in random order
   // boat.coordStatus[] follows this random order
   for (i = 0; i < boat->size; i++){
       if(boat->coordinates[j] == x && boat->coordinates[j+1] == y){
          hit = boat->coordStatus[i] = 1;  // hit = 1  //tarkista!!!
       }
       j = j+2; //tarkista
   }
   // boat.shipStatus is marked hit (hit = 1) in hit case
   if (hit == 1){
      boat->shipStatus = 1;
   }
   fflush(stdin);
   return hit;
}
       //mihin koordinaattiin osui, merku_short‰ ko. statustietoon
       //p‰‰ohjelmaan switch-case sen mukaan mihin laivaaan on osunut
       //onko uponnut-kysely
       //laivan tilan merku_shortafunktio, jos osuman sattuessa kaikki ovat osuneet!
       //rutiini laskemaan, montako laivaa on uponnut, laiva kerralaan k‰yd‰‰n l‰pi ->pelin strukti? omat osumat, toisen osumat?

// Checks if the ship has been sunk
// If sunk, make change to ship.shipStatus => 2
// Returns 1 = sunk, 0 = not sunk
/*! \fn u_short didShipSink(struct SHIP * boat, u_short x, u_short y)
 *  \brief Funktio, joka tarkistaa, onko alus uponnut.
 *  \param *boat Laivan nimi
 *  \param x Vaakakoordinaatti
 *  \param y Pystykoordinaatti
 *  \return Osumatulos: 0 = laiva ei uponnut, 1 = laiva upposi.
 */
u_short didShipSink(struct SHIP * boat, u_short x, u_short y){
   u_short i = 0;
   u_short status = 1;
   // Check if the ship sank
   for (i = 0; i < boat->size; i++){
       // if one of coordStatus[]==0, status will be zero
       // and ship has not been sunk
       // if status = 1, ship has been sunk
       status = boat->coordStatus[i] * status;
   }
   if(status == 1){
      boat->shipStatus = 2;          // 2 = sunk
   }
   fflush(stdin);
   return status;
}

// Returns value from the sea, where the (x,y) coordinates point at
// Return values are: 0 or 1 = no hit, 2 = hit to destroyer, 3 = hit to sub
// 4 = hit to cruiser, 5 = hit to battleship, 6 = hit to aircraft carrier
/*! \fn u_short returnSeaValue(u_short x, u_short y, u_short * sea)
 *  \brief Funktio, joka palauttaa merelt‰ arvon vaaka- ja pystykoordinaatin osoittamalta kohdalta.
 *  \param x Vaakakoordinaatti.
 *  \param y Pystykoordinaatti.
 *  \param *sea Meri, jossa laivat sijaitsevat.
 *  \return Osumatulos: 0 tai 1 = ei osumaa, 2 = osuma h‰vitt‰j‰‰n (destroyer),
 *  		3 = osuma sukellusveneeseen (submarine), 4 = osuma risteilij‰ (cruiser), 5 = hit to battleship,
 *  		6 = osuma lentotukialukseen (aircraft carrier).
 */
u_short returnSeaValue(u_short x, u_short y, u_short * sea){
    u_short value = 0;
    value = *(sea + x + 10*y);

    return value;
}

/*! \fn void askTarget(u_short * inX, u_short * inY)
 *  \brief Funktio, joka kysyy k‰ytt‰j‰lt‰ koordinaattiparia.
 *  \param *inX Vaakakoordinaatti.
 *  \param *inY Pystykoordinaatti.
 *  \return void.
 */
void askTarget(u_short * inX, u_short * inY){
   u_short x = 0;
   u_short y = 0;
   // TODO checking required !! types and numbers (0-9)
   //do{
   printf("\nAnna yksi ammuskoordinaatti, arvot v‰lill‰ 0-9: x y ");
   x = (u_short)getc(stdin);
   printf(" %d", x-48);		// -48: ascii -> numero
   y = (u_short)getc(stdin);
   printf(" %d ", y-48);
   *inX = x - 48;
   *inY = y - 48;
   printf("annoit (%d, %d) ", x-48, y-48);
   //if (*inX<0 || *inX>9 || *inY<0 || *inY>9) {
	 //  printf("\nKoordinaattien arvoalueet ovat v‰lill‰ 0-9.");
   //}
   //} while((*inX>=0 && *inX<=9) && (*inY>=0 && *inY<=9));
   fflush(stdin);
}

/*
void setSafetyAreaAroundSunkenShip(){

// ship is set to sea
 j = 0;
 k = 0;
for(i = 0; i < boat->size; i++){
   // set ship to sea
   x = boat->coordinates[j++];
   y = boat->coordinates[j++];
   *(sea + x + 10*y) = boat->mark;
   // look for all possible safety areas around the coordinate: up, right, down and left
   saf[k++] = x;     // up x
   saf[k++] = y-1;   // up y
   saf[k++] = x+1;   // right x
   saf[k++] = y;     // right y
   saf[k++] = x;     // down x
   saf[k++] = y+1;   // down y
   saf[k++] = x-1;   // left x
   saf[k++] = y;     // left y
}

// mark safety area around the ship
for(i = 0; i < 8*boat->size; i++){
   x = saf[i++];
   y = saf[i];
   // check out which are not legal
   if(*(sea + x + 10*y)==boat->mark || x > 9 || x < 0 || y > 9 || y < 0){
      // do nothing
   } else {
      *(sea + x + 10*y) = 1;        //safety mark = 1
   }
}
}
*/

//sets the ship coordinates to SHIP structure
//from structure they will be set to the sea
/*! \fn u_short setShipToSea(struct SHIP * boat, u_short * temp, u_short * sea)
 *  \brief Funktio, joka sijoittaa laivan sijaintikoordinaatit laivan tietuerakenteeseen.
 *  		Koordinaatit sijoitetaan tietueen kautta merelle.
 *  \param *boat Laivan nimi.
 *  \param *temp Sis‰lt‰‰ koordinaatit.
 *  \param *sea Meri, johon laiva sijoitetaan.
 *  \return Tulos: 0 = ep‰onnistuminen, 1 = tiedot kirjoitettu.
 */
u_short setShipToSea(struct SHIP * boat, u_short * temp, u_short * sea){
      u_short i = 0, j = 0, k = 0;
      u_short x = 0, y = 0;
      u_short saf[40] = {0};
      u_short result = 0;

      // Coordinates from temp[] are set to ship structure
      if(boat->size == boat->nroCoordReady){
         for(i = 0; i < 2*boat->size; i ++){  //columns
               boat->coordinates[i] = *(temp+i);       //ship stucture
         }
         result = 1;
      }

      // ship is set to sea
      j = 0;
      k = 0;
     for(i = 0; i < boat->size; i++){
        // set ship to sea
        x = boat->coordinates[j++];
        y = boat->coordinates[j++];
        *(sea + x + 10*y) = boat->mark;
        // look for all possible safety areas around the coordinate: up, right, down and left
        saf[k++] = x;     // up x
        saf[k++] = y-1;   // up y
        saf[k++] = x+1;   // right x
        saf[k++] = y;     // right y
        saf[k++] = x;     // down x
        saf[k++] = y+1;   // down y
        saf[k++] = x-1;   // left x
        saf[k++] = y;     // left y
     }

     // mark safety area around the ship
     for(i = 0; i < 8*boat->size; i++){
        x = saf[i++];
        y = saf[i];
        // check out which are not legal
        if(*(sea + x + 10*y)==boat->mark || x > 9 || x < 0 || y > 9 || y < 0){
           // do nothing
        } else {
           *(sea + x + 10*y) = 1;        //safety mark = 1
        }
     }

     return result;
}

// Searches location to a ship
// Randomizes the first coordinate until a free coordinate is found
// Searches the second coordinate by calling subroutine findSecondCoordinate()
// Searches also third, fourth and fifth coordinates using subroutine findNextCoordinate()
// depending on the struct SHIP boat->size
// returns value 1 with success and 0 with failure
// new attemp will be restarted from main
/*! \fn u_short findLocationToShip(struct SHIP * boat, u_short * sea, u_short * temp)
 *  \brief Funktio, joka etsii alukselle paikan merelt‰. Ensimm‰inen koordinaatti arvotaan
 *  	satunnaisesti, kunnes vapaa paikka lˆytyy. Toisen ja seuraavien koordinaattien etsimiseen
 *  	kutsutaan omia aliohjelmia.
 *  \param *boat Laivan nimi.
 *  \param *sea Meri, johon laiva sijoitetaan
 *  \param *temp V‰liaikaismuuttuja laivan koordinaateille.
 *  \return Tieto, onnistuiko laivan sijoitus (1) vai ep‰onnistuiko se (0).
 */
u_short findLocationToShip(struct SHIP * boat, u_short * sea, u_short * temp){
    u_short i = 0, x = 0, y = 0, found = 0;

    //first cordinate of a ship
    do{
       x = rand()%10;
       y = rand()%10;
       //printf("\nensimm‰inen koordinatti x = %d, y = %d", x, y);  //deb
       if(*(sea + x + 10*y) == 0){
          //*(temp + i++) = x; //both ways work: *temp and temp[]
          //*(temp + i++) = y;
          temp[i++] = x;     //ship coordinatas, reservation
          temp[i++] = y;
          found = 1; //first coordinates found
       }
       //tempTul(temp);   //deb

    } while(found==0);       //first
    boat->nroCoordReady = 1;

    // Second coordinate
    found = findSecondCoordinate(boat, sea, temp, &i, x, y);

    // Third, fourth and fifth coordinates
    if(boat->size >= 3){
       found = findNextCoordinate(boat, sea, temp, &i, x, y);
       //tempTul(temp);   //deb
    }
    if(boat->size >= 4){
       found = findNextCoordinate(boat, sea, temp, &i, x, y);
       //tempTul(temp);   //deb
    }
    if(boat->size >= 5){
       found = findNextCoordinate(boat, sea, temp, &i, x, y);
       //tempTul(temp);   //deb
    }
    fflush(stdin);
    return found;   //1=succeeded, 0=error
}

// Searches third, fourth and fifth coordinates of a ship
// decision if boat is horizontal or vertical
// random selection for which end to continue the ship
// if the first direction will not succeed, the other one is tried directly
/*! \fn u_short findNextCoordinate(struct SHIP * boat, u_short * sea, u_short * temp, u_short * i, u_short x, u_short y)
 *  \brief Funktio, joka etsii laivalle kolmannen, nelj‰nnen ja/tai viidennen koordinaatin.
 *  		Etsii satunnaisesti toiseen p‰‰tyyn uutta vapaata koordinaattia.
 *  \param *boat Laivan nimi.
 *  \param *sea Meri, johon laivaa ollaan sijoittamassa.
 *  \param *temp V‰liaikaismuuttuja laivan koordinaatteja varten.
 *  \param *i Taulukon indeksi.
 *  \param x Vaakakoordinaatti.
 *  \param y Pystykoordinaatti.
 *  \return Koordinaatin lˆytyminen: 0 = ei lˆytynyt, 1 = koordinaatti lˆytyi.
 */
u_short findNextCoordinate(struct SHIP * boat, u_short * sea, u_short * temp, u_short * i, u_short x, u_short y){
    u_short z = 0;
    u_short found = 0;
    u_short downY = 0, upY = 0, rightX = 0, leftX = 0;

       //Horizontal location of a ship
       if(temp[0]==temp[2]){       // horizontal
          z = rand()%2;
          //find the lowest and uppest y coordinates
          downY = findDownY(boat->nroCoordReady, temp);
          upY = findUpY(boat->nroCoordReady, temp);
          //printf("\n\t\tdownY = %d, upY = %d", downY, upY);  //deb

          if(z==0){
             found = seekDown(sea, i, temp[0], downY, temp);      //x = temp[0] = same  //&i -> i
             if(found==0){  //check the other direction
                found = seekUp(sea, i, temp[0], upY, temp);
             }
          } else {
             found = seekUp(sea, i, temp[0], upY, temp);
             if(found==0){  //check the other direction
                found = seekDown(sea, i, temp[0], downY, temp);
             }
          }

       // Vertical location
       } else {                   // vertical
          z = rand()%2;
          //find the rightmost and leftmost x coordinates
          rightX = findRightX(boat->nroCoordReady, temp);
          leftX = findLeftX(boat->nroCoordReady, temp);
          //printf("\n\t\trightX = %d, leftX = %d", rightX, leftX);  //deb

          //first to the right, if not succes, then left
          if(z==0){
             found = seekRight(sea, i, rightX, temp[1], temp);  //y = temp[1] = same
             if(found==0){  //check the other direction
                found = seekLeft(sea, i, leftX, temp[1], temp);
             }
          //fist to the left, if not succes, then right
          } else {
             found = seekLeft(sea, i, leftX, temp[1], temp);
             if(found==0){  //check the other direction
                found = seekRight(sea, i, rightX, temp[1], temp);
             }
          }
       }

       if (found == 0){
          //printf("\nEPƒONNISTUNUT ETSINTƒ");
       } else {
           boat->nroCoordReady++;
       }
       fflush(stdin);
       return found;
}

// Searches the second coordinate of a ship
// randomizes the direction (up, right, down, left)
/*! \fn u_short findSecondCoordinate(struct SHIP * boat, u_short * sea, u_short * temp, u_short * i, u_short x, u_short y)
 *  \brief Funktio, joka etsii laivalle toisen koordinaatin.
 *  \param *boat Laivan nimi.
 *  \param *sea Meri, johon laivaa ollaan sijoittamassa.
 *  \param *temp V‰liaikaismuuttuja laivan koordinaatteja varten.
 *  \param *i Taulukon indeksi.
 *  \param x Vaakakoordinaatti.
 *  \param y Pystykoordinaatti.
 *  \return Koordinaatin lˆytyminen: 0 = ei lˆytynyt, 1 = koordinaatti lˆytyi.
 */
u_short findSecondCoordinate(struct SHIP * boat, u_short * sea, u_short * temp, u_short * i, u_short x, u_short y){
    u_short found = 0;
    u_short z = 0;
    //next direction, second coordinates
    do{
       z = rand()%4;    //selecting direction
       switch(z){
       case(0): //right
                found = seekRight(sea, i, x, y, temp);  //&i -> i
                break;
       case(1): //down
                found = seekDown(sea, i, x, y, temp);
                break;
       case(2): //left
                found = seekLeft(sea, i, x, y, temp);
                break;
       case(3): //up
                found = seekUp(sea, i, x, y, temp);
                break;
       default: break;
       }
       //tempTul(temp);   //deb
    } while(found == 0);  //second

    if (found == 1){
       boat->nroCoordReady = 2;
    } else {
       //printf("\nEPƒONNISTUNUT SIJOITUS");
    }
    fflush(stdin);
    return found;
}

// Printing the temp[] table, where the unfinished ship coordinates are located
/*! \fn void tempTul(u_short * temp)
 *  \brief Funktio, joka tulostaa laivan sijaintikoordinaatteja sis‰lt‰v‰n v‰liaikaismuuttujan.
 *  		K‰ytˆss‰ vain debuggausvaiheessa.
 *  \param *temp V‰liaikaismuuttuja laivan koordinaatteja varten.
 *  \return void.
 */
void tempTul(u_short * temp){
   u_short i = 0;
   printf("\n\nTempin sis‰ltˆ:");
   for (i = 0; i < 10; i++){
       if (i%2 == 0) printf("\n%d. ",i);
       printf("%d ", *(temp+i));
   }
   fflush(stdin);
}

//return the smallest Y value (highest at the top)
/*! \fn u_short findUpY(u_short size, u_short * temp)
 *  \brief Funktio, joka palauttaa pienimm‰n y-koordinaatin arvon.
 *  \param size Laivan koko.
 *  \param *temp V‰liaikaismuuttuja laivan koordinaatteja varten.
 *  \return Pienin y-koordinaatin arvo.
 */
u_short findUpY(u_short size, u_short * temp){
   //printf("\nfindUpY()");
   u_short foo = *(temp+1);    //first x value
   //boat size = 3
   if (foo > *(temp+3)){       //first x value vs. second y value
      foo = *(temp+3);
   }
   //boat size = 4 or 5
   if (size >= 3){
      if (foo > *(temp+5)){       //max(1st,2nd) vs. 3rd y
         foo = *(temp+5);
      }
   }
   //boat size = 5
   if (size >= 4){
      if (foo > *(temp+7)){       //max(1st,2nd,3rd) vs. 4th y
         foo = *(temp+7);
      }
   }
   return foo;
}


//return the biggest Y value (lowest at the bottom)
/*! \fn u_short findDownY(u_short size, u_short * temp)
 *  \brief Funktio, joka palauttaa suurimman y-koordinaatin arvon.
 *  \param size Laivan koko.
 *  \param *temp V‰liaikaismuuttuja laivan koordinaatteja varten.
 *  \return Suurin y-koordinaatin arvo.
 */
u_short findDownY(u_short size, u_short * temp){
   //printf("\nfindDownY()");
   u_short foo = *(temp+1);    //first x value
   //boat size = 3
   if (foo < *(temp+3)){       //first x value vs. second y value
      foo = *(temp+3);
   }
   //boat size = 4 or 5
   if (size >= 3){
      if (foo < *(temp+5)){       //max(1st,2nd) vs. 3rd y
         foo = *(temp+5);
      }
   }
   //boat size = 5
   if (size >= 4){
      if (foo < *(temp+7)){       //max(1st,2nd,3rd) vs. 4th y
         foo = *(temp+7);
      }
   }
   return foo;
}


//returns the leftmost x value from x coordinates defined by size
/*! \fn u_short findLeftX(u_short size, u_short * temp)
 *  \brief Funktio, joka palauttaa pienimm‰n x-koordinaatin arvon.
 *  \param size Laivan koko.
 *  \param *temp V‰liaikaismuuttuja laivan koordinaatteja varten.
 *  \return Pienin x-koordinaatin arvo.
 */
u_short findLeftX(u_short size, u_short * temp){
   //printf("\nfindLeftX()");
   u_short foo = *temp;    //first x value
   //boat size = 3
   if (foo > *(temp+2)){       //first x value vs. second x value
      foo = *(temp+2);
   }
   //boat size = 4 or 5
   if (size >= 3){
      if (foo > *(temp+4)){       //max(1st,2nd) vs. 3rd x
         foo = *(temp+4);
      }
   }
   //boat size = 5
   if (size >= 4){
      if (foo > *(temp+6)){       //max(1st,2nd,3rd) vs. 4th x
         foo = *(temp+6);
      }
   }
   return foo;
}


//returns the rightmost value of x coordinate of a ship
//will be used only after two coordinatepairs are ready
//size is not boat->size but boat->nroCoordReady
/*! \fn u_short findRightX(u_short size, u_short * temp)
 *  \brief Funktio, joka palauttaa suurimman x-koordinaatin arvon.
 *  \param size Laivan koko.
 *  \param *temp V‰liaikaismuuttuja laivan koordinaatteja varten.
 *  \return Suurin x-koordinaatin arvo.
 */
u_short findRightX(u_short size, u_short * temp){
   //printf("\nfindRightX()");
   u_short right = *temp;    //first x value
   //boat size = 3, 4 or 5
   if (right < *(temp+2)){       //first x value vs. second x value
      right = *(temp+2);
   }
   //boat size = 4 or 5
   if (size >= 3){
      if (right < *(temp+4)){       //max(1st,2nd) vs. 3rd x
         right = *(temp+4);
      }
   }
   //boat size = 5
   if (size >= 4){
      if (right < *(temp+6)){       //max(1st,2nd,3rd) vs. 4th x
         right = *(temp+6);
      }
   }
   return right;
}

/*! \fn u_short seekUp(u_short * sea, u_short * i, u_short x, u_short y, u_short * temp)
 *  \brief Funktio, joka tutkii, onko koordinaatti (x, y-1) vapaa.
 *  \param *sea Meri, johon laivaa ollaan sijoittamassa.
 *  \param *i Taulukon indeksi.
 *  \param x Vaakakoordinaatti.
 *  \param y Pystykoordinaatti.
 *  \param *temp V‰liaikaismuuttuja laivan koordinaatteja varten.
 *  \return Koordinaatin lˆytyminen: 0 = ei lˆytynyt, 1 = koordinaatti lˆytyi.
 */
u_short seekUp(u_short * sea, u_short * i, u_short x, u_short y, u_short * temp){
   //printf("\nseekUp()");
   u_short found = 0;
   if(y!=0){
      if(*(sea + x + 10*(y-1)) == 0){
          temp[(*i)++] = x;     //ship coordinatas, reservation
          temp[(*i)++] = y-1;
          found = 1; //second coordinates found
      }
   }
   return found;
}

/*! \fn u_short seekLeft(u_short * sea, u_short * i, u_short x, u_short y, u_short * temp)
 *  \brief Funktio, joka tutkii, onko koordinaatti (x-1, y) vapaa.
 *  \param *sea Meri, johon laivaa ollaan sijoittamassa.
 *  \param *i Taulukon indeksi.
 *  \param x Vaakakoordinaatti.
 *  \param y Pystykoordinaatti.
 *  \param *temp V‰liaikaismuuttuja laivan koordinaatteja varten.
 *  \return Koordinaatin lˆytyminen: 0 = ei lˆytynyt, 1 = koordinaatti lˆytyi.
 */
u_short seekLeft(u_short * sea, u_short * i, u_short x, u_short y, u_short * temp){
   //printf("\nseekLeft()");
   u_short found = 0;
   if(x!=0){
      if(*(sea + x-1 + 10*y) == 0){
          temp[(*i)++] = x-1;     //ship coordinatas, reservation
          temp[(*i)++] = y;
          found = 1; //second coordinates found
      }
   }
   return found;
}

/*! \fn u_short seekDown(u_short * sea, u_short * i, u_short x, u_short y, u_short * temp)
 *  \brief Funktio, joka tutkii, onko koordinaatti (x, y+1) vapaa.
 *  \param *sea Meri, johon laivaa ollaan sijoittamassa.
 *  \param *i Taulukon indeksi.
 *  \param x Vaakakoordinaatti.
 *  \param y Pystykoordinaatti.
 *  \param *temp V‰liaikaismuuttuja laivan koordinaatteja varten.
 *  \return Koordinaatin lˆytyminen: 0 = ei lˆytynyt, 1 = koordinaatti lˆytyi.
 */
u_short seekDown(u_short * sea, u_short * i, u_short x, u_short y, u_short * temp){
   //printf("\nseekDown()");
   u_short found = 0;
   if(y!=9){
      if(*(sea + x + 10*(y+1)) == 0){
         temp[(*i)++] = x;     //ship coordinatas, reservation
         temp[(*i)++] = y+1;
         found = 1; //second coordinates found
      }
   }
   return found;
}

/*! \fn u_short seekRight(u_short * sea, u_short * i, u_short x, u_short y, u_short * temp)
 *  \brief Funktio, joka tutkii, onko koordinaatti (x+1, y) vapaa.
 *  \param *sea Meri, johon laivaa ollaan sijoittamassa.
 *  \param *i Taulukon indeksi.
 *  \param x Vaakakoordinaatti.
 *  \param y Pystykoordinaatti.
 *  \param *temp V‰liaikaismuuttuja laivan koordinaatteja varten.
 *  \return Koordinaatin lˆytyminen: 0 = ei lˆytynyt, 1 = koordinaatti lˆytyi.
 */
u_short seekRight(u_short * sea, u_short * i, u_short x, u_short y, u_short * temp){
   //printf("\nseekRight()");
   u_short found = 0;
   //printf("\t\tx = %d, y = %d", x, y);   //deb
   if(x!=9){   //check of boundaries
      if(*(sea + (x+1) + (10*y)) == 0){
         temp[(*i)++] = x+1;     //ship coordinatas, reservation
         temp[(*i)++] = y;
         found = 1; //second coordinates found
         //printf("\nseekRight() if-lohko");    //deb
      } else {
         //printf("\n\t\t\tseekRight() ELSE-LOHKO");  //deb
      }
   }
   fflush(stdin);
   return found;
}

/*! \fn void showShip(struct SHIP * boat)
 *  \brief Funktio, joka tulostaa tietueen sis‰llˆn.
 *  \param *boat Laivan nimi.
 *  \return void.
 */
void showShip(struct SHIP * boat){
     u_short i = 0;
     printf("\n\nmark = %d, ", boat->mark);
     printf("size = %d, ", boat->size);
     printf("shipStatus = %d, ", boat->shipStatus);
     printf("nroCoordReady = %d", boat->nroCoordReady);
     printf("\ncoordinates are:");
     for (i = 0; i < 2*boat->size; i++){
        if (i%2 == 0){
           printf("\n");
        }
        printf(" %d", boat->coordinates[i]);
     }
     printf("\nStatus per coordinate: ");
        for (i = 0; i < boat->size; i++){
            printf(" %d", boat->coordStatus[i]);
     }
     fflush(stdin);
}

/*! \fn void showSea(u_short * sea)
 *  \brief Funktio, joka tulostaa meren.
 *  \param *sea Tulostettava meri.
 *  \return void.
 */
void showSea(u_short * sea){
   u_short i = 0, j = 0;

   //taulukko: meri[rivi][sarake]
   printf("\n   0 1 2 3 4 5 6 7 8 9\n");
   for(j = 0; j < 10; j++){     // rows
      printf("\n%d ", j);       // pru_shorts numbers vertically
      //printf("\n%c ",65+j);     // pru_shorts letters vertically
      for(i = 0; i < 10; i++){  // columns
         printf(" %d", *(sea+i+10*j) );
      }
   }
   printf("\n\n");
   fflush(stdin);
}

/*! \fn void showTwoSeaParallel(u_short * sea1, u_short * sea2)
 *  \brief Funktio, joka tulostaa kaksi merialuetta vierekk‰in.
 *  \param *sea1 Vasen meri.
 *  \param *sea2 Oikea meri.
 *  \return void.
 */
void showTwoSeaParallel(u_short * sea1, u_short * sea2){
   u_short i = 0, j = 0, k = 0;

   //taulukko: meri[rivi][sarake]
   //printf("\n   Our sea             \t\t   Opponent's sea\n");
   printf("\n\n\t   Meid‰n laivasto     \t\t   Vastustajan laivasto\n");
   printf("\n\t   0 1 2 3 4 5 6 7 8 9\t\t   0 1 2 3 4 5 6 7 8 9");
   printf("\n\t");
   for(j = 0; j < 10; j++){     //rivit
      printf("\n\t%d ",j);      //tulostaa numerot pystyyn
      //printf("\n%c ",65+j);   //tulostaa kirjaimet pystyyn
      for(i = 0; i < 10; i++){  //sarakkeet
    	  if(*(sea1+i+10*j) == 0){
    		  printf(" -");
    	  } else{
    		  printf(" %d", *(sea1+i+10*j) );
    	  }
      }
      printf("\t\t");
      printf("%d ",j);        //tulostaa numerot pystyyn
      //printf("%c ",65+j);     //tulostaa kirjaimet pystyyn
      for(k = 0; k < 10; k++){  //sarakkeet
    	  if(*(sea2+k+10*j) == 0){
    		  printf(" -");
    	  } else {
    		  printf(" %d", *(sea2+k+10*j) );
    	  }
      }
   }
   printf("\n\n");
   fflush(stdin);
}
