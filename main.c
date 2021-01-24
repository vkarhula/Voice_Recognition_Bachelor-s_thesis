#include <cfg/crt.h>    /* Floating point configuration. */

#include <stdio.h>
#include <stdlib.h>
#include <io.h>

#include <dev/board.h>
#include <sys/timer.h>

#include <arpa/inet.h>
#include <sys/confnet.h>	//Verkon configurointi

#include "main.h"
#include "verkko.h"
#include "laiva.h"
#include "ioBoard.h"
#include "puheentunnistus.h"

static prog_char presskey_P[] = "Press any key...";
static prog_char pgm_ptr[] = "\nHello stranger!\n";

//Verkon alustus-bannerit
static prog_char SA[] = "\n\n============= Starting up ===============\n";
static prog_char RF[] = "Registering device failed";

static char inbuf[128];

#define MYMAC   0x00, 0x06, 0x98, 0x00, 0x00, 0x00
#define MYIP	"10.10.4.1"
#define MYMASK "255.255.0.0"

#define LOCALMASK "255.255.255.0"
#define GLOBALMASK "255.255.0.0"

//
u_short temp[10] = {0};  //for ship coordinates while searching the place
u_short targetSeaShotX[5] = {0};    // our sea
u_short targetSeaShotY[5] = {0};
u_short targetOppSeaShotX[5] = {0};    // opponent's sea //TwoPlayerGame()
u_short targetOppSeaShotY[5] = {0};		//TODO muuta samaksi kuin yhdenpelaajan peliss‰
u_short targetOppShotX[5] = {0};    // opponent's game //onePlayerGame()
u_short targetOppShotY[5] = {0};


// kaksinpeli
u_short oneValue = 0;                   // one integer value for SNMP-messages
u_short length = 0;
u_short ourCoord[10] = {0};			// our coordinates, sent to opponent sea
u_short oppCoord[10] = {0};			// coordinates send by opponent
u_short oppSeaShotEffects[5] = {0}; //effects we caused to opponent's sea = results
u_short seaShotEffects[5] = {0};    // result how opponent's shots affected, will be returned to opp
u_short oppShotEffects[5] = {0};	//yksinpeli TODO sama yksin- ja kaksinpeliin

/*
 * Funktioesittelyt
 */
void settings(void);
void changeIP(void);
void changeSNM(void);
int spokenWord(int puheenMfcc[][NMBROFFILTERS]); //static int ** puheenMfcc);

/*
 * UART sample.
 *
 * Some functions do not work with ICCAVR.
 */
/*! \fn int main(void)
 *  \brief main-funktio.
 *  \return Paluuarvo.
 */
int main(void)
{
	verbose = 0;	// tulostukset aluksi poissa p‰‰lt‰  //1
    //char choice;
	u_short choice = 0;
	//char choice2;
    u_long baud = 115200;

    static int puheenMfcc[KEHYKSIENLKM][NMBROFFILTERS];// = 0;
    int puhe = 0;
    short jarru = 0;
    u_short skip = 0;

    int puheenNoF;
    int dtwValue = BIG_VALUE;
    int dtwValueTemp;
    u_short tunnistettuLuku = 0;
    u_short nayteNro = 0;
    u_short arvot[20] = {0};


    u_short i = 0, j = 0;

    //puheenMfcc = (int**) malloc (4*KEHYKSIENLKM);
    for (i=0 ; i<KEHYKSIENLKM ; i++) {
    	//puheenMfcc[i] = (int*) malloc (4*NMBROFFILTERS);
    	for (j=0 ; j<NMBROFFILTERS ; j++) {
    		puheenMfcc[i][j] = 0;
    	}
    }

    resetWholeGame();

    /*
     * Initialize the uart device.
    */
    NutRegisterDevice(&DEV_UART, 0, 0);
    freopen(DEV_UART_NAME, "w", stdout);
    freopen(DEV_UART_NAME, "r", stdin);

    _ioctl(_fileno(stdout), UART_SETSPEED, &baud);
    NutSleep(200);

    //ƒ‰nipiirin alustukset
    initAudioDevice();

    //Verkon alustukset
    initializeNetwork();	//
    NutSleep(100);

    /*
     * Nut/OS never expects a thread to return. So we enter an
     * endless loop here.
     */
    for (i = 0;; i++) {

    	/*
    	 * Kaynnistysvalikko
    	 */
    	//printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    	printf("\n\n\t**** LAIVANUPOTUSPELI ****\n\n");
    	printf("\t1: Yksinpeli\n");
    	printf("\t2: Verkkopeli\n");
    	printf("\t3: Asetukset\n");
    	printf("\t4: Tulostus (Verbose Mode On/Off)\n");
    	printf("\t5: Puheentunnistus\n");
//    	printf("\t(6: Speech recognition)\n");
//    	printf("\t(7: Get DCT values)\n");
    	printf("\n\tValitse numero: ");

    	if(skip == 0){
    		choice = (u_short)getc(stdin);
    		choice = choice - 48;
    		//printf("\n\nValinta: %c(c) %d(d)\n", choice, choice);
    	} else {
    		if(choice == 1 || choice == 2 || choice == 3 || choice == 4 ){
    			// menn‰‰n puheentunnistuksessa tunnistetulla sanalla eteenp‰in
    			skip = 0;		// vain kerran
    		} else {
    			printf("\nPuheentunnistus ep‰onnistui. \n\n\tValitse numero: ");
    			skip = 0;
    			choice = (u_short)getc(stdin);
        		choice = choice - 48;
    		}
    	}

    	//printf("\n\nValinta: %c(c) %d(d)\n", choice, choice);
    	printf("\n\tValinta: %d\n", choice);

    	switch (choice) {
			case 1 :  printf("\tAloitetaan yksinpeli ...\n\n");
						srand(time(NULL)); //satunnaislukugeneraattorin alustus
						resetWholeGame();
						showGameStatus();
						startOnePlayerGame();
    					break;
			case 2 :  printf("\tAloitetaan kaksinpeli ...\n\n");
						srand(time(NULL)); //satunnaislukugeneraattorin alustus
						resetWholeGame();
                        showGameStatus();
                        i = findOpponent();
                        if (i==1)
                        	startTwoPlayerGame();
    					break;
    		case 3 :  // Settings
						settings();
    					break;
    		case 4 :  if (verbose) {
							verbose = 0;
							printf("\n\tVerbose ei toiminnassa.\n\n");
						}
						else {
							verbose = 1;
							printf("\tVerbose toiminnassa.\n\n");
						}
    					break;
			case 5 :	printf("\n\n\tPUHEENTUNNISTUS");
						printf("\n\n\tSanasto: Yksinpeli, Verkkopeli, Asetukset, Tulostus");
						printf("\n\n\tPaina jotain painiketta alottaaksesi.");
						jarru = (u_short)getc(stdin);	//odottaa k‰ynnistyst‰
						puhe = spokenWord(puheenMfcc);
						//printf("\npuhe = %d", puhe);
						//printf("\nchoice = %c(c), %d(d)", choice, choice);
						//printf("\nchoice = %c(c), %d(d)", choice, choice);

						// 1 = yksinpeli, 2 = verkkopeli, 3 = asetukset, 5 = tulostus
						if(puhe==5) puhe=4;  //verbose=tulostus tunnistuksessa 5, valikossa 4
						// 1 = yksinpeli, 2 = verkkopeli, 3 = asetukset, 4 = tulostus
						choice = (u_short)puhe; //-48
						skip = 1;
						break;
/*    		case 7 :
						speechRecognizer(puheenMfcc);
						break;
*/
						/*
    		case 7 :	printf("\n\tPUHEENTUNNISTUS");
						printf("\n\tSanasto: YKSINPELI, VERKKOPELI, ASETUKSET, TULOSTUS");
						printf("\n\n\tPaina jotain painiketta alottaaksesi.");
						jarru = (u_short)getc(stdin);	//odottaa k‰ynnistyst‰
						puhe = spokenWord(puheenMfcc);
						//printf("\npuhe = %d", puhe);
						//printf("\nchoice = %c(c), %d(d)", choice, choice);
						//printf("\nchoice = %c(c), %d(d)", choice, choice);

						// 1 = yksinpeli, 2 = verkkopeli, 3 = asetukset, 5 = tulostus
						if(puhe==5) puhe=4;  //verbose=tulostus tunnistuksessa 5, valikossa 4
						// 1 = yksinpeli, 2 = verkkopeli, 3 = asetukset, 4 = tulostus
						choice = (u_short)puhe; //-48
						skip = 1;
						break;
						*/
    		default  : 	printf("\tTuntematon valinta.");
    					break;
    	}
    }
    return 0;
}

/*! \fn int spokenWord(int puheenMfcc[][NMBROFFILTERS])
 *  \brief Funktio, joka vertaa puhuttua sanaa referenssisanoihin.
 *  \param puheenMfcc Sanotun sanan MFCC-kertoimet
 *  \return tunnistettuLuku Tunnistettu luku.
 */
int spokenWord(int puheenMfcc[][NMBROFFILTERS]){ //static int ** puheenMfcc) {
	u_short i = 0;
	int puheenNoF = 0;
	int dtwValue = BIG_VALUE;
	int dtwValueTemp = 0;
	u_short tunnistettuLuku = 0;
	u_short nayteNro = 0;
	u_short arvot[20] = { 0 };

	puheenNoF = speechRecognizer(puheenMfcc);
	if (puheenNoF == 0) //Poistutaan silmukasta, jos puheentallennus ei onnistunut
		return 0;

	/*** Virpi ********/
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, yksinpeliV1, puheenNoF,
			nofYksinpeliV1);
	arvot[0] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 1;
		nayteNro = 1;
	}

	dtwValueTemp = dynamicTimeWarping(puheenMfcc, yksinpeliV2, puheenNoF,
			nofYksinpeliV2);
	arvot[1] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 1;
		nayteNro = 2;
	}
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, verkkopeliV1, puheenNoF,
			nofVerkkopeliV1);
	arvot[2] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 2;
		nayteNro = 1;
	}
	/*
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, verkkopeliV2, puheenNoF,
			nofVerkkopeliV2);
	arvot[3] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 2;
		nayteNro = 2;
	}
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, verkkopeliV3, puheenNoF,
			nofVerkkopeliV3);
	arvot[4] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 2;
		nayteNro = 3;
	}
	*/
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, asetuksetV1, puheenNoF,
			nofAsetuksetV1);
	arvot[5] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 3;
		nayteNro = 1;
	}
	/*
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, asetuksetV2, puheenNoF,
			nofAsetuksetV2);
	arvot[6] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 3;
		nayteNro = 2;
	}
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, asetuksetV3, puheenNoF,
			nofAsetuksetV3);
	arvot[7] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 3;
		nayteNro = 3;
	}
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, asetuksetV4, puheenNoF,
			nofAsetuksetV4);
	arvot[8] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 3;
		nayteNro = 4;
	}
	*/
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, asetuksetV5, puheenNoF,
			nofAsetuksetV5);
	arvot[9] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 3;
		nayteNro = 5;
	}
	/*
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, moninpeliV1, puheenNoF,
			nofMoninpeliV1);
	arvot[10] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 4;
		nayteNro = 1;
	}
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, moninpeliV2, puheenNoF,
			nofMoninpeliV2);
	arvot[11] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 4;
		nayteNro = 2;

	}*/
	/*
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, tulostusV1, puheenNoF,
			nofTulostusV1);
	arvot[12] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 5;
		nayteNro = 1;

	}
	*/
	dtwValueTemp = dynamicTimeWarping(puheenMfcc, tulostusV2, puheenNoF,
			nofTulostusV2);
	arvot[13] = dtwValueTemp;
	if (dtwValueTemp < dtwValue) {
		dtwValue = dtwValueTemp;
		tunnistettuLuku = 5;
		nayteNro = 2;

	}

	if(verbose) printf("DTWvalue: %d\n", dtwValue);
	if (verbose) {
		printf("Tunnistettu luku: %d, N‰yteNro: %d, arvot: \n",
				tunnistettuLuku, nayteNro);
		for (i = 0; i < 20; i++) {
			printf("%d, ", arvot[i]);
		}
	}
	dtwValue = BIG_VALUE;
	return tunnistettuLuku;
}

/*! \fn void settings(void)
 *  \brief Funktio asetuksille.
 *  \return void.
 */
void settings(void) {
	char choice;

	printf("\n\n\t**** Asetukset ****\n\n");
	printf("\tValitse numero.\n");
	printf("\t1: Vaihda IP osoite\n");
	printf("\t2: Vaihda aliverkon maski\n");
	printf("\tMuut numerot palaavat edelliseen valikkoon.\n");

	choice = getc(stdin);

	switch (choice) {
		case '1' :  changeIP();
					break;
		case '2' : changeSNM();
					break;
		default  :  printf("\tTuntematon valinta.");
					break;
	}
}

/*! \fn void changeIP(void)
 *  \brief Funktio IP-osoitteen vaihtoon.
 *  \return void.
 */
void changeIP(void) {
	char inbuf[16];
	u_long omaIP, oIP;

	printf("\n\n\t****IP valikko****\n");
	printf("\tAnna uusi IP osoite\n");

	scanf("%s", inbuf);
	omaIP = inbuf;
	oIP = inet_addr(omaIP);

	NutNetIfConfig("eth0", confnet.cdn_mac, oIP, confnet.cdn_ip_mask);

	printf("\n\tUusi IP on: %s\n", inet_ntoa(oIP));
}

/*! \fn void changeSNM(void)
 *  \brief Funktio aliverkon peitteen vaihtoon.
 *  \return void.
 */
void changeSNM(void) {
	char choice;
	u_long ip_mask;
	u_short i = 0;

	printf("\n\n\t**** Aliverkon peite ****\n");
	printf("\tValitse numero.\n");
	printf("\t1:Aliverkon peite: 255.255.255.0\n");
	printf("\t2:Aliverkon peite: 255.255.0.0\n");
	printf("\tMuut numerot palaavat edelliseen valikkoon.\n");
	choice = getc(stdin);

	switch (choice) {
	case '1': //TODO: Subnet mask: 255.255.255.0
		ip_mask = inet_addr(LOCALMASK);
		i = 0;
		break;
	case '2': //TODO: Subnet mask: 255.255.0.0
		ip_mask = inet_addr(GLOBALMASK);
		i = 1;
		break;
	default:
		printf("\tTuntematon valinta.");
		break;
	}
	if (i == 0)
		printf("\n\tAliverkon peite: %s\n", LOCALMASK);
	else
		printf("\n\tAliverkon peite: %s\n", GLOBALMASK);

}

