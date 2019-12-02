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

/*****************************************************************************/
static void showUse(char *nomeprog) {
	fprintf(stderr, "\n");
	fprintf(stderr, "%s - Utilitario para converter arquivos .b (Applesoft DOS 3.3)\n", nomeprog);
	fprintf(stderr, "     para .ct2, padrao TK2000. Versao %s\n\n", VERSION);
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

// Public functions

/*****************************************************************************/
int main(int argc, char *argv[]) {
	FILE *fileBin = NULL, *fileCt2 = NULL;
	char *arqBin = NULL, *ct2Filename = NULL, *buffer = NULL;
	char *p, name[7], raw = 0;
	unsigned short endI = 0, endF = 0;
	int c = 1, fileSize = 0, bufSize = 0;
	struct STKAddr tkend;
	struct tk2kBinary arq;

	if (argc < 2)
		showUse(argv[0]);

	memset(name, 0, 7);
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
				ct2Filename = argv[++c];
				break;

			case 's':
				endI = _httoi(argv[++c]);
				break;

			case 'e':
				endF = _httoi(argv[++c]);
				break;

			case 'n':
				++c;
				strncpy(name, argv[c], MIN(6, strlen(argv[c])));
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

	if (strlen(name) == 0) {
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
	memset(&tkend, 0, sizeof(struct STKAddr));

	if (!raw) {
		fread(&tkend, 1, sizeof(struct STKAddr), fileBin);
//		printf("Endereco Inicial: %04X\n", tkend.EndInicial);
//		printf("Endereco Final  : %04X\n", tkend.EndFinal);
//		printf("Tamanho arquivo : %04X\n", fileSize);
		fileSize -= sizeof(struct STKAddr);

		if (tkend.endAddr != fileSize) {
			printf("Os enderecos e comprimento do arquivo binario nao batem com o tamanho do arquivo.\n");
			return -1;
		}
		if (!endI)
			endI = tkend.initialAddr;
		if (!endF)
			endF = endI + tkend.endAddr - 1;
	} else {
		if (!endF)
			endF = endI + fileSize - 1;
		if (endF - endI + 1 != fileSize) {
			printf("O comprimento indicado pelos enderecos nao correspondem ao tamanho do arquivo.\n");
			return -1;
		}
	}

	if (!ct2Filename) {
		ct2Filename = strdup((const char *)arqBin);
	}
	p = (char *)(ct2Filename + strlen(ct2Filename));
	while(--p > ct2Filename) {
		if (*p == '.') {
			*p = '\0';
			break;
		}
	}
	strcat(ct2Filename, CT2_EXT);
	if (!(fileCt2 = fopen(ct2Filename, "wb"))) {
		fprintf(stderr, "Erro ao abrir arquivo %s\n", ct2Filename);
		return -1;
	}
	memset(&arq, 0, sizeof(struct tk2kBinary));
	memset(arq.name, ' ', 6);
	memcpy(arq.name, name, strlen(name));
	arq.isComplete = 1;
	arq.data = (char *)malloc(fileSize);
	arq.endAddr = endF;
	arq.initialAddr = endI;
	arq.size = fileSize;
	arq.numberOfBlocks = (fileSize - 1) / 256 + 1;
	fread(arq.data, 1, fileSize, fileBin);
	fclose(fileBin);
	bufSize = calcCt2BufferSize(arq.size);
	if (bufSize) {
		buffer = (char *)malloc(bufSize);
		createOneCt2Binary(arq, buffer);
		fwrite(CT2_MAGIC, 1, 4, fileCt2);
		fwrite(buffer, 1, bufSize, fileCt2);
		free(buffer);
		free(arq.data);
	}
	fclose(fileCt2);
	return 0;
}
