/*
 * doxygen.c
 *
 *  Created on: 28.5.2009
 *      Author: Administrator
 */


Eli kansiossa on vanhat sourcet, jotka on sy�tetty Doxygeniin.
rtf-kansiosta l�ytyy refman, joka on Doxygenin tekem� reference manual.
html-kansiosta l�ytyy nettisivumuodossa samat tiedot, kannattaa selailla.
latex-kansiossa on latex-muodossa eli sielt� ei taida l�yty� kovin kiinnostavaa tietoa.

Esim. struktien kommentoinnissa on k�ytetty� muotoa:
/*! \struct viesti
 *  \brief Strukti, joka pit�� sis�ll��n viestin tietoja.
 */

ja funktioiden kommentoinnissa muotoa:
/*! \fn void ParseSendMessage(u_char *data, u_short size)
 *  \brief Funktio, joka parsii l�hetett�v�n viestin.
 *  \param *data L�hetett�v� data.
 *  \param size L�hetett�v�n datan koko.
 *  \return void.
 */
