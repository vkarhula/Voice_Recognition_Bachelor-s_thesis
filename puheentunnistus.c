
#include <cfg/os.h>
#include <cfg/clock.h>
#include <dev/board.h>
#include <dev/st7036.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <io.h>

#include <sys/version.h>
#include <sys/confos.h>
#include <sys/confnet.h>
#include <sys/atom.h>
#include <sys/heap.h>
#include <sys/thread.h>
#include <sys/timer.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <fs/typedefs.h>

#include <dev/vscodec.h>

#include "puheentunnistus.h"

#include "config.h"
#include "main.h"

#include "fix_fft.h"
#include "melFilterCoefficients.h"
#include "dct.h"


//hamming[] skaalattu (x10000)
int hamming10000[] = {
800, 801, 806, 813, 822, 835, 850, 868, 889, 913, 939, 968, 1000, 1034, 1071, 1111, 1153, 1198, 1245, 1295, 1347, 1402, 1459, 1519, 1581, 1645, 1712, 1781, 1852, 1925, 2001, 2078, 2157, 2239, 2322, 2407, 2494, 2583, 2673, 2765, 2859, 2954, 3051, 3149, 3249, 3350, 3452, 3555, 3659, 3765, 3871, 3979, 4087, 4196, 4305, 4416, 4527, 4638, 4750, 4863, 4976, 5089, 5202, 5315, 5428, 5542, 5655, 5768, 5881, 5993, 6106, 6217, 6329, 6439, 6549, 6659, 6767, 6875, 6982, 7088, 7193, 7297, 7400, 7501, 7601, 7700, 7797, 7893, 7988, 8081, 8172, 8262, 8350, 8436, 8520, 8602, 8683, 8761, 8837, 8912, 8984, 9054, 9121, 9187, 9250, 9311, 9369, 9426, 9479, 9530, 9579, 9625, 9669, 9710, 9748, 9784, 9817, 9847, 9875, 9899, 9922, 9941, 9958, 9972, 9983, 9991, 9997, 10000, 10000, 9997, 9991, 9983, 9972, 9958, 9941, 9922, 9899, 9875, 9847, 9817, 9784, 9748, 9710, 9669, 9625, 9579, 9530, 9479, 9426, 9369, 9311, 9250, 9187, 9121, 9054, 8984, 8912, 8837, 8761, 8683, 8602, 8520, 8436, 8350, 8262, 8172, 8081, 7988, 7893, 7797, 7700, 7601, 7501, 7400, 7297, 7193, 7088, 6982, 6875, 6767, 6659, 6549, 6439, 6329, 6217, 6106, 5993, 5881, 5768, 5655, 5542, 5428, 5315, 5202, 5089, 4976, 4863, 4750, 4638, 4527, 4416, 4305, 4196, 4087, 3979, 3871, 3765, 3659, 3555, 3452, 3350, 3249, 3149, 3051, 2954, 2859, 2765, 2673, 2583, 2494, 2407, 2322, 2239, 2157, 2078, 2001, 1925, 1852, 1781, 1712, 1645, 1581, 1519, 1459, 1402, 1347, 1295, 1245, 1198, 1153, 1111, 1071, 1034, 1000, 968, 939, 913, 889, 868, 850, 835, 822, 813, 806, 801, 800};

/*! \fn void initAudioDevice(void)
 *  \brief Funktio, joka suorittaa ‰‰nipiirin alustuksen.
 *  \return void.
 */
void initAudioDevice(void) {
	NutLoadConfig();
	if (ConfigInit()) {
		/* No configuration memory, run with factory defaults. */
		ConfigResetFactory();
	} else {
		if (ConfigLoad()) {
			/* No configuration info, use factory defaults. */
			ConfigResetFactory();
			ConfigSave();
		}
	}
	ResetDevice();
}

/*! \fn int speechRecognizer(int mfcc[][NMBROFFILTERS])
 *  \brief Funktio, joka suorittaa puheentunnistuksen.
 *  \param **mfcc Kaksiuloitteinen taulukko, johon talletetaan puhesignaalin MFCC-kertoimet.
 *  \return nmbrOfFrames Palauttaa kehyksien lukum‰‰r‰n puhesignaalissa.
 */
int speechRecognizer(int mfcc[][NMBROFFILTERS]) {
	static short inbuf1[FILESIZE] = {0};	//
	static short inbuf2[FILESIZE] = {0};	//

	int i = 0, j = 0;

	static short puheSignaali[FILESIZE];	//N‰ytteistetty puhe
	int *puheSignaaliInt;	//N‰ytteistetty puhe int-tyyppisen‰
	int *suodPuheSignaali;	//Esisuodatuksessa suodatettu
	int puheenPituus;	//N‰ytteistetyn puheenpituus
	volatile u_short nmbrOfFrames = 0;
	//u_short nmbrOfFrames1 = 0;
	static u_short nmbrOfFrames1 = 0;
	//u_short nmbrOfFrames2 = 0;
	//static u_short nmbrOfFrames2 = 0;
	//u_short nmbrOfFrames2 = nof;		//Kehyksien lkm referenssin‰ytteess‰   //obsolete
	u_short nmbrOfFrames2;
	u_short sampleNro = 1;//0;
	int scale;	//FFT-funktion skaalaus
	short* inbufp;

	static int frames[KEHYKSIENLKM][NAYTTEET];	//frame blocking
	static int hammingFrames[KEHYKSIENLKM][NAYTTEET];	//Hamming-suodatetut kehykset
	static int hammingFramesI[KEHYKSIENLKM][NAYTTEET];	//Hamming-suodatetut kehykset, imagin‰‰riosa
	static short tempFrames[NAYTTEET];	//Apumuuttuja FIR-suodatukseen
	static short tempFramesI[NAYTTEET];	//Apumuuttuja FIR-suodatukseen

	int **melResult = 0;	//Mel-suodatuksen tulos
	int **ln = 0;			//Luonnollisen logaritmin tulos
	static int **mfcc2 = 0;	//Lopulliset MFCC-kertoimet

	//Nollataan tarpeelliset muuttujat
	for(i = 0; i < KEHYKSIENLKM; i++){
		for(j = 0; j < NAYTTEET; j++){
			frames[i][j] = 0;
			hammingFrames[i][j] = 0;
			hammingFramesI[i][j] = 0;
		}
		tempFrames[i] = 0;
		tempFramesI[i] = 0;
	}

	if (verbose)
		printf("\nspeechReconizer(): Available memory: %d\n", (u_int)NutHeapAvailable());

	/*
	 * Nauhoitetaan signaali
	 */
	inbufp = inbuf1;
	recordSpeech(inbufp);

	// if-else on turha?
	if(sampleNro == 1){
		inbufp = inbuf1;
	} else {
		inbufp = inbuf2;
	}

	speechDetector(inbufp, puheSignaali, &puheenPituus);

	if (verbose) {
		/* N‰ytteiden tulostus pois
		for (i = 0; i < puheenPituus; i++) {
			printf("%d ", puheSignaali[i]);//(*inbufp));
		}
		*/
		printf("\nNaytteiden lkm: %d", puheenPituus);
		printf("\nNaytteen pituus: %d ms\n", puheenPituus/8);
		fflush(stdin);
		printf("\n\n");
	}
	/*
	 * Tarkistetaan puheSignaalin oikeellisuus
	 */
	if ( puheenPituus < NAYTTEET) {
		printf("\n*** Recording failed ***\n");
		return 0;
	}

	//Varataan tila puhesignaalille
	puheSignaaliInt = (int*) malloc (4*puheenPituus);
	for (i = 0; i < puheenPituus; i ++ ){
		puheSignaaliInt[i] = 0;
	}
	if (puheSignaaliInt == NULL)
		printf("\npuheSignaaliInt:n varaus ep‰onnistui!");
	for (i = 0; i < puheenPituus; i++){
		puheSignaaliInt[i] = puheSignaali[i];
	}
	//Esisuodatus
	suodPuheSignaali = (int*) malloc (4*puheenPituus);
	for (i = 0; i < puheenPituus; i ++ ){
		suodPuheSignaali[i] = 0;
	}
	preEmphasis(puheSignaaliInt, suodPuheSignaali, puheenPituus);

	// Kehyksiin jako
	nmbrOfFrames = (puheenPituus - 156) / 100;
	// Funktio-kommentti: tarviiko seuraavia?
	if(sampleNro == 1){
		nmbrOfFrames1 = nmbrOfFrames;
	} else {
		nmbrOfFrames2 = nmbrOfFrames;
	}
	frameBlocking(suodPuheSignaali, frames, nmbrOfFrames);
	/* testitulostuksia
	for ( i=0 ; i<nmbrOfFrames ; i++ ) {	//testaus
		for ( j=0 ; j<256 ; j++) {
			//printf("%d ", frames[i][j]);
		}
	}
	testitulostuksia */
	if (verbose)
		printf("\nkehyksi‰: %d \n", i);

	//Hamming-ikkunointi
	hammingWindowing(frames, hammingFrames, nmbrOfFrames);

	//FFT
	for (i=0 ; i<nmbrOfFrames ; i++) {	//Suoritetaan FFT kullekkin kehykselle erikseen

		for ( j=0 ; j<NAYTTEET ; j++) {	//shortiksi FFT:t‰ varten
			tempFrames[j] = hammingFrames[i][j];
			tempFramesI[j] = hammingFramesI[i][j];
		}

		//Varsinainen FFT
		scale = fix_fft( tempFrames, tempFramesI, 8 ,0 );

		for ( j=0 ; j<NAYTTEET ; j++) {	//Takaisin int:ksi + skaalaus
			hammingFrames[i][j] = tempFrames[j];
			hammingFramesI[i][j] = tempFramesI[j];
			hammingFrames[i][j] = hammingFrames[i][j] << scale;
			hammingFramesI[i][j] = hammingFramesI[i][j] << scale;
		}
	}
	//if (verbose)
		//printf("\n\nspeechRecognizer: FFT: Yhdistetty reaali+imaginaariosa:\n");
	for (i=0 ; i<nmbrOfFrames ; i++) {
		for ( j=0 ; j<NAYTTEET ; j++) {
			hammingFrames[i][j] = hammingFrames[i][j] * hammingFrames[i][j] +
								  hammingFramesI[i][j] * hammingFramesI[i][j];
			//if (verbose)
				//printf("%d ", hammingFrames[i][j]);
		}
	}
	//if (verbose)
		//printf("\n");
	//Mel-suodatus
	melResult = (int**) malloc (4*nmbrOfFrames);
	for (i = 0; i < nmbrOfFrames; i++ ){
		melResult[i] = (int*) malloc(4*19);

		for (j=0 ; j<NMBROFFILTERS ; j++) {
			melResult[i][j] = 0;
		}

	}
	//melResult: 19xNMBROFFRAMES (melResult[19][NMBROFFRAMES])
	for (j=0 ; j<nmbrOfFrames ; j++) {
		melFiltering(hammingFrames[j], melResult[j]);
	}

	//Luonnollinen logaritmi
	//ln: nmbrOfFrames*19, ln[freimien lukum‰‰r‰][kertoimien lukum‰‰r‰]
	ln = (int**) malloc (4*nmbrOfFrames);
	for (i = 0; i < nmbrOfFrames; i ++ ){
		ln[i] = (int*) malloc (4*19);
		for (j=0 ; j<NMBROFFILTERS ; j++) {
			ln[i][j] = 0;
		}
	}
	if (verbose)
		printf("\n\nLuonnollinen logaritmi:\n");
	for (i=0 ; i<nmbrOfFrames ; i++) {
		for(j=0 ; j<NMBROFFILTERS ; j++) {
			ln[i][j] = naturalLogarithm(melResult[i][j]);	//testattu, toimii
			if (verbose)
				printf("%d ", ln[i][j]);
		}
	}
	//DCT
	//mfcc: [kehyksien lukum‰‰r‰][suodattimien lukum‰‰r‰]
	// Funktio-kommentti: mit‰ tehd‰‰n sampleNro:lle
	// sample 1
	if(sampleNro == 1){
		if (verbose)
			printf("\n\nSample 1:\n");
		/*mfcc = (int**) malloc (4*nmbrOfFrames);
		//newMfcc = (int**) malloc (4*nmbrOfFrames);
		for (i = 0; i < nmbrOfFrames; i ++ ){
			mfcc[i] = (int*) malloc (4*19);
			for (j=0 ; j<NMBROFFILTERS ; j++) {
				mfcc[i][j] = 0;
			}
		}*/
		if (verbose)
			printf("\n\nDiskreettikosinimuunnos:\n");
		for(i=0 ; i<nmbrOfFrames ; i++) {
			discreteCosineTransform(ln[i], mfcc[i]);
		}
		if (verbose) {
			for (i=0 ; i<nmbrOfFrames ; i++) {
				printf("{");
				for (j=0 ; j<NMBROFFILTERS; j++) {
					//printf("%3d ", mfcc[i][j]);
					if ( j%(NMBROFFILTERS-1) == 0 && j!=0)
						printf("%d", mfcc[i][j]);
					else
						printf("%d, ", mfcc[i][j]);
				}
				printf("},\n");
			}
		}
		printf("\n");
		if (verbose) {
			for (i=0 ; i<nmbrOfFrames ; i++) {
				for (j=0 ; j<NMBROFFILTERS; j++) {
						printf("%d ", mfcc[i][j]);
				}
			}
		}


	// sample 2
	} else {
		if (verbose)
			printf("\n\nSample 2:\n");
		mfcc2 = (int**) malloc (4*nmbrOfFrames);
		for (i = 0; i < nmbrOfFrames; i ++ ){
			mfcc2[i] = (int*) malloc (4*19);
			for (j=0 ; j<NMBROFFILTERS ; j++) {
				mfcc2[i][j] = 0;
			}
		}
		if (verbose)
			printf("\n\nDiskreettikosinimuunnos:\n");
		for(i=0 ; i<nmbrOfFrames ; i++) {
			discreteCosineTransform(ln[i], mfcc2[i]);
		}
		if (verbose) {
			for (i=0 ; i<nmbrOfFrames ; i++) {
				printf("{");
				for (j=0 ; j<NMBROFFILTERS; j++) {
					//printf("%3d ", mfcc2[i][j]);
					if ( j%(NMBROFFILTERS-1) == 0 && j!=0)
						printf("%d", mfcc2[i][j]);
					else
						printf("%d, ", mfcc2[i][j]);
				}
				printf("},\n");
			}
		}
	}
	if (verbose)
		printf("nmbrOfFrames: %d\n", nmbrOfFrames);

	if (verbose)
			printf("\nspeechReconizer(): Available memory: %d\n", (u_int)NutHeapAvailable());

	if (verbose)
		printf("\nspeechReconizer(): Available memory: %d\n", (u_int)NutHeapAvailable());

	return (int)nmbrOfFrames;
}

/*! \fn void recordSpeech(short *inbuffer)
 *  \brief Funktio, joka nauhoittaa signaalin mikrofonilta.
 *  \param *inbuffer Taulukko, johon n‰ytteistetty signaali talletetaan.
 *  \return void.
 */
void recordSpeech(short *inbuffer) {
	VsTest();
	NutSleep(500);
	printf("\nRecording sample 1\n");
	fflush(stdin);
	// pointer to array, size of array, samplerate, bitspersample, mic on, stereo on

	Record(inbuffer, FILESIZE, 8000, 16, TRUE, FALSE);
}

/*! \fn void speechDetector(short *signaali, short *puheSignaali, int *puheenPituus)
 *  \brief Funktio, joka erottaa puhesignaalin taustakohinasta.
 *  \param *signaali N‰ytteistetty signaali.
 *  \param *puheSignaali N‰ytteistetyst‰ signaalista erotettu puhesignaali.
 *  \param *puheenPituus Puhesignaalin n‰ytteiden lukum‰‰r‰.
 *  \return void.
 */
void speechDetector(short *signaali, short *puheSignaali, int *puheenPituus)
{
	int i = 0, j = 0, alku = 0, loppu = 0, laskuri = 0;

	*puheenPituus = 0;

	/* Puheen alkamiskohta */
	for( i = 40 ; i < FILESIZE ; i++){
		if( abs(signaali[i]) > ALARAJA && abs(signaali[i+1]) > ALARAJA){
			alku = i;
			break;
		}
	}
	/* Puheen loppumiskohta */
	for( i = alku ; i < FILESIZE ; i++){
		if( abs(signaali[i]) < KOHINA ) //2000 )//1500 )
			laskuri++;
		if( abs(signaali[i]) > KOHINA ) //2000 )//1500 )
			laskuri = 0;
		if( laskuri == 1500 ){	//oli 1000
			loppu = i - 1499;
			*puheenPituus = loppu - alku;
			break;
		}
	}
	/* Puhesignaali omaan taulukkoon */
	for( i = alku ; i <= loppu ; i++) {
		puheSignaali[j] = signaali[i];
		j++;
	}
}

/* esivahvistus */
// H(z) = 1 - a*z^-1
// => Y(z) = H(z)X(z)
/*! \fn void preEmphasis(int * puheSignaali, int *suodPuheSignaali, u_short lkm)
 *  \brief Funktio, joka suorittaa esisuodatuksen.
 *  \param *puheSignaali N‰ytteistetyst‰ signaalista erotettu puhesignaali.
 *  \param *suodPuheSignaali Suodatettu puheSignaali.
 *  \param lkm Puhesignaalin n‰ytteiden lukum‰‰r‰.
 *  \return void.
 */
void preEmphasis(int * puheSignaali, int *suodPuheSignaali, u_short lkm) {
	u_short i;
	// ensimm‰inen n‰yte
	*(suodPuheSignaali) = *(puheSignaali);	// ensimm‰iselle n‰ytteelle x(n-1) = 0
	for(i = 1; i < lkm; i++){
		// y(n) = x(n) - 0.95 * x(n-1)
		// kertolasku kokonaisluvuilla
		suodPuheSignaali[i] = (1000 * puheSignaali[i] - 950 * puheSignaali[i - 1])/1000;
	}
}

/* Frame Blocking-funktio */
/*
 * Kehyksien lukum‰‰r‰n laskenta: (n‰ytteiden lkm - 156) / 100
 * 							eli	  (n‰ytteiden lkm - N-M) / M
 */
/*! \fn void frameBlocking(int* filtered, int frames[][NAYTTEET], u_short nmbrOfFrames)
 *  \brief Funktio, joka jakaa puhesignaalin kehyksiin.
 *  \param *filtered Suodatettu puhesignaali.
 *  \param **frames Kehykset.
 *  \param nmbrOfFrames Kehyksien lukum‰‰r‰.
 *  \return void.
 */
void frameBlocking(int* filtered, int frames[][NAYTTEET], u_short nmbrOfFrames)
{
	u_short m = 0;
	u_short separator = 100;	//kehysten v‰linen et‰isyys
	u_short i, j;

	for (i=0 ; i<nmbrOfFrames ; i++) {
		for (j=0 ; j<256 ; j++) {
			// i: kehyksen numero, j: n‰ytteen numero kehyksess‰ i
			frames[i][j] = filtered[m + j];
		}
		m += separator;
	}
}

/*! \fn void hammingWindowing(int frameIn[][NAYTTEET], int frameOut[][NAYTTEET], u_short nmbrOfFrames)
 *  \brief Funktio, joka suorittaa Hamming-ikkunoinnin.
 *  \param **frameIn Ikkunoitava kehys.
 *  \param **frameOut Ikkunoitu kehys.
 *  \param nmbrOfFrames Kehyksien lukum‰‰r‰.
 *  \return void.
 */
void hammingWindowing(int frameIn[][NAYTTEET], int frameOut[][NAYTTEET], u_short nmbrOfFrames){
	u_short i = 0, j = 0;
	for(i = 0; i < nmbrOfFrames; i ++){
		for(j = 0; j < NAYTTEET; j++){
			frameOut[i][j] = (hamming10000[j] * frameIn[i][j]) / 10000;	//Takaisin skaalaus
		}
	}
}

/*
 * Mel-suodatus
 *
 * Suodattaa 256-pisteen FFT:n tuloksen Mel-suodatin pankilla.
 * Symmetrian vuoksi riitt‰‰ ensimm‰isen 128 FFT:n tuloksen k‰sittely.
 */
/*! \fn void melFiltering(int signal[], int result[])
 *  \brief Funktio, joka suorittaa 256-pisteen FFT:n tuloksen Mel-suodatin pankilla.
 *  	   Symmetrian vuoksi riitt‰‰ ensimm‰isen 128 FFT:n tuloksen k‰sittely.
 *  \param *signal Suodatettava signaali.
 *  \param *result Suodatettu signaali.
 *  \return void.
 */
void melFiltering(int signal[], int result[]){
	u_short i, j;

	//result:n alustus
	for (i=0 ; i<19 ; i++) {
		result[i] = 0;
	}
	//Signaalin skaalaus ( /1000 )
	//Takaisin skaalausta ei tarvita, koska Mel-suodattimien kertoimia on skaalattu 1000x
	for (i=0 ; i<128 ; i++) {
		signal[i] = signal[i] / 1000;
	}
	//Varsinainen Mel-suodatus
	for (i=0 ; i<NMBROFFILTERS ; i++) {
		for (j=0 ; j<128 ; j++) {//TODO: T‰m‰n voisi viel‰ tarkistaa
			//T‰nne ei takaisin skaalausta
			result[i] = result[i] + (signal[j] * melFilters[i][j]);
		}
	}
}

/*
 * Luonnollinen logaritmi
 *
 * Laskee luonnollisen logaritmin k‰ytt‰en hakutaulukkoa.
 * "Hakutaulukon" arvot laskettu kaavalla (e^x.5)-1
 */
/*! \fn int naturalLogarithm(int input)
 *  \brief Funktio, joka laskee luonnollisen logaritmin k‰ytt‰en hakutaulukkoa.
 *  \param input Luku, josta otetaan logaritmi.
 *  \return result Luonnollinen logaritmi.
 */
int naturalLogarithm(int input) {
	int result;

	//if (input > 2174400000) result = 22;	//max(signed int == 2147483647)
	if (input > 799900000) result = 21;		//max(signed int == 2147483647)
	else if (input > 294270000) result = 20;
	else if (input > 108250000) result = 19;
	else if (input > 39825000) result = 18;
	else if (input > 14651000) result = 17;
	else if (input > 5389700) result = 16;
	else if (input > 1982800) result = 15;
	//T‰st‰ ylˆsp‰in v‰hiten merkitseviss‰ numeroissa on heittoa. Matlabin tarkkuus loppui kesken...
	//T‰st‰ alasp‰in arvot tarkkoja
	else if (input > 729416) result = 14;
	else if (input > 268337) result = 13;
	else if (input > 98715) result = 12;
	else if (input > 36315) result = 11;
	else if (input > 13360) result = 10;
	else if (input > 4914) result = 9;
	else if (input > 1808) result = 8;
	else if (input > 665) result = 7;
	else if (input > 244) result = 6;
	else if (input > 90) result = 5;
	else if (input > 33) result = 4;
	else if (input > 12) result = 3;
	else if (input > 4) result = 2;
	else if (input > 1) result = 1;
	else result = 0;

	return result;
}

/*
 * Diskreetti kosini muunnos
 * DCT: sqrt(2/M)*sum1..M( Xm * cos(pi*k*(m-0,5)/M) )
 * sqrt(2/19) = 0,3244 = 3244/10000
 */
/*! \fn void discreteCosineTransform(int input[], int output[])
 *  \brief Funktio, joka suorittaa diskreetin kosini muunnoksen.
 *  \param *input input taulukko.
 *  \param *output output taulukko.
 *  \return void.
 */
void discreteCosineTransform(int input[], int output[]) {
	u_short i, j;

	//output:n alustus
	for (i=0 ; i<19 ; i++) {
		output[i] = 0;
	}
	for (i=0 ; i<19 ; i++) {
		for (j=0 ; j<19 ; j++) {
			output[i] = output[i] + (input[j]*dctCosine10000[i][j])/10000;	//Takaisin skaalaus
		}
	}
	for (i=0 ; i<19 ; i++) {
		output[i] = (output[i]*3244)/10000;
	}
}

/*! \fn int dynamicTimeWarping(int mfcc[][NMBROFFILTERS], int mfcc2[][NMBROFFILTERS], int nmbrOfFrames1, int nmbrOfFrames2)
 *  \brief Funktio, joka suorittaa dynaamisen ajansovituksen.
 *  \param **mfcc Ensimm‰isen sanan MFCC-kertoimet.
 *  \param **mfcc2 Toisen sanan MFCC-kertoimet.
 *  \param nmbrOfFrames1 Ensimm‰isen sanan kehyksien lukum‰‰r‰.
 *  \param nmbrOfFrames2 Toisen sanan kehyksien lukum‰‰r‰.
 *  \return dtw[][] Yksi DTW-taulukon alkio.
 */
int dynamicTimeWarping(int mfcc[][NMBROFFILTERS], int mfcc2[][NMBROFFILTERS], int nmbrOfFrames1, int nmbrOfFrames2){

	int **mfcc_lyhyt = 0;
    int **mfcc_pitka = 0;
    int **dtw = 0;  //DTW-matriisi, sarakkeiden lkm=nmbrOfFrames1, rivien lkm= nmbrOfFrames
    int *x = 0;	//taulukko DTW laskettavien alkioiden x-koordinaateille (i)
    int *y = 0;	//taulukko DTW laskettavien alkioiden y-koordinaateille (j)
    int *x1 = 0;
    int *y1 = 0;

    int nmbrOfFrames_x = 0, nmbrOfFrames_y = 0;
    int d = 0;	//cost function d(x,y) -> dtw
	int i = 0, j = 0, k = 0, ind = 0, ind2 = 0;
	int D1 = 0, D2 = 0, D3 = 0;
	int minD = 0, D = 0;
	int xy_lkm = 0;	// laskettavien koordinaattien lkm (x,y)
	int a = 0;	//apuindeksi

	// pidempi puhesignaali x-akselille (= sarakkeiden lkm)
	if(nmbrOfFrames1 >nmbrOfFrames2){
		nmbrOfFrames_x = nmbrOfFrames1;
		nmbrOfFrames_y = nmbrOfFrames2;

		mfcc_pitka = (int**) malloc (4*nmbrOfFrames_x);
		for (i=0 ; i<nmbrOfFrames_x ; i++) {
			mfcc_pitka[i] = (int*) malloc (4*NMBROFFILTERS);
			for (j=0 ; j<NMBROFFILTERS ; j++) {
				mfcc_pitka[i][j] = 0;
			}
		}
		mfcc_lyhyt = (int**) malloc (4*nmbrOfFrames_y);
		for (i=0 ; i<nmbrOfFrames_y ; i++) {
			mfcc_lyhyt[i] = (int*) malloc (4*NMBROFFILTERS);
			for (j=0 ; j<NMBROFFILTERS ; j++) {
				mfcc_lyhyt[i][j] = 0;
			}
		}
		for(i = 0; i < nmbrOfFrames_x; i ++){
			for(j = 0; j < NMBROFFILTERS; j++){
				mfcc_pitka[i][j] = mfcc[i][j];
			}
		}
		for(i = 0; i < nmbrOfFrames_y; i ++){
			for(j = 0; j < NMBROFFILTERS; j++){
				mfcc_lyhyt[i][j] = mfcc2[i][j];
			}
		}
	} else {
		nmbrOfFrames_x = nmbrOfFrames2;
		nmbrOfFrames_y = nmbrOfFrames1;

		mfcc_pitka = (int**) malloc (4*nmbrOfFrames_x);
		for (i=0 ; i<nmbrOfFrames_x ; i++) {
			mfcc_pitka[i] = (int*) malloc (4*NMBROFFILTERS);
			for (j=0 ; j<NMBROFFILTERS ; j++) {
				mfcc_pitka[i][j] = 0;
			}
		}
		mfcc_lyhyt = (int**) malloc (4*nmbrOfFrames_y);
		for (i=0 ; i<nmbrOfFrames_y ; i++) {
			mfcc_lyhyt[i] = (int*) malloc (4*NMBROFFILTERS);
			for (j=0 ; j<NMBROFFILTERS ; j++) {
				mfcc_lyhyt[i][j] = 0;
			}
		}

		for(i = 0; i < nmbrOfFrames_x; i ++){
			for(j = 0; j < NMBROFFILTERS; j++){
				mfcc_pitka[i][j] = mfcc2[i][j];
			}
		}
		for(i = 0; i < nmbrOfFrames_y; i ++){
			for(j = 0; j < NMBROFFILTERS; j++){
				mfcc_lyhyt[i][j] = mfcc[i][j];
			}
		}
	}

	dtw = malloc (nmbrOfFrames_y * sizeof(int *));
	if(dtw == NULL){
		printf("\n\nERROR: dtw muistinvaraus ep‰onnistui\n");
		return 0;
	}
	for (i = 0; i < nmbrOfFrames_y; i ++ ){
		dtw[i] = malloc (nmbrOfFrames_x * sizeof(int));
		if (dtw[i]==NULL){
			printf("\n\nERROR: dtw muistinvaraus ep‰onnistui\n");
			return 0;
		}
	}
	//alustus
	for (i = 0; i < nmbrOfFrames_y; i ++ ){
		for (j=0 ; j<nmbrOfFrames_x ; j++) {
			dtw[i][j] = 0; //BIG_VALUE;
		}
	}



	x = malloc((nmbrOfFrames1 + nmbrOfFrames2) * sizeof(int));
	y = malloc((nmbrOfFrames1 + nmbrOfFrames2) * sizeof(int));
	x1 = malloc((nmbrOfFrames1 + nmbrOfFrames2) * sizeof(int));
	y1 = malloc((nmbrOfFrames1 + nmbrOfFrames2) * sizeof(int));
	for (i = 0; i < nmbrOfFrames_x + nmbrOfFrames_y; i ++ ){
		x[i] = 0;
		y[i] = 0;
		x1[i] = 0;
		y1[i] = 0;
	}
	if (verbose)
		printf("\n\nMuistivaraukset tehty\n");

	//cost function d(x,y)
	//mfcc[nmbrOfFrames][suodattimien lkm]
	// d = summa(sqr&()2: (Sample1_Frame[i]:1-4 n‰ytett‰ * Sample2_Frame[i]:1-4 n‰ytett‰))

	i = 0;
	j = 0;

	// alkukoordinaatti (i,j) = (0,0)
	x[0] = 0;
	y[0] = 0;
	xy_lkm = 1;

	for(i = 0; i < nmbrOfFrames_x + nmbrOfFrames_y - 1; i++){
		ind2 = 0;
		// k‰yd‰‰n l‰pi laskettavat koordinaatit
		//jos x = y = 99999, ei k‰ytˆss‰, muutoin on aktiivinen
		for(ind = 0; ind < xy_lkm; ind++) { //(nmbrOfFrames1 + nmbrOfFrames2); ind++){

			// kustannusfunktio d(x,y)
			d = 0;
			for(k = 0; k < 12; k++){  //nelj‰, eiku 12 ensimm‰ist‰ DCT kerrointa
				d += abs(mfcc_pitka[x[ind]][k] - mfcc_lyhyt[y[ind]][k]);
			}
			// minD(edellinen polun arvo)
			D1 = BIG_VALUE;
			D2 = BIG_VALUE;
			D3 = BIG_VALUE;
			minD = BIG_VALUE;
			if(x[ind]-1 >= 0) {
				D1 = dtw[y[ind]][x[ind]-1];   // D1 = dtw[i-1][j]
			}
			if((x[ind]-1 >= 0) && (y[ind]-1 >= 0)) {
				D2 = dtw[y[ind]-1][x[ind]-1];	//D2 = dtw[i-1][j-1]
			}
			if(y[ind]-1 >= 0) {
				D3 = dtw[y[ind]-1][x[ind]];		//D3 = dtw[i][j-1]
			}
			//printf("\nD1: %d, D2: %d, D3: %d", D1,D2,D3);

			if(minD > D1) minD = D1;
			if(minD > D2) minD = D2;
			if(minD > D3) minD = D3;
			if(minD == BIG_VALUE) minD = 0;  //tarvitaan, kun i=j=0
			D = d + minD;
			//printf("\nx[%d]=%d, y[%d]=%d, ind= %d, d=%d, D=%d ",ind, x[ind],ind, y[ind], ind, d, D);

			// kirjoitetaan arvo dtw-taulukkoon
			dtw[y[ind]][x[ind]] = D;//oli y[ind]

			//seuraavan kierroksen koordinaatit
			// y-koordinaattia kasvatetaan yhdell‰ nykyisill‰ koordinaateilla (i, j+1)
			// jos y == 0, otetaan mukaan uusi alkio oikealta, kohdasta (i+1,j)
			// max arvom‰‰r‰ = pidemm‰n puhesignaalin framien m‰‰r‰ -> ylim‰‰r‰isi‰: (nmbrOfFrames1 + nmbrOfFrames2)
			if(y[ind] < (nmbrOfFrames_y - 1)){
				x1[ind2] = x[ind]; 		//i;	//ylˆs
				y1[ind2++] = y[ind] + 1;	//j+1;
			}
			// uuden laskentakoordinaatin poimiminen oikealta
			if(y[ind] == 0){
				if(x[ind] < (nmbrOfFrames_x - 1)){
					x1[ind2] = x[ind] + 1;
					y1[ind2++] = 0; 	// j = 0	//y[ind++] = j;
				}
			}
		} //for

		//printf("\nnmbrOfFrames_x=%d, nmbrOfFrames_y=%d", nmbrOfFrames_x,nmbrOfFrames_y);
		//printf("\nuudet koordinaatit: ");
		for(a = 0; a < ind2; a++){ //ind2-1
			x[a] = x1[a];
			y[a] = y1[a];
			//printf(" (%d,%d)",x1[a], y1[a]);
		}
		xy_lkm = ind2;

	} //for

	if(verbose) printf("\n\nDTW polun pienin kustannusfunktio: %d\n", dtw[nmbrOfFrames_y-1][nmbrOfFrames_x-1]);
	NutSleep(1000);

	if (verbose) {
		for (i = 0; i < nmbrOfFrames_y; i++ ){
			printf("\n");
			for (j=0 ; j<nmbrOfFrames_x ; j++) {
				printf("%4d ", dtw[i][j]);
			}
		}
		printf("\n");
	}
	return dtw[nmbrOfFrames_y-1][nmbrOfFrames_x-1];
}



