/* ct2utils - Tools to manipulate CT2 files (TK2000 cassete)
 *
 * Copyright (C) 2014-2020  Fabio Belavenuto
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "version.h"
#include "wav.h"
#include "ct2.h"
#include "functions.h"

// Defines

// Private functions

// =============================================================================
static void showUse(char *nomeprog) {
	fprintf(stderr, "\n");
	fprintf(stderr, "%s - Utilitario para gerar arquivos .b (Applesoft DOS 3.3) a partir\n", nomeprog);
	fprintf(stderr, "          do arquivo .ct2 formato TK2000. Versao %s\n\n", VERSION);
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
int main (int argc, char *argv[]) {
	unsigned short	tam = 0, sh;
	int				c = 1, na, raw = 0, cp = 0;
	char			temp[1024], *p;
	char			*ct2Filename    = NULL;
	char			*arqBin    = NULL;
	char			nomeTK[7], arqTemp[255];
	FILE			*fileCt2   = NULL;
	FILE			*fileBin   = NULL;
	struct STKCab	tkcab;
	struct STKAddr	tkend;
	struct SCh		ch;

	if (argc < 2)
		showUse(argv[0]);

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
			ct2Filename = argv[c];
		c++;
	}

	if (!ct2Filename) {
		fprintf(stderr, "Falta nome do arquivo.\n");
		return 1;
	}

	if (!arqBin) {
		arqBin = (char *)malloc(strlen(ct2Filename) + 1);
		strcpy(arqBin, ct2Filename);
		p = (char *)(arqBin + strlen(arqBin));
		while(--p > arqBin) {
			if (*p == '.') {
				*p = '\0';
				break;
			}
		}
	}

	//
	if (!(fileCt2 = fopen(ct2Filename, "rb"))) {
		fprintf(stderr, "Erro ao abrir arquivo %s\n", ct2Filename);
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

		if (!strncmp((const char*)ch.ID, CT2_DATA, 2)) {	// Se for um chunk de dados

			fread(&tkcab, 1, sizeof(struct STKCab), fileCt2);
			// Verifica se é o primeiro bloco
			if (tkcab.actualBlock == 0) {
				int i;
				fread(&tkend, 1, sizeof(struct STKAddr), fileCt2);
				showInf(&tkcab, &tkend);
				for (i=0; i<6; i++)
					nomeTK[i] = tkcab.name[i] & 0x7F;
				nomeTK[6] = '\0';
				if (cp) {
					sprintf(arqTemp, "%s_%s.b#06%04X", arqBin, trim(nomeTK), tkend.endAddr);
				} else {
					sprintf(arqTemp, "%s_%s.b", arqBin, trim(nomeTK));
				}
				cleanFN(arqTemp);
				na = 0;
				while(1) {
					if ((fileBin = fopen(arqTemp, "rb"))) {
						na++;
						fclose(fileBin);
						if (cp) {
							sprintf(arqTemp, "%s_%s%d.b#06%04X", arqBin, trim(nomeTK), na, tkend.initialAddr);
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
					tam = tkend.endAddr;
					fwrite(&tam, 1, sizeof(short), fileBin);
				}
				tam = tkend.endAddr - tkend.initialAddr + 1;
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
			if (tkcab.actualBlock == tkcab.numberOfBlocks) {
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
