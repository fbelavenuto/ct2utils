/* ct2utils - Utilitarios para manipular arquivos CT2 (Cassete TK2000)
 *
 * Copyright 2011-2020 Fábio Belavenuto
 *
 * Este arquivo é distribuido pela Licença Pública Geral GNU.
 * Veja o arquivo "Licenca.txt" distribuido com este software.
 *
 * ESTE SOFTWARE NÃO OFERECE NENHUMA GARANTIA
 */

#ifndef CT2_H_
#define CT2_H_

/* Definicoes */
#define CT2_MAGIC "CTK2"
#define CT2_CAB_A "CA\0\0"
#define CT2_CAB_B "CB\0\0"
#define CT2_DADOS "DA"
#define DURSILENP	800			// 800 ms
#define DURSILENS	500			// 500 ms
#define DURCABA		1000		//   1 seg
#define MINCABA		500			// 500 ms
#define MAXCABA		3000		//   3 seg


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
