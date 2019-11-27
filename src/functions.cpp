/* ct2utils - Utilitarios para manipular arquivos CT2 (Cassete TK2000)
 *
 * Copyright 2011-2020 Fábio Belavenuto
 *
 * Este arquivo é distribuido pela Licença Pública Geral GNU.
 * Veja o arquivo "Licenca.txt" distribuido com este software.
 *
 * ESTE SOFTWARE NÃO OFERECE NENHUMA GARANTIA
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "ct2.h"
#include "functions.h"

// ============================================================================
char *trim(char *s) {
	char *ptr;
	if (!s)
		return NULL; // handle NULL string
	if (!*s)
		return s; // handle empty string
	for (ptr = s + strlen(s) - 1; (ptr >= s) && (*ptr == ' '); --ptr)
		;
	ptr[1] = '\0';
	return s;
}

// ============================================================================
char *extractFileName(char *s) {
	char *ptr = s + strlen(s);
	while(--ptr > s) {
		if (*ptr == '\\' || *ptr == '/')
			return ptr+1;
	}
	return s;
}

// ============================================================================
char *cleanFN(char *s) {
	char *ptr = s;
	while (*ptr) {
		if (*ptr == '\\' || *ptr == '/' || *ptr == ':' || *ptr == '*' || *ptr
				== '?' || *ptr == '"' || *ptr == '<' || *ptr == '>' || *ptr
				== '|') {
			*ptr = '_';
		}
		ptr++;
	}
	return s;
}

// ============================================================================
int _httoi(char *value) {
	struct CHexMap {
		char chr;
		int value;
	};
	const int HexMapL = 16;
	CHexMap HexMap[HexMapL] = {
			{ '0', 0 }, { '1', 1 }, { '2', 2 }, { '3', 3 },
			{ '4', 4 }, { '5', 5 }, { '6', 6 }, { '7', 7 },
			{ '8', 8 }, { '9', 9 }, { 'A', 10 }, { 'B', 11 },
			{ 'C', 12 }, { 'D', 13 }, {'E', 14 }, { 'F', 15 } };
	int result = 0;
	char *temp = strdup((const char *)value);
	char *s = temp;

	while(*s) {
		*s = toupper(*s);
		s++;
	}
	s = temp;
	if (*s == '0' && *(s + 1) == 'X')
		s += 2;
	while (*s) {
		bool found = false;
		for (int i = 0; i < HexMapL; i++) {
			if (*s == HexMap[i].chr) {
				result <<= 4;
				result |= HexMap[i].value;
				found = true;
				break;
			}
		}
		if (!found)
			break;
		s++;
	}
	free(temp);
	return result;
}

// ============================================================================
int calculateCt2BufferSize(struct arqTK2000 *arq) {
	int m, n, tamBuf;

	if (!strlen(arq->nome) || !arq->completo || !arq->dados || !arq->tamanho)
		return 0;
	/* Calcular tamanho do buffer */
	n = arq->tamanho / 256;
	m = arq->tamanho % 256;
	tamBuf = 4;
	tamBuf += 4 + 4 + sizeof(TTKCab) + sizeof(TTKEnd) + 1;
	tamBuf += n * (4 + 4 + sizeof(TTKCab) + 256 + 1);
	if (m)
		tamBuf += 4 + 4 + sizeof(TTKCab) + m + 1;
	return tamBuf;
}

// ============================================================================
bool makeCt2File(struct arqTK2000 *arq, char *buffer) {
	int t, da;
	char *p, *pc, *pd, *l;
	unsigned char cs;
	unsigned short tt;
	TTKCab tkcab;
	TTKEnd tkend;

	if (!strlen(arq->nome) || !arq->completo || !arq->dados || !arq->tamanho)
		return false;
	p = buffer;

	// Cabecalho A
	memcpy(p, CT2_CAB_A, 4);
	p += 4;
	// Bloco 0
	memcpy(p, CT2_CAB_B, 4);
	p += 4;
	memcpy(p, CT2_DADOS, 2);
	p += 2;
	tt = sizeof(TTKCab) + sizeof(TTKEnd) + 1;
	memcpy(p, &tt, 2);
	p += 2;
	for (t = 0; t < 6; t++) {
		tkcab.Nome[t] = arq->nome[t] | 0x80;
	}
	tkcab.TotalBlocos = arq->totalBlocos;
	tkcab.BlocoAtual = 0;
	tkend.EndInicial = arq->endInicial;
	tkend.EndFinal = arq->endFinal;
	pc = p;
	memcpy(p, &tkcab, sizeof(TTKCab));
	p += sizeof(TTKCab);
	memcpy(p, &tkend, sizeof(TTKEnd));
	p += sizeof(TTKEnd);
	// calcular checksum
	cs = 0xFF;
	for (l = pc; l < p; l++) {
		cs ^= *l;
	}
	memcpy(p, &cs, 1);
	p += 1;
	da = arq->tamanho;
	pd = arq->dados;
	while (da > 0) {
		if (da > 256)
			t = 256;
		else
			t = da;
		memcpy(p, CT2_CAB_B, 4);
		p += 4;
		memcpy(p, CT2_DADOS, 2);
		p += 2;
		tt = t + sizeof(TTKCab) + 1;
		memcpy(p, &tt, 2);
		p += 2;
		tkcab.BlocoAtual++;
		pc = p;
		memcpy(p, &tkcab, sizeof(TTKCab));
		p += sizeof(TTKCab);
		memcpy(p, pd, t);
		p += t;
		// calcular checksum
		cs = 0xFF;
		for (l = pc; l < p; l++) {
			cs ^= *l;
		}
		memcpy(p, &cs, 1);
		p += 1;
		pd += t;
		da -= t;
	}
	return true;
}

// =============================================================================
void imprimeInf(TTKCab *tkcab, TTKEnd *tkend) {
	int  i;
	char nome[7];

	for (i=0; i<6; i++)
		nome[i] = tkcab->Nome[i] & 0x7F;
	nome[6] = 0;
	printf("'%s' de 0x%.2X blocos, ", nome, tkcab->TotalBlocos);
	printf("de 0x%.4X a 0x%.4X\n", tkend->EndInicial, tkend->EndFinal);
}
