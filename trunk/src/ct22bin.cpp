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
 * ct22bin.cpp
 *
 *  Created on: 15/02/2012
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
void imprimeInf(TTKCab *tkcab, TTKEnd *tkend)
{
	int  i;
	char nome[7];

	for (i=0; i<6; i++)
		nome[i] = tkcab->Nome[i] & 0x7F;
	nome[6] = 0;
	printf("'%s' de 0x%.2X blocos, ", nome, tkcab->TotalBlocos);
	printf("de 0x%.4X a 0x%.4X\n", tkend->EndInicial, tkend->EndFinal);
}

// =============================================================================
void mostraUso(char *nomeprog)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "%s - Utilitario para gerar arquivos .b (Applesoft DOS 3.3) a partir\n", nomeprog);
	fprintf(stderr, "          do arquivo .ct2 formato TK2000. Versao %s\n\n", VERSAO);
	fprintf(stderr, "  Uso:\n");
	fprintf(stderr, "    %s [opcoes] <arquivo>\n\n", nomeprog);
	fprintf(stderr, "  Opcoes:\n");
	fprintf(stderr, "  -o <arq.>   - Prefixo do(s) arquivo(s) binario(s).\n");
	fprintf(stderr, "  -r          - Formato RAW, nao salva endereco e comprimento.\n");
	fprintf(stderr, "  -c          - Adiciona no nome do arquivo o tipo e endereco para\n");
	fprintf(stderr, "                preservacao dos atributos, no formato do CiderPress.\n");
	fprintf(stderr, "                Ex: ARQUIVO.b#060800\n");
	fprintf(stderr, "\n");
	exit(0);
}

// =============================================================================
int main (int argc, char *argv[])
{
	unsigned short	tam = 0, sh;
	int				c = 1, na, raw = 0, cp = 0;
	char			temp[1024], *p;
	char			*arqCt2    = NULL;
	char			*arqBin    = NULL;
	char			nomeTK[7], arqTemp[255];
	FILE			*fileCt2   = NULL;
	FILE			*fileBin   = NULL;
	TTKCab			tkcab;
	TTKEnd			tkend;
	TCh				ch;

	if (argc < 2)
		mostraUso(argv[0]);

	// Interpreta linha de comando
	while (c < argc) {
		if (argv[c][0] == '-' || argv[c][0] == '/') {
			if (argv[c][1] == 'o' && c+1 == argc) {
				fprintf(stderr, "Falta parametro para a opcao %s", argv[c]);
				return 1;
			}
			switch(argv[c][1]) {

			case 'o':
				arqBin = argv[++c];
				break;

			case 'r':
				raw = 1;
				break;

			case 'c':
				cp = 1;
				break;

			default:
				fprintf(stderr, "Opcao invalida: %s\n", argv[c]);
				return 1;
				break;
			} // switch
		} else
			arqCt2 = argv[c];
		c++;
	}

	if (!arqCt2) {
		fprintf(stderr, "Falta nome do arquivo.\n");
		return 1;
	}

	if (!arqBin) {
		arqBin = (char *)malloc(strlen(arqCt2) + 1);
		strcpy(arqBin, arqCt2);
		p = (char *)(arqBin + strlen(arqBin));
		while(--p > arqBin) {
			if (*p == '.') {
				*p = '\0';
				break;
			}
		}
	}

	//
	if (!(fileCt2 = fopen(arqCt2, "rb"))) {
		fprintf(stderr, "Erro ao abrir arquivo %s\n", arqCt2);
		return -1;
	}


	fread(temp, 1, 4, fileCt2);
	if (strncmp(temp, CT2_MAGIC, 4)) {
		fclose(fileCt2);
		fclose(fileBin);
		fprintf(stderr, "ERRO: Arquivo nao esta no formato .CT2\n");
		return 1;
	}

	// Enquanto não acabar o arquivo
	while (!feof(fileCt2)) {
		// Lê Chunk de 4 bytes
		fread(&ch, 1, 4, fileCt2);

		if (!strncmp((const char*)ch.ID, CT2_DADOS, 2)) {	// Se for um chunk de dados

			fread(&tkcab, 1, sizeof(TTKCab), fileCt2);
			// Verifica se é o primeiro bloco
			if (tkcab.BlocoAtual == 0) {
				fread(&tkend, 1, sizeof(TTKEnd), fileCt2);
				imprimeInf(&tkcab, &tkend);
				for (int i=0; i<6; i++)
					nomeTK[i] = tkcab.Nome[i] & 0x7F;
				nomeTK[6] = '\0';
				if (cp) {
					sprintf(arqTemp, "%s_%s.b#06%04X", arqBin, trim(nomeTK), tkend.EndInicial);
				} else {
					sprintf(arqTemp, "%s_%s.b", arqBin, trim(nomeTK));
				}
				cleanFN(arqTemp);
				na = 0;
				while(true) {
					if ((fileBin = fopen(arqTemp, "rb"))) {
						na++;
						fclose(fileBin);
						if (cp) {
							sprintf(arqTemp, "%s_%s%d.b#06%04X", arqBin, trim(nomeTK), na, tkend.EndInicial);
						} else {
							sprintf(arqTemp, "%s_%s%d.b", arqBin, trim(nomeTK), na);
						}
						cleanFN(arqTemp);
					} else
						break;
				};
				if (!(fileBin = fopen(arqTemp, "wb"))) {
					fprintf(stderr, "Erro ao criar arquivo %s", arqTemp);
					return 1;
				}
				if (!raw) {
					/* Grava endereço inicial e tamanho */
					tam = tkend.EndInicial;
					fwrite(&tam, 1, sizeof(short), fileBin);
				}
				tam = tkend.EndFinal - tkend.EndInicial + 1;
				if (!raw) {
					fwrite(&tam, 1, sizeof(short), fileBin);
				}
			} else {
				sh = (tam > 256) ? 256 : tam;
				tam -= sh;
				fread(temp, 1, sh, fileCt2);
				fwrite(temp, 1, sh, fileBin);
			}
			/* Ler e descartar checksum */
			fread(temp, 1, 1, fileCt2);
			/* Verifica se é o final do arquivo */
			if (tkcab.BlocoAtual == tkcab.TotalBlocos) {
				fclose(fileBin);
				fileBin = NULL;
			}
		}
	}
	if (fileBin)
		fclose(fileBin);
	fclose(fileCt2);
	return 0;
}
