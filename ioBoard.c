/*
 * ioBoard.c
 *
 *  Created on: 8.4.2009
 *      Author: Mauni
 */

#include <dev/gpio.h>
#include "ioBoard.h"

/*
 * Ledit p‰‰lle.
 */
void led1On(void) {
    outr(PIOA_OER, _BV(0));
    outr(PIOA_SODR, _BV(0));
}

void led2On(void) {
    outr(PIOA_OER, _BV(1));
    outr(PIOA_SODR, _BV(1));
}

void led3On(void) {
    outr(PIOA_OER, _BV(2));
    outr(PIOA_SODR, _BV(2));
}

void led4On(void) {
    outr(PIOA_OER, _BV(22));
    outr(PIOA_SODR, _BV(22));
}

/*
 * Ledit pois p‰‰lt‰.
 */
void led1Off(void) {
	outr(PIOA_OER, _BV(0));
	outr(PIOA_CODR, _BV(0));
}

void led2Off(void) {
	outr(PIOA_OER, _BV(1));
	outr(PIOA_CODR, _BV(1));
}

void led3Off(void) {
	outr(PIOA_OER, _BV(2));
	outr(PIOA_CODR, _BV(2));
}

void led4Off(void) {
	outr(PIOA_OER, _BV(22));
	outr(PIOA_CODR, _BV(22));
}

void ledsOff(void) {
	led1Off();
	led2Off();
	led3Off();
	led4Off();
}



