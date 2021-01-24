#ifndef _CONFIG_H_
#define _CONFIG_H_

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
 * \file config.h
 * \brief Configuration items.
 *
 * \verbatim
 *
 * $Log$
 *
 * \endverbatim
 */

#include <sys/types.h>

#ifndef VERSION
#define VERSION "1.2.1"
#endif

#if defined(AT91SAM9260_EK) || defined(ELEKTOR_IR1)
#ifndef USE_LARGE_RAM
#define USE_LARGE_RAM   1
#endif
#endif

#ifndef USE_DISPLAY
/*!
 * \brief Enable or disable display control.
 *
 * To route the LCD output to stdout, use
 * \code
 * #define USE_TERM  1
 * \endcode
 * Seems to work with Hyperterminal, but not with TeraTerm.
 * Note, that you must also set
 * \code
 * #define LOG_ENABLE 0
 * \endcode
 * in logmsg.h.
 */
#if defined(AT91SAM7X_EK) || defined(AT91SAM9260_EK)
#define USE_DISPLAY     1
#else
#define USE_DISPLAY     0
#endif
#endif

#ifndef USE_BUTTONS
/*!
 * \brief Enable or disable button monitoring.
 */
#if defined(AT91SAM7X_EK) || defined(AT91SAM9260_EK)
#define USE_BUTTONS     1
#else
#define USE_BUTTONS     0
#endif
#endif

#ifndef MAX_WAIT_MP3BUF_FILLED
#define MAX_WAIT_MP3BUF_FILLED  10
#endif

#if defined(AT91SAM7X_EK) || defined(AT91SAM9260_EK)
#define USE_SOFTWARE_CODEC
#endif

/*
 * Wether we should run a web interface.
 */
#if defined(AT91SAM9260_EK) || defined(ELEKTOR_IR1) || defined(ETHERNUT3)
#define USE_HTTPSERVER
#ifndef HTTPD_TCP_TIMEOUT
#define HTTPD_TCP_TIMEOUT   500
#endif
#endif

/* Wether to use dynamically created threads. */
#define USE_DYNAMIC_THREADS

/*
 * Maximum segment size, choose 536 up to 1460. Note, that segment 
 * sizes above 536 may result in fragmented packets. Remember, that 
 * Nut/Net doesn't support TCP fragmentation.
 */
#ifndef MAX_TCPSEG_SIZE
#define MAX_TCPSEG_SIZE 536
#endif

/*
 * TCP buffer size.
 */
#ifndef MAX_TCPBUF_SIZE
#if defined(AT91SAM9260_EK) || defined(ELEKTOR_IR1)
#define MAX_TCPBUF_SIZE (60 * MAX_TCPSEG_SIZE)
#else
#define MAX_TCPBUF_SIZE (3 * MAX_TCPSEG_SIZE)
#endif
#endif

#ifndef MP3_BUFSIZ
#if defined (AT91SAM9260_EK) || defined(ELEKTOR_IR1)
#define MP3_BUFSIZ          1048576
#else
#define MP3_BUFSIZ          (4 * MAINBUF_SIZE)
#endif
#endif

#ifndef MAX_WAITSTREAM
#define MAX_WAITSTREAM      20
#endif

/*
 * Socket receive timeout.
 */
#ifndef MAX_TCPRCV_WAIT
#define MAX_TCPRCV_WAIT     5000
#endif

/*!
 * \brief Proxy configuration structure.
 */
typedef struct {
    /*! \brief Proxy IP address or host name. */
    char proxy_host[128];
    /*! \brief Proxy's HTTP port.
     *
     * If not zero, all HTTP requests will be directed
     * to \ref proxy_host.
     */
    u_short proxy_port;
    //u_long proxy_ip;
    //u_char *proxy_user;
} HTTP_PROXY;

extern HTTP_PROXY proxy;

__BEGIN_DECLS
/* Prototypes */

extern int ConfigInit(void);
extern void ConfigResetFactory(void);
extern size_t ConfigSize(void);
extern int ConfigLoad(void);
extern int ConfigSave(void);

extern void ConfigFlush(void);
extern void ConfigSaveString(char *str);
extern size_t ConfigLoadString(char * str, size_t size);
extern void ConfigSaveBinary(void *data, size_t len);
extern void ConfigLoadBinary(void *data, size_t len);
extern void ConfigRewind(u_long pgn);

__END_DECLS
/* End of prototypes */
#endif
