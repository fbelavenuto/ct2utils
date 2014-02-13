/*
 * ct2.h
 *
 *  Created on: 05/05/2011
 *      Author: Fabio Belavenuto
 */

#ifndef CT2_H_
#define CT2_H_

#define CT2_MAGIC "CTK2"
#define CT2_CAB_A "CA\0\0"
#define CT2_CAB_B "CB\0\0"
#define CT2_DADOS "DA"

typedef struct STKCab
{
	unsigned char Nome[6];
	unsigned char TotalBlocos;
	unsigned char BlocoAtual;
}__attribute__((__packed__)) TTKCab;

typedef struct STKEnd
{
	unsigned short EndInicial;
	unsigned short EndFinal;
}__attribute__((__packed__)) TTKEnd;

typedef struct SCh
{
	unsigned char ID[2];
	unsigned short Tam;
}__attribute__((__packed__)) TCh;


struct arqTK2000 {
	char nome[7];
	int  totalBlocos;
	int  endInicial;
	int  endFinal;
	int  tamanho;
	bool completo;
	char *mapa;
	char *dados;
};

#endif /* CT2_H_ */
