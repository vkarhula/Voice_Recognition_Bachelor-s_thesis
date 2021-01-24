/*
 * doxygen.c
 *
 *  Created on: 28.5.2009
 *      Author: Administrator
 */


Eli kansiossa on vanhat sourcet, jotka on syötetty Doxygeniin.
rtf-kansiosta löytyy refman, joka on Doxygenin tekemä reference manual.
html-kansiosta löytyy nettisivumuodossa samat tiedot, kannattaa selailla.
latex-kansiossa on latex-muodossa eli sieltä ei taida löytyä kovin kiinnostavaa tietoa.

Esim. struktien kommentoinnissa on käytettyä muotoa:
/*! \struct viesti
 *  \brief Strukti, joka pitää sisällään viestin tietoja.
 */

ja funktioiden kommentoinnissa muotoa:
/*! \fn void ParseSendMessage(u_char *data, u_short size)
 *  \brief Funktio, joka parsii lähetettävän viestin.
 *  \param *data Lähetettävä data.
 *  \param size Lähetettävän datan koko.
 *  \return void.
 */
