/* ct2utils - Utilitarios para manipular arquivos CT2 (Cassete TK2000)
 *
 * Copyright 2011-2020 Fábio Belavenuto
 *
 * Este arquivo é distribuido pela Licença Pública Geral GNU.
 * Veja o arquivo "Licenca.txt" distribuido com este software.
 *
 * ESTE SOFTWARE NÃO OFERECE NENHUMA GARANTIA
 */

//#define DEBUG1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "version.h"
#include "wav.h"
#include "ct2.h"

// Definições

// Variáveis
int  DurSilencioP    = DURSILENP;
int  DurSilencioS    = DURSILENS;
int  DurCabA         = DURCABA;
int  TaxaAmostragem  = TAXAAMOST;
int  totalDados      = 0;

// Funções

// =============================================================================
static void silencio(FILE *fileWav, int duracaoms) {
	int   Total;
	short *buffer;

	Total = (TaxaAmostragem * duracaoms) / 1000;
	buffer = (short *)malloc(Total * sizeof(short) + 1);
	memset(buffer, 0, Total * sizeof(short));
	fwrite(buffer, sizeof(short), Total, fileWav);
	totalDados += Total * sizeof(short);
	free(buffer);
}

// =============================================================================
static void tom(FILE *fileWav, int frequencia, int duracaoms) {
	static short Pico = 32767;
	short *buffer;
	int   c, i;
	int   Total, CicloT, Ciclo1, Ciclo2;

	CicloT = TaxaAmostragem / frequencia;
	Ciclo1 = (int)((double)CicloT / 2 + .5);
	Ciclo2 = CicloT / 2;
	Total = CicloT * duracaoms;
	buffer = (short *)malloc(Total * sizeof(short) + sizeof(short));

	c = i = 0;
	while (c < Total) {
		i = 0;
		while (i++ < Ciclo1) {
			buffer[c++] = Pico;
		}
		Pico = 0-Pico;
		i = 0;
		while (i++ < Ciclo2) {
			buffer[c++] = Pico;
		}
		Pico = 0-Pico;
	}
	fwrite(buffer, sizeof(short), c, fileWav);
	totalDados += c * sizeof(short);
	free(buffer);
}

// =============================================================================
static void mostraUso(char *nomeprog) {
	fprintf(stderr, "\n");
	fprintf(stderr, "%s - Utilitario para gerar arquivo de audio .wav a partir\n", nomeprog);
	fprintf(stderr, "          do arquivo .ct2 formato TK2000. Versao %s\n\n", VERSAO);
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
	int      c = 1, silencioAtual = DurSilencioP;
	int      pos, comp, cont;
	char     temp[1024], b, *p;
	char     *arqCt2    = NULL;
	char     *arqWav    = NULL;
	FILE     *fileCt2   = NULL;
	FILE     *fileWav   = NULL;
	TTKCab   tkcab;
	TTKEnd   tkend;
	TCh      ch;
	TWaveCab waveCab;

	if (argc < 2)
		mostraUso(argv[0]);

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
				arqWav = argv[++c];
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

	if (!arqWav) {
		arqWav = (char *)malloc(strlen(arqCt2) + 1);
		strcpy(arqWav, arqCt2);
		p = (char *)(arqWav + strlen(arqWav));
		while(--p > arqWav) {
			if (*p == '.') {
				strncpy(p, ".wav\0", 5);
				break;
			}
		}
	}

	if (!(fileCt2 = fopen(arqCt2, "rb"))) {
		fprintf(stderr, "Erro ao abrir arquivo %s\n", arqCt2);
		return -1;
	}

	if (!(fileWav = fopen(arqWav, "wb"))) {
		fprintf(stderr, "Erro ao criar arquivo %s", arqWav);
		return 1;
	}


	fread(temp, 1, 4, fileCt2);
	if (strncmp(temp, CT2_MAGIC, 4)) {
		fclose(fileCt2);
		fclose(fileWav);
		fprintf(stderr, "ERRO: Arquivo nao esta no formato .CT2\n");
		return 1;
	}

	memset(&waveCab, 0, sizeof(TWaveCab));

	strcpy((char *)waveCab.GroupID,  "RIFF");
	waveCab.GroupLength    = 0;			// Não fornecido agora
	strcpy((char *)waveCab.TypeID,   "WAVE");
	strcpy((char *)waveCab.FormatID, "fmt ");
	waveCab.FormatLength   = 16;
	waveCab.wFormatTag     = WAVE_FORMAT_PCM;
	waveCab.NumChannels    = 1;
	waveCab.SamplesPerSec  = TaxaAmostragem;
	waveCab.BytesPerSec    = TaxaAmostragem * (16 / 8);
	waveCab.nBlockAlign    = (16 / 8);
	waveCab.BitsPerSample  = 16;
	strcpy((char *)waveCab.DataID,   "data");
	waveCab.DataLength     = 0;			// Não fornecido agora
	fwrite(&waveCab, 1, sizeof(TWaveCab), fileWav);

	// Enquanto não acabar o arquivo
	while (!feof(fileCt2)) {
		// Lê Chunk de 4 bytes
		fread(&ch, 1, 4, fileCt2);
		// Tamanho do chunk
		comp = ch.Tam;
		if (!strncmp((const char*)ch.ID, CT2_CAB_A, 4)) {			// Se for um chunk informando ser cabecalho A
#ifdef DEBUG1
			printf("Cabecalho A\n");
#endif
			silencio(fileWav, silencioAtual);
			tom(fileWav, FREQBIT1, DurCabA);
			silencioAtual = DurSilencioS;
		} else if (!strncmp((const char*)ch.ID, CT2_CAB_B, 4)) {	// Se for um chunk informando ser cabecalho B
#ifdef DEBUG1
			printf("Cabecalho B\n");
#endif
			tom(fileWav, FREQCABB, 30);
			tom(fileWav, FREQBIT0, 1);		// bit 0
		} else if (!strncmp((const char*)ch.ID, CT2_DADOS, 2)) {	// Se for um chunk informando ser dados
#ifdef DEBUG1
			printf("Dados\n");
#endif
			// Toca Dados
			pos = ftell(fileCt2);
			fread(&tkcab, 1, sizeof(TTKCab), fileCt2);
			// Verifica se é o primeiro bloco
			if (tkcab.BlocoAtual == 0) {
				fread(&tkend, 1, sizeof(TTKEnd), fileCt2);
				imprimeInf(&tkcab, &tkend);
#ifdef DEBUG1
			} else {
				printf("bloco %.2X\n", tkcab.BlocoAtual);
#endif
			}
			// Toca Dados
			fseek(fileCt2, pos, SEEK_SET);
			for (cont = 0; cont < comp; cont++) {
				unsigned char mask = 128;		// MSB Primeiro
				fread(&b, 1, 1, fileCt2);
				while(mask) {
					if (b & mask) {
						tom(fileWav, FREQBIT1, 1);		// bit 1
					} else {
						tom(fileWav, FREQBIT0, 1);		// bit 0
					}
					mask >>= 1;
				}

			}
			// Verifica se é o final do arquivo;
			if (tkcab.BlocoAtual == tkcab.TotalBlocos) {
				silencio(fileWav, 100);
				//
			}
		}
	}

	fseek(fileWav, 0, SEEK_SET);
	fread(&waveCab, 1, sizeof(TWaveCab), fileWav);
	waveCab.DataLength = totalDados;
	waveCab.GroupLength = totalDados + sizeof(TWaveCab) - 8;
	fseek(fileWav, 0, SEEK_SET);
	fwrite(&waveCab, 1, sizeof(TWaveCab), fileWav);
	fclose(fileWav);
	fclose(fileCt2);
	return 0;
}
