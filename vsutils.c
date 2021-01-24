/*!
 * $Id$
 */
#include <stdlib.h>
#include <string.h>
#include <dev/vscodec.h>
#include <fs/typedefs.h>
//#include "recordtestapp.h"

/* DataFlash at PA11_SPI0_NPCS0_A */
#define VS_DREQ_BIT     PA30_IRQ1_A         /* PA30 */
#define VS_XCS_BIT      PA31_SPI0_NPCS1_A   /* PA31 */
#define VS_XDCS_BIT     PB30_SPI0_NPCS2_A   /* PB30 */
#define VS_XRESET_BIT   31                  /* PB31 */

/*
 * Instruction opcodes.
 */
#define VS_OPCODE_READ  3
#define VS_OPCODE_WRITE 2

#define VS_MODE_REG         0
#define VS_STATUS_REG       1

#define SCI_MODE	0x0000
#define SM_SDIORD			0	/* 1 = MSb first, 2 = MSb last */

#define VS10XX_DREQ_TST()    ((inr(PIOA_PDSR) & _BV(VS_DREQ_BIT)) == _BV(VS_DREQ_BIT))

#define VS_SM_CANCEL        0x0008

#define AUTOGAINAMP			1024//65535//1x:1024		oli:65535
/*
 * 1024: maksimia ja minimi‰
 */
#define RECORDINGGAIN		500//450
/*
 * RECORDINGGAIN
 * 0: automaattinen
 * 1: pelkk‰‰ minimi‰
 * 5: minimi ja maksimi vuorottelevat
 * 10: maksimiarvoja ??!!
 * 20: maksimiarvoja
 * 100: maksimiarvoja
 *
 */
#define JOINTSTEREO			0x00
#define MONOLEFTCH			0x02
#define MONORIGHTCH			0x03
#define	LINPCM				0x04
#define	ADPCM				0x00
#define WAVHEADERSIZE		44		//in bytes

//static unsigned char wavheader[44] = { /* RIFF WAV header */
//    0x52,0x49,0x46,0x46,	/* ChunkID = "RIFF" */
//    0xFF,0xFF,0xFF,0xFF,	/* ChunkSize */
//    0x57,0x41,0x56,0x45,	/* Format = "WAVE" */
//    0x66,0x6D,0x74,0x20,	/* SubChunk1ID = "fmt " */
//    0x10,0x00,0x00,0x00,	/* SubChunk1Size = 16 */
//    0x01,0x00,				/* AudioFormat: 1 = Linear PCM, 2 = ADPCM */
//    0x02,0x00,				/* NumOfChannels: 1 = Mono, 2 = Stereo */
//    0x40,0x1F,0x00,0x00,	/* Sample rate = 8000 (0x1F 0x40) */
//    0x00,0x7D,0x00,0x00,	/* ByteRate = (2ch * 16bit / 8) * 8000Hz = 32000 (0x7D 0x00) */
//    0x04,0x00,				/* BlockAlign: 2 = Mono, 4 = Stereo 16bit */
//    0x10,0x00,				/* BitsPerSample = 16 */
//    0x64,0x61,0x74,0x61,	/* SubChunk2ID = "data" */
//    0xFF,0xFF,0xFF,0xFF		/* SubChunk2Size: decoding is continued until it is cancelled */
//};


static unsigned char SciByte(unsigned char val)
{
#if 0
    putchar('[');
    if (val != 0xFF) {
        printf("s%02X", val);
    }
#endif

    /* Transmission is started by writing the transmit data. */
    outr(SPI0_TDR, val);
    /* Wait for receiver data register full. */
    while((inr(SPI0_SR) & SPI_RDRF) == 0);
    /* Read data. */
    val = (unsigned char)inr(SPI0_RDR);

#if 0
    if (val != 0xFF) {
        printf("r%02X", val);
    }
    putchar(']');
#endif
    return val;
}


/* Low level data write command */
static unsigned char SpiByte(unsigned char val)
{
    /* Transmission is started by writing the transmit data. */
    outr(SPI0_TDR, val);
    /* Wait for receiver data register full. */
    while((inr(SPI0_SR) & SPI_RDRF) == 0);
    /* Read data. */
    val = (unsigned char)inr(SPI0_RDR);

    return val;
}


/* Waits until audio chip is ready */
static int VsWaitReady(void)
{
    int tmo;

    for (tmo = 0; tmo < 5000; tmo++) {
        if (VS10XX_DREQ_TST()) {
            return 0;
        }
    }
    return -1;
}


/* Write data to audio chip */
static int VsSdiWrite(CONST unsigned char * data, size_t len)
{
    while (len--) {
        if (!VS10XX_DREQ_TST() && VsWaitReady()) {
            return -1;
        }
        SpiByte(*data);
        data++;
    }
    return 0;
}

/* Write data from program memory to audio chip */
static int VsSdiWrite_P(PGM_P data, size_t len)
{
    while (len--) {
        if (!VS10XX_DREQ_TST() && VsWaitReady()) {
            return -1;
        }
        SpiByte(PRG_RDB(data));
        data++;
    }
    return 0;
}


void SdiDeselect(void)
{
    outr(PIOB_SODR, _BV(VS_XDCS_BIT));
}


void SdiSelect(void)
{
    outr(PIOB_CODR, _BV(VS_XDCS_BIT));
}


void SciDeselect(void)
{
    outr(PIOA_SODR, _BV(VS_XCS_BIT));
}


void SciSelect(void)
{
    outr(PIOA_CODR, _BV(VS_XCS_BIT));
}


/* Changes between command and data interfaces */
static void VsSciSelect(unsigned int on)
{
    if (on) {
        /* Disable data interface. */
        SdiDeselect();
        /* Enable cmd interface. */
        SciSelect();
    }
    else {
        /* Disable cmd interface. */
        SciDeselect();
        /* Enable data interface. */
        SdiSelect();
    }
}


/* Read from audio chip register */
static unsigned short VsRegRead(unsigned int reg)
{
    unsigned short data;

    /* Select command channel. */
    VsWaitReady();
    VsSciSelect(1);

    SciByte(VS_OPCODE_READ);
    SciByte((unsigned char) reg);
    data = (unsigned short)SciByte(0) << 8;
    data |= SciByte(0);

    /* Select data channel. */
    VsSciSelect(0);

    return data;
}


/* Write into audio chip register */
void VsRegWrite(unsigned int reg, unsigned short data)
{
    VsWaitReady();
    VsSciSelect(1);

    SciByte(VS_OPCODE_WRITE);
    SciByte((unsigned char) reg);
    SciByte((unsigned short) (data >> 8));
    SciByte((unsigned short) data);

    VsSciSelect(0);
}

/* Hardware support is not yet complete in nut/os, elektor team devised
 * this initialization routine */
static void SpiInit(void)
{
    /* Enable peripherals. */
    outr(PIOA_ASR, inr(PIOA_ASR)
        | _BV(PA12_SPI0_MISO_A)
        | _BV(PA13_SPI0_MOSI_A)
        | _BV(PA14_SPI0_SPCK_A));
        //| _BV(PA31_SPI0_NPCS1_A);
        //| _BV(PA30_IRQ1_A);

    /* Disable PIO lines used for SPI. */
    outr(PIOA_PDR, _BV(PA12_SPI0_MISO_A)
        | _BV(PA13_SPI0_MOSI_A)
        | _BV(PA14_SPI0_SPCK_A));
        //| _BV(PA31_SPI0_NPCS1_A);
        //| _BV(PA30_IRQ1_A);

    /* Chip selects are manually controlled. */
    /* Enable XCS control. */
    outr(PIOA_PER, _BV(VS_XCS_BIT));
    outr(PIOA_SODR, _BV(VS_XCS_BIT));
    outr(PIOA_OER, _BV(VS_XCS_BIT));

    /* Enable XDCS control. */
    outr(PIOB_PER, _BV(VS_XDCS_BIT));
    outr(PIOB_SODR, _BV(VS_XDCS_BIT));
    outr(PIOB_OER, _BV(VS_XDCS_BIT));

    /* Release reset line. */
    outr(PIOB_PER, _BV(VS_XRESET_BIT));
    outr(PIOB_SODR, _BV(VS_XRESET_BIT));
    outr(PIOB_OER, _BV(VS_XRESET_BIT));

    /* Enable SPI clock. */
    outr(PMC_PCER, _BV(SPI0_ID));

    /* SPI enable and reset. */
    outr(SPI0_CR, SPI_SPIEN | SPI_SWRST);
    outr(SPI0_CR, SPI_SPIEN);

    /* Set SPI to master mode, fixed peripheral, fault detection disabled. */
    outr(SPI0_MR, (1 << SPI_PCS_LSB) | SPI_MODFDIS | SPI_MSTR);

    /* Data changes during clock low and will be sampled on rising edges. */
    outr(SPI0_CSR1, (50 << SPI_SCBR_LSB) | SPI_NCPHA);
    outr(SPI0_CSR2, (50 << SPI_SCBR_LSB) | SPI_NCPHA);
}


int Vs10x3Check(void)
{
    unsigned short mode_reg;
    unsigned short stat_reg;

    //printf("Detect VS10X3...");
    SpiInit();
    mode_reg = VsRegRead(VS_MODE_REG);
    if (mode_reg != 0x4800) {
        //puts("Error");
        //printf("VS Mode = %04X\n", mode_reg);
        //return -1;
    }
    stat_reg = VsRegRead(VS_STATUS_REG);
    if ((stat_reg & 0xfff0) != 0x0040) {
        //puts("Error");
        //printf("VS Status = %04X\n", stat_reg);
        //return -1;
    }
    //puts("OK");

    return 0;
}



void Play(unsigned char* Data, unsigned int FileLen)
{
	unsigned short mode_reg;
/******************	MAUNIN KYNIMƒ VERSIO *************************************/
	mode_reg = VS_MODE_REG | VS_SM_RESET | SM_SDIORD | VS_SM_SDINEW;
	VsRegWrite(SCI_MODE, mode_reg);
	VsRegWrite(VS_VOL_REG, 0x0F0F);
	VsSdiWrite(Data, FileLen);
}


/*
 * Initialzizes audio chip
 * Samplerate
 * MicOn:		TRUE if microphone is used, FALSE for line in
 * Stereon:		TRUE for stereo, FALSE for mono
 * */
int RecordDeviceInit(unsigned short SampleRate, int MicOn, int StereoOn)
{
	unsigned short aictrl0 = SampleRate; 	/* sample rate */
	unsigned short aictrl1 = RECORDINGGAIN; /* recording gain, 0 = automatic */
	unsigned short aictrl2 = AUTOGAINAMP; 	/* maximum autogain amplification 1024 = 1x */
	/*
	SCI_AICTRL3
	1..0 	0 = joint stereo (common AGC), 1 = dual channel (separate AGC),
			2 = left channel, 3 = right channel
	2 		0 = IMA ADPCM mode, 1 = LINEAR PCM mode
	15..3 	reserved, set to 0
	*/
	unsigned short aictrl3 = 0x06;//0;				/* stereo/mono, adpcm/linpcm */
	unsigned short mode_reg;

	mode_reg = VsRegRead(SCI_MODE);
	mode_reg = VS_SM_RESET | VS_SM_ADPCM;	/* here adpcm only means recording, selecting
	 										 * between adpcm and linear pcm is done later */

//	if (MicOn == FALSE) mode_reg = mode_reg | VS_SM_LINE_IN;

//	if (StereoOn == TRUE) aictrl3 = aictrl3 | JOINTSTEREO;
/*	else aictrl3 = aictrl3 | MONOLEFTCH;*/

//	aictrl3 = aictrl3 | LINPCM;

	VsRegWrite(VS_AICTRL0_REG, aictrl0);
	VsRegWrite(VS_AICTRL1_REG, aictrl1); /* 0 = automatic autogain control */
	VsRegWrite(VS_AICTRL2_REG, aictrl2);
	VsRegWrite(VS_AICTRL3_REG, aictrl3); /* joint stereo, LINEAR PCM */
	//VsRegWrite(VS_AICTRL3_REG, 0); /* joint stereo, IMA ADPCM */

	VsRegWrite(SCI_MODE, mode_reg);

	/* patch  for vs1053b */
	VsRegWrite(VS_WRAMADDR_REG, 0x8010);
	VsRegWrite(VS_WRAM_REG, 0x3e12);
	VsRegWrite(VS_WRAM_REG, 0xb817);
	VsRegWrite(VS_WRAM_REG, 0x3e14);
	VsRegWrite(VS_WRAM_REG, 0xf812);
	VsRegWrite(VS_WRAM_REG, 0x3e01);

	VsRegWrite(VS_WRAM_REG, 0xb811);
	VsRegWrite(VS_WRAM_REG, 0x0007);
	VsRegWrite(VS_WRAM_REG, 0x9717);
	VsRegWrite(VS_WRAM_REG, 0x0020);
	VsRegWrite(VS_WRAM_REG, 0xffd2);

	VsRegWrite(VS_WRAM_REG, 0x0030);
	VsRegWrite(VS_WRAM_REG, 0x11d1);
	VsRegWrite(VS_WRAM_REG, 0x3111);
	VsRegWrite(VS_WRAM_REG, 0x8024);
	VsRegWrite(VS_WRAM_REG, 0x3704);

	VsRegWrite(VS_WRAM_REG, 0xc024);
	VsRegWrite(VS_WRAM_REG, 0x3b81);
	VsRegWrite(VS_WRAM_REG, 0x8024);
	VsRegWrite(VS_WRAM_REG, 0x3101);
	VsRegWrite(VS_WRAM_REG, 0x8024);

	VsRegWrite(VS_WRAM_REG, 0x3b81);
	VsRegWrite(VS_WRAM_REG, 0x8024);
	VsRegWrite(VS_WRAM_REG, 0x3f04);
	VsRegWrite(VS_WRAM_REG, 0xc024);
	VsRegWrite(VS_WRAM_REG, 0x2808);

	VsRegWrite(VS_WRAM_REG, 0x4800);
	VsRegWrite(VS_WRAM_REG, 0x36f1);
	VsRegWrite(VS_WRAM_REG, 0x9811);
	VsRegWrite(VS_WRAMADDR_REG, 0x8028);
	VsRegWrite(VS_WRAM_REG, 0x2a00);

	VsRegWrite(VS_WRAM_REG, 0x040e);
	/* patch end */
	//NutSleep(1000);
	return 1;
}


/*
 * Writes header, must be called for the same file that audio is to recorded before
 * recording begins.
 * Be careful not to overwrite this data.
 */
void RecordFileInit(unsigned char* File, unsigned int SampleRate, unsigned int BitsPerSample, int StereoOn)
{
	// Example wave header
	// static unsigned char wavheader[44] = { /* RIFF WAV header */
	//    0x52,0x49,0x46,0x46,	/* ChunkID = "RIFF" */
	//    0xFF,0xFF,0xFF,0xFF,	/* ChunkSize */
	//    0x57,0x41,0x56,0x45,	/* Format = "WAVE" */
	//    0x66,0x6D,0x74,0x20,	/* SubChunk1ID = "fmt " */
	//    0x10,0x00,0x00,0x00,	/* SubChunk1Size = 16 */
	//    0x01,0x00,				/* AudioFormat: 1 = Linear PCM, 2 = ADPCM */
	//    0x02,0x00,				/* NumOfChannels: 1 = Mono, 2 = Stereo */
	//    0x40,0x1F,0x00,0x00,	/* Sample rate = 8000 (0x1F 0x40) */
	//    0x00,0x7D,0x00,0x00,	/* ByteRate = (2ch * 16bit / 8) * 8000Hz = 32000 (0x7D 0x00) */
	//    0x04,0x00,				/* BlockAlign: 2 = Mono, 4 = Stereo 16bit */
	//    0x10,0x00,				/* BitsPerSample = 16 */
	//    0x64,0x61,0x74,0x61,	/* SubChunk2ID = "data" */
	//    0xFF,0xFF,0xFF,0xFF		/* SubChunk2Size: decoding is continued until it is cancelled */
	//};

	unsigned int byterate = 0;
	unsigned int i;

	static unsigned char wavheaderbegin[22] = {
	    0x52,0x49,0x46,0x46,	/* ChunkID = "RIFF" */
	    0xFF,0xFF,0xFF,0xFF,	/* ChunkSize */
	    0x57,0x41,0x56,0x45,	/* Format = "WAVE" */
	    0x66,0x6D,0x74,0x20,	/* SubChunk1ID = "fmt " */
	    0x10,0x00,0x00,0x00,	/* SubChunk1Size = 16 */
	    0x01,0x00				/* AudioFormat: 1 = Linear PCM, 2 = ADPCM */
	};

	byterate = SampleRate;
	if (BitsPerSample == 16) byterate = byterate * 2;
	if (StereoOn == TRUE) byterate = byterate * 2;

	// common header data
	for (i = 0; i < sizeof(wavheaderbegin); i++){
		(*File) = wavheaderbegin[i];
		File++;
	}

	// number of channels
	if (StereoOn == TRUE) (*File) = 0x02;
	else (*File) = 0x01;
	File++;
	(*File) = 0x00;
	File++;

	unsigned long integer32 = 0;
	// Sample rate
	integer32 = (unsigned long)SampleRate;
	(*File) = integer32;
	File++;
	(*File) = (integer32 >> 8);
	File++;
	(*File) = (integer32 >> 16);
	File++;
	(*File) = (integer32 >> 24);
	File++;

	// ByteRate
	integer32 = (unsigned long)byterate;
	(*File) = integer32;
	File++;
	(*File) = (integer32 >> 8);
	File++;
	(*File) = (integer32 >> 16);
	File++;
	(*File) = (integer32 >> 24);
	File++;

	// BlockAlign
	if (StereoOn == TRUE) (*File) = 0x04;
	else (*File) = 0x02;
	File++;
	(*File) = 0x00;
	File++;

	// BitsPerSample
	if (BitsPerSample == 16) (*File) = 0x10;
	else (*File) = 0x08;
	File++;
	(*File) = 0x00;
	File++;

	// SubChunk2ID
	(*File) = 0x64;
	File++;
	(*File) = 0x61;
	File++;
	(*File) = 0x74;
	File++;
	(*File) = 0x61;
	File++;

	// SubChunk2Size
	(*File) = 0xFF;
	File++;
	(*File) = 0xFF;
	File++;
	(*File) = 0xFF;
	File++;
	(*File) = 0xFF;
	File++;

}


/*
 * Record audio into file or array pointed by File, records as long as possible so length
 * of file must be calculated beforehand and given as FileLen. Remember that header will
 * need approximately 44 bytes.
 */
void Record(/*unsigned char**/u_short* File, unsigned int FileLen, unsigned int SampleRate, unsigned int BitsPerSample, int MicOn, int StereoOn)
{
	unsigned int samplecount;
	unsigned short sample;
	unsigned short mode_reg;

	RecordFileInit(File, SampleRate, BitsPerSample, StereoOn);
	RecordDeviceInit(SampleRate, MicOn, StereoOn);

	File = File + WAVHEADERSIZE;
	mode_reg = VsRegRead(VS_MODE_REG);

	if (BitsPerSample == 16) {
		while (FileLen > 1){
			samplecount = VsRegRead(VS_HDAT1_REG);
			// spec page 54: if sci_hdat1 >= 896 may be better not
			// to read and wait for overflow
			if ( (samplecount > 0)){
				for (; samplecount > 0 && FileLen > 1; samplecount--){
					sample = VsRegRead(VS_HDAT0_REG);
					(*File) = (sample);
					File++;
					FileLen--;
					/*(*File) =(unsigned char)(sample >> 8);
					File++;
					FileLen--;*/
				}
			}
		}
	}
	else {
		while (FileLen > 1){
			samplecount = VsRegRead(VS_HDAT1_REG);
			// spec page 54: if sci_hdat1 >= 896 may be better not
			// to read and wait for overflow
			if ( (samplecount > 0) && (samplecount < 896) ){
				for (; samplecount > 0 && FileLen > 1; samplecount--){
					sample = VsRegRead(VS_HDAT0_REG);
					(*File) =(unsigned char)(sample >> 8);
					File++;
					FileLen--;
				}
			}
		}
	}
}


/* Enters test mode and plays sine wave */
void VsTest(void)
{
    static prog_char on[] = { 0x53, 0xEF, 0x6E };
    //static prog_char off[] = { 0x45, 0x78, 0x69, 0x74 };
    static prog_char end[] = { 0x00, 0x00, 0x00, 0x00 };
    static prog_char sine[] = { 0x7E };

    VsRegWrite(VS_MODE_REG, VS_SM_TESTS | VS_SM_SDINEW);

    VsSdiWrite_P(on, sizeof(on));
    VsSdiWrite_P(sine , sizeof(sine));
    VsSdiWrite_P(end, sizeof(end));
}


/* Does some low-level initialization */
void InitAudio(void)
{
	SpiInit();
}


/* Cancels ongoing operation and the resets audio chip, plain cancel
 * should be enough though */
void CancelReset(void)
{
	unsigned short mode_reg;

    mode_reg = VS_MODE_REG | VS_SM_CANCEL;
	VsRegWrite(SCI_MODE, mode_reg);
	//NutSleep(2000);

    mode_reg = VS_MODE_REG | VS_SM_RESET;
	VsRegWrite(SCI_MODE, mode_reg);
	//NutSleep(2000);
}


/* Quite extreme way to reset device, must be called when mode of operation
 * is changed eg. from record to play */
void ResetDevice(void)
{
    CancelReset();
    InitAudio();
}
