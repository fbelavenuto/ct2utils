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

//#define DEBUG1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "version.h"
#include "functions.h"
#include "wav.h"
#include "ct2.h"

// Defines

// Variables
int  DurSilencioP    = DURSILENP;
int  DurSilencioS    = DURSILENS;
int  DurCabA         = DURCABA;
int  TaxaAmostragem  = 44100;
int  totalDados      = 0;

// Private functions

// =============================================================================
static void showUse(char *nomeprog) {
	fprintf(stderr, "\n");
	fprintf(stderr, "%s - Utilitario para gerar arquivo de audio .wav a partir\n", nomeprog);
	fprintf(stderr, "          do arquivo .ct2 formato TK2000. Versao %s\n\n", VERSION);
	fprintf(stderr, "  Uso:\n");
	fprintf(stderr, "    %s [opcoes] <arquivo>\n\n", nomeprog);
	fprintf(stderr, "  Opcoes:\n");
	fprintf(stderr, "  -t <sps>    - Determina a taxa de amostragem em \"Samples per Second\".\n");
	fprintf(stderr, "                Padrao: %d sps\n", TaxaAmostragem);
	fprintf(stderr, "  -p <ms>     - Determina a duracao do silencio inicial.\n");
	fprintf(stderr, "                Padrao: %d ms \n", DurSilencioP);
	fprintf(stderr, "  -s <ms>     - Determina a duracao do silencio entre 2 arquivos.\n");
	fprintf(stderr, "                Padrao: %d ms \n", DurSilencioS);
	fprintf(stderr, "  -c <ms>     - Determina a duracao do cabecalho.\n");
	fprintf(stderr, "                Padrao: %d ms, Minimo de %d ms, Maximo de %d ms \n", DurCabA, MINCABA, MAXCABA);
	fprintf(stderr, "  -o <arq.>   - Salva resultado nesse arquivo.\n");
	fprintf(stderr, "\n");
	exit(0);
}

// =============================================================================
int main (int argc, char *argv[]) {
	int           c = 1, silencioAtual = DurSilencioP;
	int           pos, comp, cont;
	char          temp[1024], b, *p;
	char          *ct2Filename    = NULL;
	char          *wavFilename    = NULL;
	FILE          *fileCt2   = NULL;
	struct STKCab tkcab;
	struct STKAddr tkend;
	struct SCh    ch;

	if (argc < 2)
		showUse(argv[0]);

	// Interpreta linha de comando
	while (c < argc) {
		if (argv[c][0] == '-' || argv[c][0] == '/') {
			if (c+1 == argc) {
				fprintf(stderr, "Falta parametro para a opcao %s", argv[c]);
				return 1;
			}
			switch(argv[c][1]) {

			case 't':
				++c;
				TaxaAmostragem = MIN(88200, MAX(8000, atoi(argv[c])));
				break;

			case 'p':
				++c;
				DurSilencioP = MAX(10, atoi(argv[c]));
				break;

			case 's':
				++c;
				DurSilencioS = MAX(10, atoi(argv[c]));
				break;

			case 'c':
				++c;
				DurCabA = MIN(MAXCABA, MAX(MINCABA, atoi(argv[c])));
				break;

			case 'o':
				wavFilename = argv[++c];
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

	if (!wavFilename) {
		wavFilename = (char *)malloc(strlen(ct2Filename) + 1);
		strcpy(wavFilename, ct2Filename);
		p = (char *)(wavFilename + strlen(wavFilename));
		while(--p > wavFilename) {
			if (*p == '.') {
				strncpy(p, ".wav\0", 5);
				break;
			}
		}
	}

	if (!(fileCt2 = fopen(ct2Filename, "rb"))) {
		fprintf(stderr, "Erro ao abrir arquivo %s\n", ct2Filename);
		return -1;
	}

	fread(temp, 1, 4, fileCt2);
	if (strncmp(temp, CT2_MAGIC, 4)) {
		fclose(fileCt2);
		fprintf(stderr, "ERRO: Arquivo nao esta no formato .CT2\n");
		return 1;
	}

	wavConfig(WF_SINE, TaxaAmostragem, 16, 1.0, 0);
	if (!createWaveFile(wavFilename)) {
		fprintf(stderr, "Erro ao criar arquivo %s", wavFilename);
		return 1;
	}

	// Enquanto não acabar o arquivo
	while (!feof(fileCt2)) {
		// Lê Chunk de 4 bytes
		fread(&ch, 1, 4, fileCt2);
		// Tamanho do chunk
		comp = ch.size;
		if (!strncmp((const char*)ch.ID, CT2_CAB_A, 4)) {			// Se for um chunk informando ser cabecalho A
#ifdef DEBUG1
			printf("Cabecalho A\n");
#endif
			playSilence(silencioAtual);
			playTone(TK2000_BIT1, DurCabA, 0.5);
			silencioAtual = DurSilencioS;
		} else if (!strncmp((const char*)ch.ID, CT2_CAB_B, 4)) {	// Se for um chunk informando ser cabecalho B
#ifdef DEBUG1
			printf("Cabecalho B\n");
#endif
			playTone(TK2000_CABB, 30, 0.5);
			playTone(TK2000_BIT0, 1, 0.5);		// bit 0
		} else if (!strncmp((const char*)ch.ID, CT2_DATA, 2)) {	// Se for um chunk informando ser dados
#ifdef DEBUG1
			printf("Dados\n");
#endif
			// Toca Dados
			pos = ftell(fileCt2);
			fread(&tkcab, 1, sizeof(struct STKCab), fileCt2);
			// Verifica se é o primeiro bloco
			if (tkcab.actualBlock == 0) {
				fread(&tkend, 1, sizeof(struct STKAddr), fileCt2);
				showInf(&tkcab, &tkend);
#ifdef DEBUG1
			} else {
				printf("bloco %.2X\n", tkcab.BlocoAtual);
#endif
			}
			// Toca Dados
			fseek(fileCt2, pos, SEEK_SET);
			for (cont = 0; cont < comp; cont++) {
				fread(&b, 1, 1, fileCt2);
				tk2kPlayByte(b);
			}
			// Verifica se é o final do arquivo;
			if (tkcab.actualBlock == tkcab.numberOfBlocks) {
				playSilence(100);
				//
			}
		}
	}
	finishWaveFile();
	fclose(fileCt2);
	return 0;
}
