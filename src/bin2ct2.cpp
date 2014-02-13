/*  bin2ct2 - Utilitário para ler arquivos .b (formato AppleDOS 3.3 tipo B)
 * 				e gerar arquivos .CT2
 *
 *  by Fábio Belavenuto - Copyright 2011
 *
 *  Versão 0.1beta
 *
 *  Este arquivo é distribuido pela Licença Pública Geral GNU.
 *  Veja o arquivo "Licenca.txt" distribuido com este software.
 *
 *  ESTE SOFTWARE NÃO OFERECE NENHUMA GARANTIA
 */

/*
 * bin2ct2.cpp
 *
 *  Created on: 25/05/2011
 *      Author: Fabio Belavenuto
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wav.h"
#include "ct2.h"
#include "functions.h"

// Definições
#define VERSAO "0.1"

// =============================================================================
void mostraUso(char *nomeprog) {
	fprintf(stderr, "\n");
	fprintf(stderr, "%s - Utilitario para converter arquivos .b (Applesoft DOS 3.3)\n", nomeprog);
	fprintf(stderr, "     para .ct2, padrao TK2000. Versao %s\n\n", VERSAO);
	fprintf(stderr, "  Uso:\n");
	fprintf(stderr, "    %s [opcoes] <arquivo>\n\n", nomeprog);
	fprintf(stderr, "  Opcoes:\n");
	fprintf(stderr, "  -o <arq.>     - Salva resultado nesse arquivo.\n");
	fprintf(stderr, "  -n <nome>     - Nome do arquivo TK2000.\n");
	fprintf(stderr, "  -s <endereco> - Indica endereco inicial do arquivo (em hexa).\n");
	fprintf(stderr, "  -e <endereco> - Indica endereco final do arquivo (em hexa).\n");
	fprintf(stderr, "  -r            - Formato RAW, nao interpreta enderecos.\n");
	fprintf(stderr, "\n");

	exit(0);
}

// =============================================================================
int main(int argc, char *argv[]) {
	FILE *fileBin = NULL, *fileCt2 = NULL;
	char *arqBin = NULL, *arqCt2 = NULL, *buffer = NULL;
	char *p, nome[7], raw = 0;
	unsigned short endI = 0, endF = 0;
	int c = 1, fileSize = 0, bufSize = 0;
	TTKEnd tkend;
	struct arqTK2000 arq;

	if (argc < 2)
		mostraUso(argv[0]);

	memset(nome, 0, 7);
	// Interpreta linha de comando
	while (c < argc) {
		if (argv[c][0] == '-' || argv[c][0] == '/') {
			if (c + 1 == argc) {
				if (argv[c][1] == 'o' || argv[c][1] == 's' ||
						argv[c][1] == 'e' || argv[c][1] == 'n') {
					fprintf(stderr, "Falta parametro para a opcao %s", argv[c]);
					return 1;
				}
			}
			switch (argv[c][1]) {

			case 'o':
				arqCt2 = argv[++c];
				break;

			case 's':
				endI = _httoi(argv[++c]);
				break;

			case 'e':
				endF = _httoi(argv[++c]);
				break;

			case 'n':
				++c;
				strncpy(nome, argv[c], MIN(6, strlen(argv[c])));
				break;

			case 'r':
				raw = 1;
				break;

			default:
				fprintf(stderr, "Opcao invalida: %s\n", argv[c]);
				return 1;
				break;
			} // switch
		} else
			arqBin = argv[c];
		c++;
	}

	if (!arqBin) {
		fprintf(stderr, "Falta nome do arquivo de entrada.\n");
		return 1;
	}

	if (strlen(nome) == 0) {
		fprintf(stderr, "Falta nome do arquivo do TK2000.\n");
		return 1;
	}
	printf("Lendo arquivo '%s'\n", arqBin);
	if (!(fileBin = fopen(arqBin, "rb"))) {
		fprintf(stderr, "Erro ao abrir arquivo %s\n", arqBin);
		return -1;
	}
	fseek(fileBin, 0, SEEK_END);
	fileSize = ftell(fileBin);
	fseek(fileBin, 0, SEEK_SET);
	memset(&tkend, 0, sizeof(TTKEnd));

	if (!raw) {
		fread(&tkend, 1, sizeof(TTKEnd), fileBin);
//		printf("Endereco Inicial: %04X\n", tkend.EndInicial);
//		printf("Endereco Final  : %04X\n", tkend.EndFinal);
//		printf("Tamanho arquivo : %04X\n", fileSize);
		fileSize -= sizeof(TTKEnd);

		if (tkend.EndFinal != fileSize) {
			printf("Os enderecos e comprimento do arquivo binario nao batem com o tamanho do arquivo.\n");
			return -1;
		}
		if (!endI)
			endI = tkend.EndInicial;
		if (!endF)
			endF = endI + tkend.EndFinal - 1;
	} else {
		if (!endF)
			endF = endI + fileSize - 1;
		if (endF - endI + 1 != fileSize) {
			printf("O comprimento indicado pelos enderecos nao correspondem ao tamanho do arquivo.\n");
			return -1;
		}
	}

	if (!arqCt2) {
		arqCt2 = strdup((const char *)arqBin);
	}
	p = (char *)(arqCt2 + strlen(arqCt2));
	while(--p > arqCt2) {
		if (*p == '.') {
			*p = '\0';
			break;
		}
	}
	strcat(arqCt2, ".ct2");
	if (!(fileCt2 = fopen(arqCt2, "wb"))) {
		fprintf(stderr, "Erro ao abrir arquivo %s\n", arqCt2);
		return -1;
	}
	memset(&arq, 0, sizeof(struct arqTK2000));
	memset(arq.nome, ' ', 6);
	memcpy(arq.nome, nome, strlen(nome));
//	printf("Len Nome: %d\n", strlen(nome));
	arq.completo = true;
	arq.dados = (char *)malloc(fileSize);
	arq.endFinal = endF;
	arq.endInicial = endI;
	arq.tamanho = fileSize;
	arq.totalBlocos = (fileSize - 1) / 256 + 1;
	fread(arq.dados, 1, fileSize, fileBin);
	fclose(fileBin);
	bufSize = calculateCt2BufferSize(&arq);
	if (bufSize) {
		buffer = (char *)malloc(bufSize);
		makeCt2File(&arq, buffer);
		fwrite(CT2_MAGIC, 1, 4, fileCt2);
		fwrite(buffer, 1, bufSize, fileCt2);
		free(buffer);
		free(arq.dados);
	}
	fclose(fileCt2);
	return 0;
}
