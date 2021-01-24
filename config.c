/*
 * Copyright (C) 2006-2007 by egnite Software GmbH. All rights reserved.
 * Copyright (C) 2008 by egnite GmbH. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * For additional information see http://www.ethernut.de/
 */

/*!
 * \file config.c
 * \brief Radio configuration.
 *
 * \verbatim
 * $Id$
 * \endverbatim
 */

#include <cfg/clock.h>

#include <dev/board.h>
#include <dev/at45db.h>

#include <sys/confos.h>
#include <sys/confnet.h>

#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>

#include "config.h"
//#include "recordtestapp.h"

#define CONFIG_MAGIC    "ir1"
#define CONFOS_SECTOR   0
#define CONFNET_SECTOR  1
#define CONFIG_SECTOR   2

static int at45db;
static u_int page_size;

/*!
 * \brief HTTP proxy configuration.
 */
HTTP_PROXY proxy;

#define DF_WRITE_POLLS        1000

#define DFCMD_READ_STATUS       0xD7
#define DFCMD_CONT_READ         0xE8
#define DFCMD_BUF1_WRITE        0x84
#define DFCMD_BUF1_FLASH        0x83
#define DFCMD_PAGE_ERASE        0x81

#if defined(ELEKTOR_IR1)

#define VS10XX_XCS_BIT          31
#define VS10XX_XCS_PIO_ID       PIOA_ID
#define VS10XX_XDCS_BIT         30
#define VS10XX_XDCS_PIO_ID      PIOB_ID
#define VS10XX_DREQ_BIT         30
#define VS10XX_DREQ_PIO_ID      PIOA_ID
#define VS10XX_SIGNAL           sig_INTERRUPT1
#define VS_XRESET_BIT           31 /* PB31 */

/*
 * Temporary hack to get the radio running with our incomplete system software.
 */
int InitElektorHardware(void)
{
    static int done;

    if (!done) {
        /* Enable peripherals. */
        outr(PIOA_ASR, inr(PIOA_ASR)
            | _BV(PA12_SPI0_MISO_A)
            | _BV(PA13_SPI0_MOSI_A)
            | _BV(PA14_SPI0_SPCK_A)
            | _BV(PA30_IRQ1_A));

        /* Disable PIO lines used for peripherals. */
        outr(PIOA_PDR, _BV(PA12_SPI0_MISO_A)
            | _BV(PA13_SPI0_MOSI_A)
            | _BV(PA14_SPI0_SPCK_A)
            | _BV(PA30_IRQ1_A));

        /* Chip selects are manually controlled. */
        /* Enable XCS control. */
        outr(PIOA_PER, _BV(VS10XX_XCS_BIT));
        outr(PIOA_SODR, _BV(VS10XX_XCS_BIT));
        outr(PIOA_OER, _BV(VS10XX_XCS_BIT));

        /* Enable XDCS control. */
        outr(PIOB_PER, _BV(VS10XX_XDCS_BIT));
        outr(PIOB_SODR, _BV(VS10XX_XDCS_BIT));
        outr(PIOB_OER, _BV(VS10XX_XDCS_BIT));

        /* Release reset line. */
        outr(PIOB_PER, _BV(VS_XRESET_BIT));
        outr(PIOB_SODR, _BV(VS_XRESET_BIT));
        outr(PIOB_OER, _BV(VS_XRESET_BIT));

        /* DataFlash chip select. */
        outr(PIOA_PER, _BV(PA11_SPI0_NPCS0_A));
        outr(PIOA_SODR, _BV(PA11_SPI0_NPCS0_A));
        outr(PIOA_OER, _BV(PA11_SPI0_NPCS0_A));

        /* Enable clocks. */
        outr(PMC_PCER, _BV(SPI0_ID) | _BV(IRQ1_ID) | _BV(PIOA_ID) | _BV(PIOB_ID) | _BV(PIOC_ID));

        /* SPI enable and reset. */
        outr(SPI0_CR, SPI_SPIEN | SPI_SWRST);
        outr(SPI0_CR, SPI_SPIEN);

        done = 1;
    }
    return 0;
}
#endif

/*
 * No interlink available yet. Provide basic SPI routines.
 */

int SpiSetMode(void)
{
    /* Set SPI to master mode, fixed peripheral, fault detection disabled. */
    outr(SPI0_MR, SPI_MODFDIS | SPI_MSTR);

    /* Data changes during clock low and will be sampled on rising edges. */
    outr(SPI0_CSR0, (255 << SPI_SCBR_LSB) | SPI_NCPHA);

    return 0;
}

static u_char SpiByte(u_char val)
{
    //printf("[%02x", val);
    /* Transmission is started by writing the transmit data. */
    outr(SPI0_TDR, val);
    /* Wait for receiver data register full. */
    while((inr(SPI0_SR) & SPI_RDRF) == 0);
    /* Read data. */
    val = (u_char)inr(SPI0_RDR);
    //printf(":%02x]", val);

    return val;
}

int At91SpiTransfer2(u_int base, u_int cs, CONST void *txbuf, void *rxbuf, int xlen, CONST void *txnbuf, void *rxnbuf, int xnlen)
{
    u_char *txp;
    u_char *rxp;
    u_char b;

    //putchar('\n');
    SpiSetMode();
    outr(PIOA_CODR, _BV(PA11_SPI0_NPCS0_A));
    txp = (u_char *)txbuf;
    rxp = (u_char *)rxbuf;
    b = 0xff;
    while (xlen--) {
        if (txp) {
            b = *txp++;
        }
        b = SpiByte(b);
        if (rxp) {
            *rxp++ = b;
        }
    }
    txp = (u_char *)txnbuf;
    rxp = (u_char *)rxnbuf;
    b = 0xff;
    while (xnlen--) {
        if (txp) {
            b = *txp++;
        }
        b = SpiByte(b);
        if (rxp) {
            *rxp++ = b;
        }
    }
    outr(PIOA_SODR, _BV(PA11_SPI0_NPCS0_A));

    return 0;
}


/*!
 * \brief Calculate total size of configuration data.
 *
 * Should be used to avoid configuration data overflow. Untested.
 *
 * \return Number of bytes used by configuration data.
 */
size_t ConfigSize(void)
{
    size_t rc = 0;
    return rc + 1;
}

static u_long pgnum;
static int pgpos = -1;
static u_char pgbuf[1056];

void ConfigFlush(void)
{
    if (pgpos > 0) {
        At45dbPageErase(at45db, pgnum);
        At45dbPageWrite(at45db, pgnum, pgbuf, page_size);
    }
}

static void ConfigPut(u_char ch)
{
    if (pgpos < 0) {
        memset(pgbuf, 0xFF, page_size);
        pgpos = 0;
    }
    pgbuf[pgpos++] = ch;
    if (pgpos >= page_size) {
        ConfigFlush();
        pgnum++;
        pgpos = 0;
    }
}

static u_char ConfigGet(void)
{
    if (pgpos < 0 || pgpos >= page_size) {
        At45dbPageRead(at45db, pgnum, pgbuf, page_size);
        pgnum++;
        pgpos = 0;
    }
    return pgbuf[pgpos++];
}

void ConfigSaveString(char *str)
{
    //printf("[Save'%s']", str);
    do {
        ConfigPut(*str);
    } while(*str++);
}

size_t ConfigLoadString(char * str, size_t size)
{
    size_t rc = 0;
    u_char ch;

    while (rc < size && (ch = ConfigGet()) != 0) {
        str[rc++] = ch;
    }
    str[rc++] = 0;
    //printf("[Load'%s']", str);

    return rc;
}

void ConfigSaveBinary(void *data, size_t len)
{
    u_char *dp = (u_char *)data;

    while (len--) {
        ConfigPut(*dp);
        dp++;
    }
}

void ConfigLoadBinary(void *data, size_t len)
{
    u_char *dp = (u_char *)data;

    while (len--) {
        *dp++ = ConfigGet();
    }
}

void ConfigRewind(u_long pgn)
{
    pgnum = pgn;
    pgpos = -1;
}

/*
 * No NvMem support for all platforms. Provide system configuration.
 */
CONFOS confos;
CONFNET confnet;

/*!
 * \brief Load Nut/OS configuration from non-volatile memory.
 *
 * This routine is automatically called during system
 * initialization.
 *
 * \return 0 if OK, -1 if configuration isn't available.
 */
int NutLoadConfig(void)
{
    if (ConfigInit() == 0) {
        ConfigRewind(CONFOS_SECTOR);
        ConfigLoadBinary(&confos, sizeof(CONFOS));
        if (confos.size == sizeof(CONFOS) && confos.magic[0] == 'O' && confos.magic[1] == 'S') {
            return 0;
        }
    }
    memset(&confos, 0, sizeof(CONFOS));
    return -1;
}

/*!
 * \brief Save Nut/OS configuration in non-volatile memory.
 *
 * \return 0 if OK, -1 on failures.
 */
int NutSaveConfig(void)
{
    confos.size = sizeof(CONFOS);
    confos.magic[0] = 'O';
    confos.magic[1] = 'S';
    ConfigRewind(CONFOS_SECTOR);
    ConfigSaveBinary(&confos, sizeof(CONFOS));
    ConfigFlush();

    return 0;
}

/*!
 * \brief Load network configuration from non-volatile memory.
 *
 * If no configuration is available in EEPROM, all configuration
 * parameters are cleared to zero. Except the MAC address, which
 * is set to the Ethernet broadcast address.
 *
 * \param name Name of the device.
 *
 * \return 0 if configuration has been read. Otherwise the
 *         return value is -1.
 */
int NutNetLoadConfig(CONST char *name)
{
    if (ConfigInit() == 0) {
        ConfigRewind(CONFNET_SECTOR);
        ConfigLoadBinary(&confnet, sizeof(CONFNET));
        if (confnet.cd_size == sizeof(CONFNET) && strcmp(confnet.cd_name, name) == 0) {
            return 0;
        }
    }
    memset(&confnet, 0, sizeof(confnet));
    memset(confnet.cdn_mac, 0xFF, sizeof(confnet.cdn_mac));

    return -1;
}

/*!
 * \brief Save network configuration in non-volatile memory.
 *
 * \return 0 if OK, -1 on failures.
 */
int NutNetSaveConfig(void)
{
    confnet.cd_size = sizeof(CONFNET);
    ConfigRewind(CONFNET_SECTOR);
    ConfigSaveBinary(&confnet, sizeof(CONFNET));
    ConfigFlush();

    return 0;
}

/*!
 * \brief Save configuration in non volatile memory.
 */
int ConfigSave(void)
{
    /* Save our name. */
    ConfigRewind(CONFIG_SECTOR);
    ConfigSaveString(CONFIG_MAGIC);

    ConfigFlush();
    return 0;
}

int ConfigLoad(void)
{
    int rc = -1;
    char *buf;

    buf = malloc(32);

    ConfigRewind(CONFIG_SECTOR);
    ConfigLoadString(buf, 32);
    if (strcmp(buf, CONFIG_MAGIC) == 0) {
        rc = 0;
    } else {
    }
    free(buf);

    return rc;
}

/*!
 * \brief Reset configuration to factory defaults.
 */
void ConfigResetFactory(void)
{
    memset(&proxy, 0, sizeof(proxy));
}

int ConfigInit(void)
{
    if (page_size == 0) {
#if defined(ELEKTOR_IR1)
        InitElektorHardware();
#endif
        if ((at45db = At45dbInit(SPI0_BASE, 0)) == -1) {
            return -1;
        }
        page_size = At45dbPageSize(at45db);
    }
    return 0;
}
