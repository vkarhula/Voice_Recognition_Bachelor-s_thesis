//TODO: vaihda vv.portit parseSnmpResponsesta kts. rivi 664

/*
 * verkko.c
 *
 *  Created on: 14.2.2009
 *      Author: Administrator
 */

/*
halt
flash write_bank 0 "c:\workspace\sop\sop.bin" 0
at91sam7 gpnvm 0 2 set
resume
reset
*/

#include <stdio.h>
#include <io.h>

#include <dev/board.h>
#include <sys/timer.h>

#include <arpa/inet.h>
#include <sys/confnet.h>	//Verkon configurointi
#include <sys/socket.h>

#include <sys/thread.h>

//#include <string.h>

#include "verkko.h"
#include "laiva.h"	//
#include "main.h"

#define MYMAC   0x00, 0x06, 0x98, 0x00, 0x00, 0x00
//#define MYIP	"10.10.4.3"
#define MYIP	"10.10.4.30"
#define MYMASK "255.255.0.0"
#define UDP_BCA "10.10.255.255"	//UDP Broadcast osoite

#define LPORT 161

/*
 * Globaalit muuttujat
 */
UDPSOCKET *sock;
u_long addr;

u_short port;
u_char data[200];
u_short  size;
u_char parsittudata[200];

struct viesti vv;    // vastaanotettu viesti
struct viesti lv;    // lähetettävä viesti

struct snmp vast;	//vastaanotettu SNMP-struct
struct snmp lahteva;//lähetettävä SNMP-struct



/********************************************************************/
//20.3.2009: THREADIT MUKAAN MUUTOKSET THREAD():SSÄ JA INITIALIZE():SSA
//MYÖS SNMPSEND()
/********************************************************************/

/********************************************************************/
//21.3.2009: SNMP-testiviestien lähetystä
/********************************************************************/

/********************************************************************/
//27.3.2009: vastustajan osoitteen manuaalinen määritys. Voi käyttää myös mainissa omassa IP:ssä
//Sekvenssien lähetys toimii, mutta toinen EIR ei tulosta niitä (yksi ylimääräinen
//tavu näkyy wiresharkissa??!!)
/********************************************************************/

/********************************************************************/
//6.4.2009: integrointia laivanupotukseen
/********************************************************************/

/* Threadit SNMP-viestien lähettämiseen ja vastaanottamiseen */
THREAD(Vastaanotto, arg)
{
	/************ threadien prioriteetit vedetty hatusta ************************/
	//NutThreadSetPriority(48);
	NutThreadSetPriority(32);
	while (1) {
		printf("Message receiver module ready...\n");
		receiveMessage();
		NutSleep(125);
	}
}
THREAD(Lahetys, arg)//Tämän threadin voi poistaa, kunhan testaus tältä osin valmis
{
	u_short choice;	//testimuuttuja viestien lähetykseen
	NutSleep(200);	//Annetaan vastaanottomoduulin asettua...
	NutThreadSetPriority(32);
	while (1) {
		choice = printLahetys();
		sendSnmpMessageTestaus(choice);
		NutSleep(150);
	}
}

THREAD(Kaksinpeli, arg)
{
	u_short oneValue = 0x00;	// no error
	NutThreadSetPriority(48);	// < default:64
	srand(time(NULL)); //satunnaislukugeneraattorin alustus
	resetWholeGame();
	game.playerAddress = vv.osoite;
	game.player[1].playerStatus = RESERVED;
	showGameStatus();
	parseSnmpResponse(2, &oneValue);
	printf("THREAD: Kaksinpeli: Aloitetaan peli.\n");
	startTwoPlayerGame();
	printf("THREAD: Kaksinpeli: Peli loppui.\n");
	printf("Suljetaan kaksinpeli threadi***************\n");
	NutThreadExit();
}

/*! \fn void initializeNetwork(void)
 *  \brief Funktio, joka suorittaa EIR:n verkkotoiminnallisuuksien alustamisen.
 *  \return void.
 */
void initializeNetwork(void) {

	/*
     * Register Ethernet controller.
     */
    while ( NutRegisterDevice(&DEV_ETHER, 0, 0/*0x8300, 5*/) !=0 )
    	printf("Registering network device failed! ");
    printf("\n\n============= Starting Network ===============\n");

    /*
     * LAN configuration using fixed values.
     */
    u_char mac[] = {MYMAC};
    u_long ip_addr = inet_addr(MYIP);
    u_long ip_mask = inet_addr(MYMASK);

    NutNetIfConfig("eth0", mac, ip_addr, ip_mask);
    printf("%s ready\n", inet_ntoa(confnet.cdn_ip_addr));


	sock = NutUdpCreateSocket(LPORT);

	//Bufferia viestien vastaanottoon
	u_short rxsz = 1024;
	NutUdpSetSockOpt(sock, SO_RCVBUF, &rxsz, sizeof(rxsz));

	printf("Listening on port: %d \n", LPORT);

	NutThreadCreate("T1", Vastaanotto, 0, 2000);
	//NutThreadCreate("T2", Lahetys, 0, 768);

}

/*! \fn void receiveMessage(void)
 *  \brief Funktio, joka hoitaa viestien vastaanoton.
 *  \return void.
 */
void receiveMessage(void) {
	u_short i;
	u_short choice;
	u_short oneValue = 0;

	while (1) {

		/* Talletetaan kanavasta tuleva data */
		vv.daatankoko = NutUdpReceiveFrom(sock, &vv.osoite, &vv.portti,
				vv.daatta, sizeof(vv.daatta), 50);//2000);
		/* Tulostetaan vastaanotettu data */
		if (vv.daatankoko > 0) {
			if (verbose) {
				if(vv.osoite == game.playerAddress){
					printf("\n\n====== Got %d bytes from %s:%d ======\n", vv.daatankoko, inet_ntoa(
						vv.osoite), vv.portti);
				}
			}
			snmpReceive();	//vastaanotettu data SNMP-structiksi
			u_short oid = checkOID();

			/**************************************************/
			//Vastustajan vastaus READY-TO-PLAY kyselyyn
			//
			//		jos oid==1 JA ei erroria (vast.error.error_value==0) JA ei olla varattuja: niin aseta game->playerAnnounced=1
			//		JA game->playerAddress = vv.osoite	TARKISTA toiminta!!!
			//TESTAUSTA VAILLE VALMIS
			if (oid==1 && vast.error.error_value==0 && game.player[1].playerStatus != RESERVED) {
				game.playerAnnounced=1;
				game.playerAddress = vv.osoite;
				if (verbose) {
					printf("\nverkko: game.playerAnnounced=1;");  //
					printf("receiveMessage: game.playerAddress: %s\n",inet_ntoa(game.playerAddress));
					//showGameStatus(&game);		//
				}
			}

			//Vastustajan vastaus START-TURN-kyselyyn
			//		jos oid==2 JA vast.PDU.PDU_type==0xA2: game->selectedOpponentHasAnswered=1
			//			jos vast.error.error_value==0: game->opponentAnswer=1
			//			else game->opponentAnswer = 0
			//TESTAUSTA VAILLE VALMIS
			if (oid==2 && vast.PDU.PDU_type==0xA2) {
				game.selectedOpponentHasAnswered = 1;
				if (vast.error.error_value==0x00)
					game.opponentAnswer = 1;
				else
					game.opponentAnswer = 0;
			}

			//Ollaan varattuja, vastaukset READY-TO-PLAYhin ja START-TURNiin
			//		jos game->player[1].playerStatus==1 (RESERVED) JA (oid==1 TAI oid==2) JA !GET_RESPONSE:
			//			vastaa GET-RESPONSE, value 0,jos oid == 1
			//			TAI GET-RESPONSE, error (5), jos oid==2
			if (verbose)
				printf("game.player[1].playerStatus:%d", game.player[1].playerStatus);
			if ( game.player[1].playerStatus==RESERVED && (oid==1 || oid==2)
					&& vast.PDU.PDU_type!=0xA2 ) {
				//Vastaukset peliä kyseleville
				if (oid == 1) {
					oneValue = 0x00;
					parseSnmpResponse(1, &oneValue);
				}
				else if (oid == 2 && game.playerAddress != vv.osoite ) {
					oneValue = 0x05;
					parseSnmpResponse(2, &oneValue);
				}
				//Vastaukset vastustajalle
				else if (oid==2 && game.playerAddress==vv.osoite && vast.value.value_value[0]>1) {
					//TODO: Kuva3x viesti 10
					if (verbose)
						printf("\nstartTurn = 1, 2. else-if");
					game.startTurn = 1;
					//oneValue = 0x00;
					//parseSnmpResponse(2, &oneValue);	//OK-vastaus pelikaverille
					//TODO: Laiva.c rivi 522
				}
				/*else if (oid==2 && game.playerAddress==vv.osoite && vast.PDU.PDU_type == 0xA2) {
					printf("\nstartTurn = 1, 3. else-if");
					game.startTurn = 1;
				}*/
			}
			if (game.player[1].playerStatus == RESERVED && oid == 2
					&& game.playerAddress == vv.osoite && vast.PDU.PDU_type	== 0xA2) {
				if (verbose)
					printf("\nstartTurn = 1, 3. else-if");
				game.startTurn = 1;
			}

			//Vastaus READY-TO-PLAY kyselyyn, kun ei olla varattuja(oid==1 JA ei GET-RESPONSE):
			//		jos game->player[1].playerStatus==1(RESERVED): GET_RESPONSE, value=0
			//		else : GET_RESPONSE, value=1
			//TESTAUSTA VAILLE VALMIS
			if (oid==1 && vast.PDU.PDU_type!=0xA2 && game.player[1].playerStatus==FREE) {
					NutSleep(1000);
					oneValue = 0x01;
					parseSnmpResponse(1, &oneValue);
			}

			//Vastaus START-TURN kyselyyn, kun ei olla varattuja
			//		ja vast.value==0
			//		Eli vastustaja kysyy meitä pelaamaan.
			//
			if (oid == 2 && vast.PDU.PDU_type==0xA3 && game.player[1].playerStatus==FREE
					&& vast.value.value_value[0]==0) {
				printf("Do you want to play with %s?\n", inet_ntoa(vv.osoite));
				printf("1: Play\nAny other key: Don't play\n");
				choice = (u_short) getc(stdin);
				choice -= 48;
				if (choice == 1) { //Aloitetaan kaksinpeli
					NutThreadCreate("Peli", Kaksinpeli, 0, 768);
					/*
					resetWholeGame();
					game.playerAddress = vv.osoite;
					game.player[1].playerStatus = RESERVED;
					showGameStatus();
					parseSnmpResponse(2, 0x00);
					startTwoPlayerGame();
					*/
				}
				else {
					oneValue = 0x05;
					parseSnmpResponse(2, &oneValue);
				}
			}

			/********** PELI KÄYNNISSÄ ************************/
			//vastustaja asettanut laivansa
			//		vastaanotetaan oid==2 JA SET-REQUEST: aseta game->player[0].shipSetStatus=1
			//		//Ota talteen vastustajan viesti!!!!!!!!!!!!!!!!!!! tarkistellaan nämä vielä
			//		//Vaatii vielä vuoronumeron tarkistuksen??? (vuoro==1)?
//17.4.2009
			if (oid==2 && vast.PDU.PDU_type == 0xA3) { //&& vast.value.value_value[0]==1)//TODO: AND vast.value.value_value[0]==1 ???
				game.player[0].shipsSetStatus=1;
				SR_2 = vast;	//Otetaan vastustajan viesti talteen globaaliin structiin
				//char nimi[] = "SR_2";	suoraan sijoitus toimii
				//printStruct(nimi, &SR_2);
			}

			//omat laivat on asetettu
			//		vastaanotetaan oid==2 ja GET-RESPONSE: aseta game->player[0].shipSetStatus=1 (lähes sama kuin edellä->yhdistä?)
			//		//Ota talteen vastustajan viesti!!!!!!!!!!!!!!!!!!! tarkistellaan nämä vielä
			//		//Vaatii vielä vuoronumeron tarkistuksen??? (vuoro==0)?
			if (oid==2 && vast.PDU.PDU_type == 0xA2)
				game.player[0].shipsSetStatus=1;

			//vastustaja lähettänyt 10 koordinaattia
			//		vastaanotetaan oid==3 ja SET-REQUEST
			//		-kymmenen integeriä globaaliin int oppCoord[10]:iin
			//		-game.coordSend = 1
			//		-vastaa OK
			//TESTAUSTA VAILLE VALMIS
			if (oid==3 && vast.PDU.PDU_type==0xA3) {
				//printf("Asetetaan koordinaatit.\n");
				//fflush(stdin);
				if (verbose)
					printf("\nVastaaotettiin oid==3 ja SET-REQUEST\n****************************");
				for (i=0 ; i<10 ; i++) {
					//printf("receiveMessage: vast.value.value_value[%d]: %d\n", i, vast.value.value_value[i]);
					oppCoord[i] = (u_short) vast.value.value_value[i];
				}
				game.coordSend = 1;
				if (verbose)
					printf("\n\t\tVastustajan koordinaatit otettu vastaan. game.coordSend = 1\n");
				//fflush(stdin);	//klo1230
				//siirretty laivaan 17.4.2009
				//oneValue = 0x00;
				//parseSnmpResponse(3, &oneValue);
			}

			//vastustaja kuitannut omat lähetetyt 10 koordinaattia
			//		vastaanotetaan oid==3 ja GET-RESPONSE
			//		game.coordGetOk=1
			//TESTAUSTA VAILLE VALMIS
			if (oid==3 && vast.PDU.PDU_type==0xA2){
				game.coordGetOk = 1;
				if (verbose) {
					printf("\nVastustaja kuittaa saaneensa 10 koordinaattia");
					printf("\n\t\tgame.coordGetOk asetettu: 1");
				}
			}

			//vastustaja kysyy meiltä tuloksia eli vastataan omilla osumilla(laiva.c)
			//		vastaanotetaan oid==4 ja GET-REQUEST
			//		gameresultsAsked=1
			//TESTAUSTA VAILLE VALMIS
			if (oid==4 && vast.PDU.PDU_type==0xA0) {
				//while(resultReady == 0){NutSleep(100); printf("\nodotetaan että resultReady muuttuu -> 1");}	// Odotetaan, kunnes lähetettävät tulokset on laskettu
				//parseSnmpResponse(4, seaShotEffects);
				game.resultsAsked=1;
//17.4.2009
				GR_4 = vast;
			}

			//vastustaja lähettänyt SALVO-RESULTS eli osumat
			//		vastaanotetaan oid==4 ja GET-RESPONSE
			//		game.resultsSend=1
			//TESTAUSTA VAILLE VALMIS
			if (oid==4 && vast.PDU.PDU_type==0xA2) {
				for (i=0 ; i<5 ; i++)
					oppShotEffects[i] = (u_short) vast.value.value_value[i];
				game.resultsSend = 1;
			}

			//vastustaja lähettää TRAP-viestin
			//		vastaanotetaan oid==6
			//		game->gameStatus=CANCELLED
			//TRAP-VIESTIT!!!???
			if (oid==6)
				game.gameStatus = CANCELLED;

			/**************************************************/
			//Tulostetaan vastaanotettu viesti, jos oid € (2, 3, 5 )
			if ( ((oid==2 || oid==3 || oid==4) && verbose == 1) || oid==5)  {
				printf("\nVastaanotettu viesti: ");
				for (i = 0; i < vast.value.value_length; i++) {
					if (oid == 5)
						printf("%c", vast.value.value_value[i]);
					else
						printf("%d", vast.value.value_value[i]);
				}
				printf("\n");
			}

		}
		//NutSleep(1000);
		NutSleep(1);//20);	//17.4.2009: oli 200
	}
}

/*! \fn void snmpReceive(void)
 *  \brief Funktio, joka parsii vastaanotetun SNMP viestin uuteen structiin.
 *  \return void.
 */
void snmpReceive(void)
{
	int indeksi;
	vast.message.message_type = vv.daatta[0];
	vast.message.message_length = vv.daatta[1];

	vast.version.version_type = vv.daatta[2];
	vast.version.version_length = vv.daatta[3];
	vast.version.version_value = vv.daatta[4];

	vast.comm_string.comm_string_type = vv.daatta[5];
	vast.comm_string.comm_string_length = vv.daatta[6];

	int i;
	for(i = 0 ; i < vast.comm_string.comm_string_length ; i++){
		vast.comm_string.comm_string_value[i] = vv.daatta[7 + i];
	}
	indeksi = 7 + i;

	vast.PDU.PDU_type = vv.daatta[indeksi];
	indeksi++;
	vast.PDU.PDU_length = vv.daatta[indeksi];
	indeksi++;

	vast.requestID.requestID_type = vv.daatta[indeksi];
	indeksi++;
	vast.requestID.requestID_length = vv.daatta[indeksi];
	indeksi++;

	if(vast.requestID.requestID_length > 1){
		for(i = 0 ; i < vast.requestID.requestID_length ; i++){
			vast.requestID.requestID_value[i] = vv.daatta[indeksi];
			indeksi++;
		}
	}
	else{
		vast.requestID.requestID_value[0] = vv.daatta[indeksi];
		indeksi++;
	}

	vast.error.error_type = vv.daatta[indeksi];
	indeksi++;
	vast.error.error_length = vv.daatta[indeksi];
	indeksi++;
	vast.error.error_value = vv.daatta[indeksi];
	indeksi++;

	vast.error_index.error_index_type = vv.daatta[indeksi];
	indeksi++;
	vast.error_index.error_index_length = vv.daatta[indeksi];
	indeksi++;
	vast.error_index.error_index_value = vv.daatta[indeksi];
	indeksi++;

	vast.varbind_list.varbind_list_type = vv.daatta[indeksi];
	indeksi++;
	vast.varbind_list.varbind_list_length = vv.daatta[indeksi];
	indeksi++;

	vast.varbind.varbind_type = vv.daatta[indeksi];
	indeksi++;
	vast.varbind.varbind_length = vv.daatta[indeksi];
	indeksi++;

	vast.OID.OID_type = vv.daatta[indeksi];
	indeksi++;
	vast.OID.OID_length = vv.daatta[indeksi];
	indeksi++;

	for(i = 0 ; i < vast.OID.OID_length ; i++){
		vast.OID.OID_value[i] = vv.daatta[indeksi];
		indeksi++;
	}

	vast.value.value_type = vv.daatta[indeksi];
	indeksi++;
	vast.value.value_length = vv.daatta[indeksi];
	indeksi++;

	for(i = 0 ; i < vast.value.value_length ; i++){
		vast.value.value_value[i] = vv.daatta[indeksi];
		indeksi++;
	}

	//char nimi[] = "vast";
	//printStruct(nimi, &vast);
}

/*! \fn u_short checkOID(void)
 *  \brief Funktio, joka tarkistaa ja palauttaa vastaanotetun SNMP-viestin OID:n.
 *  \return oid SNMP-viestin OID.
 */
u_short checkOID(void) {
	u_short oid = vast.OID.OID_value[6];

	if (vv.osoite == game.playerAddress) {

		if (verbose) {
			switch (oid) {
			case 1:
				printf("Received READY-TO-PLAY message\n");
				break;
			case 2:
				printf("Received START-TURN message\n");
				break;
			case 3:
				printf("Received SALVO message\n");
				break;
			case 4:
				printf("Received SALVO-RESULTS message\n");
				break;
			case 5:
				printf("Received CHAT MESSAGE message\n");
				break;
			case 6:
				printf("Received QUIT-TRAP message\n");
				break;
			default:
				printf("checkOID(): Invalid OID value.\n");
				break;
			}
		}

	}//if

	return oid;
}

/*
 * Funktio: void parseSnmpResponse()
 * Parsii lähetettävän SNMP viestin tavu tavulta vastaanotetun viestin pohjalle.
 * Parametrit: u_short oid, u_short value: virheen tai value kentän arvo
 */
/*! \fn void parseSnmpResponse(u_short oid, u_short *value)
 *  \brief Funktio, joka parsii lähetettävän SNMP viestin tavu tavulta vastaanotetun viestin pohjalle.
 *  \param oid Vastaanotetun viestin OID.
 *  \param *value Value-kentän arvo.
 *  \return void.
 */
void parseSnmpResponse(u_short oid, u_short *value)
{
	int indeksi;
	lahteva.message.message_type = 0x30;
//	lahteva.message.message_length = vv.daatta[1];

	lahteva.version.version_type = 0x02;
	lahteva.version.version_length = 0x01;
	lahteva.version.version_value = 0x00;

	lahteva.comm_string.comm_string_type = 0x04;
	lahteva.comm_string.comm_string_length = vast.comm_string.comm_string_length;

	int i;
	for(i = 0 ; i < lahteva.comm_string.comm_string_length ; i++){
		lahteva.comm_string.comm_string_value[i] = vast.comm_string.comm_string_value[i];
	}
	indeksi = 7 + i;

	lahteva.PDU.PDU_type = 0xA2;	//GET-RESPONSE

	indeksi++;
//	lahteva.PDU.PDU_length = vv.daatta[indeksi];	//lasketaan kenttien pituudet mmyöhempänä
	indeksi++;

	lahteva.requestID.requestID_type = vast.requestID.requestID_type;
	indeksi++;
	lahteva.requestID.requestID_length = vast.requestID.requestID_length;
	indeksi++;

	if(lahteva.requestID.requestID_length > 1){
		for(i = 0 ; i < lahteva.requestID.requestID_length ; i++){
			lahteva.requestID.requestID_value[i] = vast.requestID.requestID_value[i];
			indeksi++;
		}
	}
	else{
		lahteva.requestID.requestID_value[0] = vast.requestID.requestID_value[0];
		indeksi++;
	}

	lahteva.error.error_type = 0x02;
	indeksi++;
	lahteva.error.error_length = 0x01;
	indeksi++;
	lahteva.error.error_value = 0x00;
	indeksi++;

	lahteva.error_index.error_index_type = 0x02;
	indeksi++;
	lahteva.error_index.error_index_length = 0x01;
	indeksi++;
	lahteva.error_index.error_index_value = 0x00;
	indeksi++;

	lahteva.varbind_list.varbind_list_type = 0x30;
	indeksi++;
//	lahteva.varbind_list.varbind_list_length = vv.daatta[indeksi];
	indeksi++;

	lahteva.varbind.varbind_type = 0x30;
	indeksi++;
//	lahteva.varbind.varbind_length = vv.daatta[indeksi];
	indeksi++;

	lahteva.OID.OID_type = 0x06;
	indeksi++;
	lahteva.OID.OID_length = 0x07;
	indeksi++;

	lahteva.OID.OID_value[0] = 0x2B;
	indeksi++;
	lahteva.OID.OID_value[1] = 0x06;
	indeksi++;
	lahteva.OID.OID_value[2] = 0x01;
	indeksi++;
	lahteva.OID.OID_value[3] = 0x03;
	indeksi++;
	lahteva.OID.OID_value[4] = 0x37;
	indeksi++;
	lahteva.OID.OID_value[5] = 0x00;
	indeksi++;
//	lahteva.OID.OID_value[0] = 0x43;
	indeksi++;

	lahteva.OID.OID_value[6] = vast.OID.OID_value[6];

//	lahteva.value.value_type = vv.daatta[indeksi];

	//lahteva.value.value_type = 0x05;	//NULL
	indeksi++;
	//lahteva.value.value_length = 0;	//Integer, ready-to-play, noot
	indeksi++;

	switch (oid) {
	case 1:
		if (verbose) {
			printf("********** RECEIVER CASE: 1 ***********\n");
			printf("Sending READY-TO-PLAY answer to %s:%d\n", inet_ntoa(vv.osoite), vv.portti );
		}
		lahteva.error.error_value = 0x00;
		lahteva.value.value_type = 0x02;
		lahteva.value.value_length = 0x01;
		lahteva.value.value_value[0] = value[0];	//6.4.2009, 14.4.2009
		break;
	case 2:

		if (verbose) {
			printf("********** RECEIVER CASE: 2 ***********\n");
			printf("Sending START-TURN answer to %s:%d\n", inet_ntoa(vv.osoite), vv.portti );
		}
		lahteva.error.error_value = value[0];	//14.4.2009
		lahteva.value.value_type = 0x05;		//6.4.2009
		lahteva.value.value_length = 0x00;
		//lahteva.value.value_value = NULL;
		break;
	case 3:
		if (verbose) {
			printf("********** RECEIVER CASE: 3 ***********\n");
			printf("Sending SALVO answer to %s:%d\n", inet_ntoa(vv.osoite), vv.portti );
		}
		lahteva.error.error_value = value[0];
		lahteva.value.value_type = 0x05;
		lahteva.value.value_length = 0x00;
		//lahteva.value.value = NULL;
		break;
	case 4:
		if (verbose) {
			printf("********** RECEIVER CASE: 4 ***********\n");
			printf("Sending SALVO-RESULTS answer to %s:%d\n", inet_ntoa(vv.osoite), vv.portti );
		}
		lahteva.error.error_value = 0x00;
		lahteva.value.value_type = 0x30;//sequence of 5 integers
		//TODO: epästandardia...
		//lahteva.value.value_type = 0x04;
		lahteva.value.value_length = 0x05;
		for ( i=0 ; i<5; i++) //lahteva.value.value_length ; i++)
			lahteva.value.value_value[i] = (int) value[i];
		break;
	case 5:
		if (verbose) {
			printf("********** RECEIVER CASE: 5 ***********\n");
			printf("Sending CHAT-MESSAGE confirmation to %s:%d\n", inet_ntoa(vv.osoite), vv.portti );
		}
		lahteva.error.error_value = 0x05;//error 0x05, jos ei tueta chattia
		lahteva.value.value_type = 0x05;
		lahteva.value.value_length = 0x00;
		//lahteva.value.value = NULL;
		break;
	//case 6:sta ei ole tehty snmpSendin osalta
	//Ei tarvitse vastata TRAPiin, mutta peli pitää keskeyttää!!??
	case 6:
		//ABORT ABORT eli lopeta peli
		break;
	}

	lahteva.varbind.varbind_length = lahteva.value.value_length + 2 + lahteva.OID.OID_length + 2;
	lahteva.varbind_list.varbind_list_length = lahteva.varbind.varbind_length + 2;
	lahteva.PDU.PDU_length = lahteva.varbind_list.varbind_list_length + 10 + lahteva.requestID.requestID_length;
	lahteva.message.message_length = lahteva.PDU.PDU_length + 7 + lahteva.comm_string.comm_string_length;

	if (oid == 1)
		compileOutboundMessage(&lahteva, vv.osoite, vv.portti);
	else
		compileOutboundMessage(&lahteva, game.playerAddress, vv.portti);//14.4.2009
}



/*
 * Funktio: void parseSnmpResponse()
 * Parsii lähetettävän SNMP viestin tavu tavulta vastaanotetun viestin pohjalle.
 * Parametrit: u_short oid, u_short value: virheen tai value kentän arvo
 */
/*! \fn void parseSnmpResponse_ok(struct snmp *lahteva_ok ,u_short oid, u_short *value)
 *  \brief Funktio, joka parsii lähetettävän SNMP viestin tavu tavulta vastaanotetun viestin pohjalle.
 *  \param *lahteva_ok Tietue, jonka pohjalle lähetettävä viesti rakennetaan.
 *  \param oid Vastaanotetun viestin OID.
 *  \param *value Value-kentän arvo.
 *  \return void.
 */
void parseSnmpResponse_ok(struct snmp *lahteva_ok ,u_short oid, u_short *value)
{
	int indeksi;
	lahteva_ok->message.message_type = 0x30;
//	lahteva_ok->message.message_length = vv.daatta[1];

	lahteva_ok->version.version_type = 0x02;
	lahteva_ok->version.version_length = 0x01;
	lahteva_ok->version.version_value = 0x00;

	lahteva_ok->comm_string.comm_string_type = 0x04;
	lahteva_ok->comm_string.comm_string_length = vast.comm_string.comm_string_length;

	int i;
	for(i = 0 ; i < lahteva_ok->comm_string.comm_string_length ; i++){
		lahteva_ok->comm_string.comm_string_value[i] = vast.comm_string.comm_string_value[i];
	}
	indeksi = 7 + i;

	lahteva_ok->PDU.PDU_type = 0xA2;	//GET-RESPONSE

	indeksi++;
//	lahteva_ok->PDU.PDU_length = vv.daatta[indeksi];	//lasketaan kenttien pituudet mmyöhempänä
	indeksi++;

	lahteva_ok->requestID.requestID_type = vast.requestID.requestID_type;
	indeksi++;
	lahteva_ok->requestID.requestID_length = vast.requestID.requestID_length;
	indeksi++;

	if(lahteva_ok->requestID.requestID_length > 1){
		for(i = 0 ; i < lahteva_ok->requestID.requestID_length ; i++){
			lahteva_ok->requestID.requestID_value[i] = vast.requestID.requestID_value[i];
			indeksi++;
		}
	}
	else{
		lahteva_ok->requestID.requestID_value[0] = vast.requestID.requestID_value[0];
		indeksi++;
	}

	lahteva_ok->error.error_type = 0x02;
	indeksi++;
	lahteva_ok->error.error_length = 0x01;
	indeksi++;
	lahteva_ok->error.error_value = 0x00;
	indeksi++;

	lahteva_ok->error_index.error_index_type = 0x02;
	indeksi++;
	lahteva_ok->error_index.error_index_length = 0x01;
	indeksi++;
	lahteva_ok->error_index.error_index_value = 0x00;
	indeksi++;

	lahteva_ok->varbind_list.varbind_list_type = 0x30;
	indeksi++;
//	lahteva_ok->varbind_list.varbind_list_length = vv.daatta[indeksi];
	indeksi++;

	lahteva_ok->varbind.varbind_type = 0x30;
	indeksi++;
//	lahteva_ok->varbind.varbind_length = vv.daatta[indeksi];
	indeksi++;

	lahteva_ok->OID.OID_type = 0x06;
	indeksi++;
	lahteva_ok->OID.OID_length = 0x07;
	indeksi++;

	lahteva_ok->OID.OID_value[0] = 0x2B;
	indeksi++;
	lahteva_ok->OID.OID_value[1] = 0x06;
	indeksi++;
	lahteva_ok->OID.OID_value[2] = 0x01;
	indeksi++;
	lahteva_ok->OID.OID_value[3] = 0x03;
	indeksi++;
	lahteva_ok->OID.OID_value[4] = 0x37;
	indeksi++;
	lahteva_ok->OID.OID_value[5] = 0x00;
	indeksi++;
//	lahteva_ok->OID.OID_value[0] = 0x43;
	indeksi++;

	lahteva_ok->OID.OID_value[6] = vast.OID.OID_value[6];

//	lahteva_ok->value.value_type = vv.daatta[indeksi];

	//lahteva_ok->value.value_type = 0x05;	//NULL
	indeksi++;
	//lahteva_ok->value.value_length = 0;	//Integer, ready-to-play, noot
	indeksi++;

	switch (oid) {
	case 1:
		if (verbose) {
			printf("********** RECEIVER CASE: 1 ***********\n");
			printf("Sending READY-TO-PLAY answer to %s:%d\n", inet_ntoa(vv.osoite), vv.portti );
		}
		lahteva_ok->error.error_value = 0x00;
		lahteva_ok->value.value_type = 0x02;
		lahteva_ok->value.value_length = 0x01;
		lahteva_ok->value.value_value[0] = value[0];	//6.4.2009, 14.4.2009
		break;
	case 2:
		if (verbose) {
			printf("********** RECEIVER CASE: 2 ***********\n");
			printf("Sending START-TURN answer to %s:%d\n", inet_ntoa(vv.osoite), vv.portti );
		}
		lahteva_ok->error.error_value = value[0];	//14.4.2009
		lahteva_ok->value.value_type = 0x05;		//6.4.2009
		lahteva_ok->value.value_length = 0x00;
		//lahteva_ok->value.value_value = NULL;
		break;
	case 3:
		if (verbose) {
			printf("********** RECEIVER CASE: 3 ***********\n");
			printf("Sending SALVO answer to %s:%d\n", inet_ntoa(vv.osoite), vv.portti );
		}
		lahteva_ok->error.error_value = value[0];
		lahteva_ok->value.value_type = 0x05;
		lahteva_ok->value.value_length = 0x00;
		//lahteva_ok->value.value = NULL;
		break;
	case 4:

		if (verbose) {
			printf("********** RECEIVER CASE: 4 ***********\n");
			printf("Sending SALVO-RESULTS answer to %s:%d\n", inet_ntoa(vv.osoite), vv.portti );
		}
		lahteva_ok->error.error_value = 0x00;
		lahteva_ok->value.value_type = 0x30;//sequence of 5 integers
		//TODO: epästandardia...
		//lahteva_ok->value.value_type = 0x04;
		lahteva_ok->value.value_length = 0x05;
		for ( i=0 ; i<5; i++) //lahteva_ok->value.value_length ; i++)
			lahteva_ok->value.value_value[i] = (int) value[i];
		break;
	case 5:
		if (verbose) {
			printf("********** RECEIVER CASE: 5 ***********\n");
			printf("Sending CHAT-MESSAGE confirmation to %s:%d\n", inet_ntoa(vv.osoite), vv.portti );
		}
		lahteva_ok->error.error_value = 0x05;//error 0x05, jos ei tueta chattia, 0x00 jos tuetaan
		lahteva_ok->value.value_type = 0x05;
		lahteva_ok->value.value_length = 0x00;
		//lahteva_ok->value.value = NULL;
		break;
	//case 6:sta ei ole tehty snmpSendin osalta
	//Ei tarvitse vastata TRAPiin, mutta peli pitää keskeyttää!!??
	case 6:
		//ABORT ABORT eli lopeta peli
		break;
	}

	lahteva_ok->varbind.varbind_length = lahteva_ok->value.value_length + 2 + lahteva_ok->OID.OID_length + 2;
	lahteva_ok->varbind_list.varbind_list_length = lahteva_ok->varbind.varbind_length + 2;
	lahteva_ok->PDU.PDU_length = lahteva_ok->varbind_list.varbind_list_length + 10 + lahteva_ok->requestID.requestID_length;
	lahteva_ok->message.message_length = lahteva_ok->PDU.PDU_length + 7 + lahteva_ok->comm_string.comm_string_length;

	if (oid == 1)
		compileOutboundMessage(lahteva_ok, vv.osoite, vv.portti);
	else
		compileOutboundMessage(lahteva_ok, game.playerAddress, vv.portti);//14.4.2009
}


/*
 * Parsii lähetettävän SNMP-viestin tavu tavulta.
 *
 *  Lähetettävien viestien numerot:
 * 	1: READY-TO-PLAY (GET-REQUEST (OID:1) )
 * 	2: START-TURN (SET-REQUEST (OID:2 ) )
 * 	2: START-TURN (SET-REQUEST (OID:2) höystettynä vuoronumerolla (value = vuoronumero))
 *  3: SALVO (SET-REQUEST (OID:3) value = sequence of 5 integers)
 *  4: SALVO_RESULTS (GET-REQUEST (OID:4))
 *  5: CHAT (SET-REQUEST (OID: 5))
 *  6: Ending the game (TRAP (OID: 6) value = optional octet string)
 */
/*! \fn void parseOutboundMessage(u_char OIDlastValue, u_short value[], u_short value_length)
 *  \brief Funktio, joka parsii lähetettävän SNMP-viestin tavu tavulta.
 *  \param OIDlastValue OID-kentän viimeinen numero.
 *  \param *value Value-kentän arvo.
 *  \param value_length Value-kentän koko.
 *  \return void.
 */
void parseOutboundMessage(u_char OIDlastValue, u_short value[], u_short value_length) {
	u_short i;
	struct snmp outb;
	u_long osoite = game.playerAddress;	//Vastustajan IP-osoite
	u_short portti;	//Vastaanottajan portti

	outb.message.message_type = 0x30;

	outb.version.version_type = 0x02;
	outb.version.version_length = 0x01;
	outb.version.version_value = 0x00;

	outb.comm_string.comm_string_type = 0x04;
	outb.comm_string.comm_string_length	= 0x0A;
	outb.comm_string.comm_string_value[0] = 'B';
	outb.comm_string.comm_string_value[1] = 'a';
	outb.comm_string.comm_string_value[2] = 't';
	outb.comm_string.comm_string_value[3] = 't';
	outb.comm_string.comm_string_value[4] = 'l';
	outb.comm_string.comm_string_value[5] = 'e';
	outb.comm_string.comm_string_value[6] = 's';
	outb.comm_string.comm_string_value[7] = 'h';
	outb.comm_string.comm_string_value[8] = 'i';
	outb.comm_string.comm_string_value[9] = 'p';

	outb.requestID.requestID_type = 0x02;
	outb.requestID.requestID_length = 0x01;
	outb.requestID.requestID_value[0] = 0x04;

	outb.error.error_type = 0x02;
	outb.error.error_length = 0x01;
	outb.error.error_value = 0x00;

	outb.error_index.error_index_type = 0x02;
	outb.error_index.error_index_length = 0x01;
	outb.error_index.error_index_value = 0x00;

	outb.varbind_list.varbind_list_type = 0x30;

	outb.varbind.varbind_type = 0x30;

	outb.OID.OID_type = 0x06;
	outb.OID.OID_length = 0x07;
	outb.OID.OID_value[0] = 0x2B;
	outb.OID.OID_value[1] = 0x06;
	outb.OID.OID_value[2] = 0x01;
	outb.OID.OID_value[3] = 0x03;
	outb.OID.OID_value[4] = 0x37;
	outb.OID.OID_value[5] = 0x00;

	switch (OIDlastValue) {
	case 1:
		if (verbose)
			printf("SENDER CASE: 1 *** ");
		outb.PDU.PDU_type = 0xA0;
		outb.OID.OID_value[6] = 0x01;
		outb.value.value_type = 0x05;	//NULL
		outb.value.value_length = 0x00;
		osoite = inet_addr(UDP_BCA);
		portti = 161;
		if (verbose)
			printf("Sending READY-TO-PLAY request to %s:%d\n", inet_ntoa(osoite), portti );
		break;
	case 2:
		if (verbose)
			printf("SENDER CASE: 2 *** ");
		outb.PDU.PDU_type = 0xA3;
		outb.OID.OID_value[6] = 0x02;
		outb.value.value_type = 0x02;	//Integer
		outb.value.value_length = 0x01;
		outb.value.value_value[0] = value[0];
		portti = 161;
		if (verbose)
			printf("Sending START-TURN request to %s:%d\n", inet_ntoa(osoite), portti );
		break;
	case 3:
		if (verbose)
			printf("SENDER CASE: 3 *** ");
		outb.PDU.PDU_type = 0xA3;
		outb.OID.OID_value[6] = 0x03;
		outb.value.value_type = 0x30;	//Sequence
		//TODO: ALEMPI RIVI EI OLE STANDARDIN MUKAINEN!!!!
		//outb.value.value_type = 0x04;	//Octet-String 12.4.2009
		outb.value.value_length = 0x0A;
		for ( i=0 ; i<10 ; i++ ) {
			outb.value.value_value[i] = (int) value[i];
			//printf("i: %d, value[i]: %d\n", i, value[i]);
		}
		portti = 161;
		if (verbose)
			printf("Sending SALVO request to %s:%d\n", inet_ntoa(osoite), portti );
		break;
	case 4:
		if (verbose)
			printf("SENDER CASE: 4 *** ");
		outb.PDU.PDU_type = 0xA0;
		outb.OID.OID_value[6] = 0x04;
		outb.value.value_type = 0x05;	//NULL
		outb.value.value_length = 0x00;
		portti = 161;
		if (verbose)
			printf("Sending SALVO-RESULTS request to %s:%d\n", inet_ntoa(osoite), portti  );
		break;
	case 5:
		if (verbose)
			printf("SENDER CASE: 5 *** ");
		outb.PDU.PDU_type = 0xA3;
		outb.OID.OID_value[6] = 0x05;
		outb.value.value_type = 0x04;	//Octet-String
		outb.value.value_length = 0x05;
		for ( i=0 ; i<value_length ; i++ )
			outb.value.value_value[i] = value[i];
		portti = 161;
		if (verbose)
			printf("Sending CHAT-MESSAGE to %s:%d\n", inet_ntoa(osoite), portti  );
		break;
	case 6:	//TODO: Tähän 27.3.2009. Trap viesti vielä arvoitus
		if (verbose)
			printf("SENDER CASE: 6 / TRAP *** ");
		outb.PDU.PDU_type = 0xA4;//0x02;//p.o. integer? 0xA0;	//TARKASTA TYYPPI 0xA4 tunnistuu wiresharkissa...15.4.2009
		outb.OID.OID_value[6] = 0x06;
		outb.value.value_type = 0x04;	//TARKASTA TYYPPI
		outb.value.value_length = 0x07;	//TARKASTA PITUUS (lopetusviestin mukaan)
		outb.value.value_value[0] = 'I';
		outb.value.value_value[1] = ' ';
		outb.value.value_value[2] = 'q';
		outb.value.value_value[3] = 'u';
		outb.value.value_value[4] = 'i';
		outb.value.value_value[5] = 't';
		outb.value.value_value[6] = '.';
		portti = 161;
		if (verbose)
			printf("Sending TRAP to %s:%d\n", inet_ntoa(osoite), portti  );
		break;
	}
	//printf("outb.value.value_length: %d\n", outb.value.value_length);

	outb.varbind.varbind_length = outb.value.value_length + 2
			+ outb.OID.OID_length + 2;
	outb.varbind_list.varbind_list_length = outb.varbind.varbind_length + 2;
	outb.PDU.PDU_length = outb.varbind_list.varbind_list_length + 10
			+ outb.requestID.requestID_length;
	outb.message.message_length = outb.PDU.PDU_length + 7
			+ outb.comm_string.comm_string_length;

	compileOutboundMessage(&outb, osoite, portti);

	//char out[] = "outb";
	//printStruct(out, &outb);
}

/*
 * Laskee SNMP-viestin pituuskenttien arvot
 */
/*! \fn void void calculateLengths(struct snmp *laskettava)
 *  \brief Funktio, joka laskee ja asettaa SNMP-viestin pituuskenttien arvot.
 *  \param *laskettava Tietue, jonka pituuskenttien arvot lasketaan.
 *  \return void.
 */
void calculateLengths(struct snmp *laskettava) {
	//Lasketaan pituuskenttien arvot
	lahteva.varbind.varbind_length = lahteva.value.value_length + 2 + lahteva.OID.OID_length + 2;
	lahteva.varbind_list.varbind_list_length = lahteva.varbind.varbind_length + 2;
	lahteva.PDU.PDU_length = lahteva.varbind_list.varbind_list_length + 10 + lahteva.requestID.requestID_length;
	lahteva.message.message_length = lahteva.PDU.PDU_length + 7 + lahteva.comm_string.comm_string_length;
}

/*
 * compileOutboundMessage
 * Parsii SNMP-structin kanavaan lähetettäväksi viesti-strucktiksi
 * lisäämällä SNMP-structiin kohde IP:n ja portin.
 */
/*! \fn void compileOutboundMessage(const struct snmp *lahetettava, u_long osoite, u_short portti)
 *  \brief Funktio, joka parsii SNMP-structin kanavaan lähetettäväksi viesti-strucktiksi
 * 		   lisäämällä SNMP-structiin kohde IP:n ja portin numeron.
 *  \param *lahetettava Lähetettävä tietue.
 *  \param osoite IP-osoite, johon viesti lähetetään.
 *  \param portti Portti, johon viesti lähetetään.
 *  \return void.
 */
void compileOutboundMessage(const struct snmp *lahetettava, u_long osoite, u_short portti) {
	//Parsitaan kanavaan lähetettävä viesti
	int indeksi = 0;
	int i = 0;

	lv.daatta[indeksi] = lahetettava->message.message_type;
	indeksi++;
	lv.daatta[indeksi] = lahetettava->message.message_length;
	indeksi++;
	lv.daatta[indeksi] = lahetettava->version.version_type;
	indeksi++;
	lv.daatta[indeksi] = lahetettava->version.version_length;
	indeksi++;
	lv.daatta[indeksi] = lahetettava->version.version_value;
	indeksi++;
	lv.daatta[indeksi] = lahetettava->comm_string.comm_string_type;
	indeksi++;
	lv.daatta[indeksi] = lahetettava->comm_string.comm_string_length;
	indeksi++;
	for (i = 0; i < lahetettava->comm_string.comm_string_length; i++) {
		lv.daatta[indeksi] = lahetettava->comm_string.comm_string_value[i];
		indeksi++;
	}
	lv.daatta[indeksi] = lahetettava->PDU.PDU_type;
	indeksi++;
	lv.daatta[indeksi] = lahetettava->PDU.PDU_length;
	indeksi++;
	lv.daatta[indeksi] = lahetettava->requestID.requestID_type;
	indeksi++;
	lv.daatta[indeksi] = lahetettava->requestID.requestID_length;
	indeksi++;
	for (i = 0; i < lahetettava->requestID.requestID_length; i++) {
		lv.daatta[indeksi] = lahetettava->requestID.requestID_value[i];
		indeksi++;
	}
	lv.daatta[indeksi] = 0x02; //error type
	indeksi++;
	lv.daatta[indeksi] = 0x01; //error length
	indeksi++;
	lv.daatta[indeksi] = lahetettava->error.error_value; //error value
	indeksi++;
	lv.daatta[indeksi] = 0x02; //error index type
	indeksi++;
	lv.daatta[indeksi] = 0x01; //error index length
	indeksi++;
	lv.daatta[indeksi] = 0x00; //error index value
	indeksi++;
	lv.daatta[indeksi] = 0x30; //varbind list type
	indeksi++;
	lv.daatta[indeksi] = lahetettava->varbind_list.varbind_list_length;
	indeksi++;
	lv.daatta[indeksi] = 0x30; //varbind type
	indeksi++;
	lv.daatta[indeksi] = lahetettava->varbind.varbind_length;
	indeksi++;
	lv.daatta[indeksi] = 0x06; //OID type
	indeksi++;
	lv.daatta[indeksi] = 0x07; //OID length
	indeksi++;
	for (i = 0; i < lahetettava->OID.OID_length; i++) {
		lv.daatta[indeksi] = lahetettava->OID.OID_value[i];
		indeksi++;
	}
	lv.daatta[indeksi] = lahetettava->value.value_type;
	indeksi++;
	lv.daatta[indeksi] = lahetettava->value.value_length;
	indeksi++;
	for (i = 0; i < lahetettava->value.value_length; i++) {
		//printf("lahetettava->value.value_value[%d]: %d\n",i, lahetettava->value.value_value[i]);
		lv.daatta[indeksi] = lahetettava->value.value_value[i];
		indeksi++;
	}
	//if(verbose) printf("\n");

	lv.daatankoko = lahetettava->message.message_length + 2;

	lv.osoite = osoite;
	lv.portti = portti;

	//Lähetetään viesti kanavaan
	sendMessageToChannel(&lv);
}

/*
 * Funktio: sendMessage
 * Hoitaa viestien lähetyksen kanavaan.
 */
/*! \fn void sendMessageToChannel(struct viesti *lv)
 *  \brief Funktio, joka lähettää viestin kanavaan.
 *  \param lv Lähetettävä tietue.
 *  \return void.
 */
void sendMessageToChannel(struct viesti *lv)
{
	u_short error;

	if (verbose)
		printf("sendMessageToChannel(): Sending %d bytes to %s:%d. ", lv->daatankoko, inet_ntoa(lv->osoite), lv->portti);

	//NutSleep(1000);
	error = NutUdpSendTo(sock, lv->osoite, lv->portti, lv->daatta, lv->daatankoko);

	if(error == 0) {
		if (verbose)
			printf("Lähetys onnistui.\n");
	}
	else {
		printf("****** SNMP-viestin lähetys epäonnistui. ******\n");
		printf("Väärä subnet mask tai IP:tä ei määritetty?\n\n");
	}
}

/*
 * Funktio, joka tulostaa SNMP-structin.
 * Käyttö:
 * char nimi[] = "structin nimi";
 * printStruct(nimi, &structi);
 */
/*! \fn void printStruct(char* nimi, const struct snmp *tulostettava)
 *  \brief Funktio, joka tulostaa SNMP-structin.
 *  \param *nimi Tulostettavan tietueen nimi.
 *  \param *tulostettava Tulostettava tietue.
 *  \return void.
 */
void printStruct(char* nimi, const struct snmp *tulostettava) {
	int i;

	printf("\n\n*** Tästä alkaa structin tulostus. ***\n");
	printf("%s.message.message_type: %x\n", nimi,tulostettava->message.message_type);
	printf("%s.message.message_length: %x\n", nimi, tulostettava->message.message_length);

	printf("%s.version.version_type: %x\n", nimi, tulostettava->version.version_type);
	printf("%s.version.version_length: %x\n", nimi, tulostettava->version.version_length);
	printf("%s.version.version_value: %x\n", nimi, tulostettava->version.version_value);

	printf("%s.comm_string.comm_string_type: %x\n", nimi, tulostettava->comm_string.comm_string_type);
	printf("%s.comm_string.comm_string_length: %x\n", nimi, tulostettava->comm_string.comm_string_length);

	for (i = 0; i < tulostettava->comm_string.comm_string_length; i++) {
		printf("%s.comm_string.comm_string_value[%d]: %x\n", nimi, i, tulostettava->comm_string.comm_string_value[i]);
	}

	printf("%s.PDU.PDU_type: %x\n", nimi, tulostettava->PDU.PDU_type);
	printf("%s.PDU.PDU_length: %x\n", nimi, tulostettava->PDU.PDU_length);

	printf("%s.requestID.requestID_type: %x\n", nimi, tulostettava->requestID.requestID_type);
	printf("%s.requestID.requestID_length: %x\n", nimi, tulostettava->requestID.requestID_length);

	if ( tulostettava->requestID.requestID_length > 1 ) {
		for (i = 0; i < tulostettava->requestID.requestID_length; i++) {
			printf("%s.requestID.requestID_value[%d]: %x\n", nimi, i, tulostettava->requestID.requestID_value[i]);
		}
	} else {
		printf("%s.requestID.requestID_value[0]: %x\n", nimi, tulostettava->requestID.requestID_value[0]);
	}

	printf("%s.error.error_type: %x\n", nimi, tulostettava->error.error_type);
	printf("%s.error.error_length: %x\n", nimi, tulostettava->error.error_length);
	printf("%s.error.error_value: %x\n", nimi, tulostettava->error.error_value);

	printf("%s.error_index.error_index_type: %x\n", nimi, tulostettava->error_index.error_index_type);
	printf("%s.error_index.error_index_length: %x\n", nimi, tulostettava->error_index.error_index_length);
	printf("%s.error_index.error_index_value: %x\n", nimi, tulostettava->error_index.error_index_value);

	printf("%s.varbind_list.varbind_list_type: %x\n", nimi, tulostettava->varbind_list.varbind_list_type);
	printf("%s.varbind_list.varbind_list_length: %x\n", nimi, tulostettava->varbind_list.varbind_list_length);

	printf("%s.varbind.varbind_type: %x\n", nimi, tulostettava->varbind.varbind_type);
	printf("%s.varbind.varbind_length: %x\n", nimi, tulostettava->varbind.varbind_length);

	printf("%s.OID.OID_type: %x\n", nimi, tulostettava->OID.OID_type);
	printf("%s.OID.OID_length: %x\n", nimi, tulostettava->OID.OID_length);

	for (i = 0; i < tulostettava->OID.OID_length; i++) {
		printf("%s.OID.OID_value[%d]: %x\n", nimi, i, tulostettava->OID.OID_value[i]);
	}

	printf("%s.value.value_type: %x\n", nimi, tulostettava->value.value_type);
	printf("%s.value.value_length: %x\n", nimi, tulostettava->value.value_length);

	for (i = 0; i < tulostettava->value.value_length; i++) {
		printf("%s.value.value_value[%d]: %x\n", nimi, i, tulostettava->value.value_value[i]);
	}
}

/*! \fn u_short printLahetys(void)
 *  \brief Testausfunktio SNMP-viestien lähetykseen.
 *  \return choice Valinnen numero.
 */
u_short printLahetys(void) {
	u_short choice;

	printf("\n****Send message menu****\n");
	printf("Select a number.\n");
	printf("1:Send READY-TO-PLAY\n");
	printf("2:Send START-TURN\n");
	printf("3:Send SALVO\n");
	printf("4:Send SALVO-RESULTS\n");
	printf("5:Send CHAT MESSAGE\n");
	printf("6:Send QUIT-TRAP\n");
	printf("7:Aseta vastustajan IP-osoite.\n");
	printf("8:vastustajan ip: 10.10.4.99");
	printf("\n");

	fflush(stdin);
	choice = (int) getc(stdin);
	return choice;
}

/*
 * Testausfunktio, joka lähettää kanavaan yhden kuudesta
 * SNMP-viesteistä.
 */
/*! \fn void sendSnmpMessageTestaus (u_short choice)
 *  \brief Testausfunktio SNMP-viestien lähetykseen.
 *  \param choice Valinnen numero.
 *  \return void.
 */
void sendSnmpMessageTestaus (u_short choice) {
	char inbuf[16];
	char chatBuf[50];
	u_short koko;
	u_short integeri[1];
	// u_short intti;	//TODO: testaa tämä

	switch (choice) {
	case '1':
		printf("Sending READY-TO-PLAY...\n");
		parseOutboundMessage(0x01, NULL, 0);
		break;
	case '2':
		printf("Sending START-TURN...\n");
		integeri[0] = 0;
		//parseOutboundMessage(0x02, 0, 0);//tämä ei toimi(int -> pointer)
		parseOutboundMessage(0x02, integeri, 0);
		//parseOutboundMessage(0x02, &intti, 0);
		break;
	case '3':
		printf("Sending SALVO...\n");
		//parseOutboundMessage(0x03, sequenceOf10Integers, 0);
		break;
	case '4':
		printf("Sending SALVO-RESULTS...\n");
		parseOutboundMessage(0x04, NULL, 0);
		break;
	case '5':
		printf("Kirjoita chat viesti. (max. 50 merkkiä)\n");
		//koko = scanf("%s", chatBuf);	//ei toimi näin koko on aina 1
		//koko = _read(stdin, chatBuf, sizeof(chatBuf));	//tämä kaataa koko paskan
		printf("koko: %d", koko);
		printf("Sending CHAT MESSAGE...\n");
		parseOutboundMessage(0x05, chatBuf, koko);
		break;
	case '6':
		printf("Sending QUIT-TRAP...\n");
		parseOutboundMessage(0x06, "blaablaa", 0);
		//TODO: Miten lähetetään lopetusviesti
		break;
	case '7':
		printf("Anna vastustajan IP-osoite: \n");
		scanf("%s", inbuf);
		//vastustajanIP = inbuf;
		//vIP = inet_addr(vastustajanIP);
		//printf("Uusi vastustajan IP: %s\n", inet_ntoa(vIP));
		break;
	case '8':
		//vIP = inet_addr("10.10.4.99");
		//printf("Uusi vastustajan IP: %s\n", inet_ntoa(vIP));
		break;
	default:
		printf("Invalid choice.\n");
		break;
	}

}

