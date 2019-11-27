/* ct2utils - Utilitarios para manipular arquivos CT2 (Cassete TK2000)
 *
 * Copyright 2011-2020 Fábio Belavenuto
 *
 * Este arquivo é distribuido pela Licença Pública Geral GNU.
 * Veja o arquivo "Licenca.txt" distribuido com este software.
 *
 * ESTE SOFTWARE NÃO OFERECE NENHUMA GARANTIA
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
void imprimeInf(TTKCab *tkcab, TTKEnd *tkend);

#endif /* FUNCTIONS_H_ */
