/*
 * functions.h
 *
 *  Created on: 24/05/2011
 *      Author: Fabio Belavenuto
 */

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include "ct2.h"

char *trim(char *s);
char *extractFileName(char *s);
char *cleanFN(char *s);
int _httoi(char *value);
int calculateCt2BufferSize(struct arqTK2000 *arq);
bool makeCt2File(struct arqTK2000 *arq, char *buffer);

#endif /* FUNCTIONS_H_ */
