/*  wav2ct2 - Utilitário para ler arquivos de áudio e interpretar
 *            no formato do TK2000, gerando arquivo .CT2
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
 * wav2ct2.c
 *
 *  Created on: 01/05/2011
 *      Author: Fabio Belavenuto
 */
//#define DEBUG1
//#define DEBUG2

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wav.h"
#include "ct2.h"
#include "functions.h"

// Definições
#define VERSAO "0.1"
#define BIT_LIMIAR  30		// limiar 2 ciclos
#define CABB_LIMIAR 23		// limiar 1 ciclo

// Variáveis
int limiar_sync = CABB_LIMIAR;
int limiar_bit  = BIT_LIMIAR;
char separado = 0, binarios = 0, incompleto = 0;
unsigned int tamLens, posLens = 0, contArqs = 0;
unsigned char *lens;
struct arqTK2000 *arqs[100];
char *arqWav = NULL, *arqCt2 = NULL;
FILE *fileLog = stdout;

// ============================================================================
int getLens(FILE *f) {
	short ub;
	int c = 0, p = 0, s = 1024576;
	char sinal, sinalA = -1;

	lens = (unsigned char *)malloc(s);
	while (!feof(f)) {
		fread(&ub, 2, 1, f);
		sinal = (ub < 0) ? -1 : 1;
		if (sinal != sinalA) {
			sinalA = sinal;
			if (c < 80)
				lens[p++] = c;
			if (p >= s) {
				s += 1024576;
				lens = (unsigned char *)realloc(lens, s);
			}
			c = 0;
		}
		c++;
	}
	return p;
}

// ============================================================================
int sincroniza() {
	int c, b = 0;

	// espera 16 ciclos do cabecalho B
	while (posLens < tamLens) {
		c = lens[posLens++];
#ifdef DEBUG2
		printf("%d,", c);
#endif
		if (c > limiar_sync) {
			b++;
		} else {
			b = 0;
		}
		if (b > 16)
			break;
	}
	if (posLens >= tamLens)
		return -1;
	// espera restante do cabecalho B
	while (posLens < tamLens) {
		c = lens[posLens++];
#ifdef DEBUG2
		printf("%d,", c);
#endif
		if (c < limiar_sync)
			break;
	}
	if (posLens >= tamLens)
		return -1;
	posLens++;	// ignora meio-ciclo
#ifdef DEBUG2
	printf("sinc\n");
#endif
	return 0;
}

// ============================================================================
int lerByte() {
	int i, c;
	int r = 0;

	for (i = 0; i < 8; i++) {
		r <<= 1;
		c = lens[posLens++];
#ifdef DEBUG2
		printf("BIT: %d ", c);
#endif
		c += lens[posLens++];
#ifdef DEBUG2
		printf("%d = %d\n", lens[posLens-1], c);
#endif

#ifdef DEBUG1
		printf("%d ", c);
#endif
		if (c > limiar_bit)
			r |= 1;
	}
	return r;
}

// ============================================================================
int lerBloco(char *buffer, int len, int *cs) {
	int cb = 0, bb;
	unsigned char b;

	while (cb < len) {
		bb = lerByte();
		if (bb < 0) {
			fprintf(fileLog, "\nErro lendo byte\n");
			return -1;
		}
		b = (unsigned char)bb;
		buffer[cb++] = b;
		*cs = *cs ^ b;
	}
	return cb;
}

// =============================================================================
int salvaArquivosCT2() {
	FILE *fileCt2 = NULL, *fileBin = NULL;
	char arqSaida[127], nomeA[7];
	char mc, *p, *buffer = NULL;
	unsigned short sh;
	unsigned int i;
	int na, t, tamBuffer;

	if (!arqCt2) {
		arqCt2 = strdup(arqWav);
	}
	p = (char *)(arqCt2 + strlen(arqCt2));
	while(--p > arqCt2) {
		if (*p == '.') {
			*p = '\0';
			break;
		}
	}

	fprintf(fileLog, "\nInterpretado %d arquivos:\n", contArqs);
	for (i = 0; i < contArqs; i++) {
		struct arqTK2000 *a = arqs[i];
		mc = 0;
		if (a->endInicial == a->endFinal) {
			mc = 1;
		} else {
			for(t = 0; t < a->totalBlocos; t++) {
				if (a->mapa[t] == 0) {
					mc = 1;
					t = a->totalBlocos;
				} else if(a->mapa[t] == 2) {
					mc = 2;
					t = a->totalBlocos;
				}
			}
		}
		fprintf(fileLog, "'%s' de 0x%.2X blocos com endereco de 0x%.4X a 0x%.4X ", a->nome, a->totalBlocos, a->endInicial, a->endFinal);
		if (mc == 1)
			fprintf(fileLog, "- INCOMPLETO");
		else if (mc == 2)
			fprintf(fileLog, "- ERRO CHECKSUM");
		fprintf(fileLog, "\n");
		if (mc != 1 || incompleto == 1) {
			a->completo = true;
			trim(a->nome);
			if (!strlen(a->nome)) {
				strcpy(a->nome, "NONAME");
			}
			if (separado) {
				if (strcmp(a->nome, nomeA)) {
					strcpy(nomeA, a->nome);
					if (fileCt2) {
						fclose(fileCt2);
						fileCt2 = NULL;
					}
					sprintf(arqSaida, "%s_%s.ct2", extractFileName(arqCt2), a->nome);
					cleanFN(arqSaida);
					if (!(fileCt2 = fopen(arqSaida, "wb"))) {
						fprintf(stderr, "Erro ao abrir arquivo %s\n", arqSaida);
						return -1;
					}
					fwrite(CT2_MAGIC, 1, 4, fileCt2);
				}
			} else if (fileCt2 == NULL) {
				sprintf(arqSaida, "%s.ct2", extractFileName(arqCt2));
				if (!(fileCt2 = fopen(arqSaida, "wb"))) {
					fprintf(stderr, "Erro ao abrir arquivo %s\n", arqSaida);
					return -1;
				}
				fwrite(CT2_MAGIC, 1, 4, fileCt2);
			}
			tamBuffer = calculateCt2BufferSize(a);
			if (tamBuffer) {
				buffer = (char *)malloc(tamBuffer);
				makeCt2File(a, buffer);
				fwrite(buffer, 1, tamBuffer, fileCt2);
				free(buffer);
			}
			if (binarios) {
				sprintf(arqSaida, "%s.b", a->nome);
				cleanFN(arqSaida);
				na = 0;
				while(true) {
					if ((fileBin = fopen(arqSaida, "rb"))) {
						na++;
						fclose(fileBin);
						sprintf(arqSaida, "%s_%d.b", a->nome, na);
						cleanFN(arqSaida);
					} else
						break;
				};
				if (!(fileBin = fopen(arqSaida, "wb"))) {
					fprintf(stderr, "Erro ao abrir arquivo %s\n", arqSaida);
					return -1;
				}
				// salvar endereco inicial e tamanho
				sh = a->endInicial;
				fwrite(&sh, 1, sizeof(short), fileBin);
				sh = a->tamanho;
				fwrite(&sh, 1, sizeof(short), fileBin);
				fwrite(a->dados, 1, a->tamanho, fileBin);
				fclose(fileBin);
			}
		}
		free(a->mapa);
		free(a->dados);
	}

	if (fileCt2)
		fclose(fileCt2);
	return 0;
}

// =============================================================================
void mostraUso(char *nomeprog) {
	fprintf(stderr, "\n");
	fprintf(stderr, "%s - Utilitario para interpretar arquivo .wav e converter\n", nomeprog);
	fprintf(stderr, "     para .ct2, padrao TK2000. Versao %s\n\n", VERSAO);
	fprintf(stderr, "  Uso:\n");
	fprintf(stderr, "    %s [opcoes] <arquivo>\n\n", nomeprog);
	fprintf(stderr, "  Opcoes:\n");
	fprintf(stderr, "  -o <arq.>   - Salva resultado nesse arquivo.\n");
	fprintf(stderr, "  -s          - Separar arquivos diferentes.\n");
	fprintf(stderr, "  -c          - Ignora erros de checksum.\n");
	fprintf(stderr, "  -i          - Ignorar arquivos com nome diferente do atual.\n");
	fprintf(stderr, "  -n          - Salva arquivos incompletos.\n");
	fprintf(stderr, "  -l          - Gerar mais logs.\n");
	fprintf(stderr, "  -k          - Salvar log em arquivo.\n");
	fprintf(stderr, "  -b          - Salvar arquivos '.b'.\n");
	fprintf(stderr, "  -q <tempo>  - Configura tempo do limiar de sincronismo\n");
	fprintf(stderr, "                Padrao: %d\n", limiar_sync);
	fprintf(stderr, "  -w <tempo>  - Configura tempo do limiar de bit\n");
	fprintf(stderr, "                Padrao: %d\n", limiar_bit);
	fprintf(stderr, "\n");

	exit(0);
}

// =============================================================================
int main(int argc, char *argv[]) {
	FILE *fileWav = NULL;
	char nR, ics = 0, logs = 0, loga = 0;
	int i, c = 1, b, ct, cb, da = 0, ni, ign = 0, posSync;
	TWaveCab waveCab;
	char temp[1024], nome[7], *p;
	TTKCab tkcab;
	TTKEnd tkend;
	struct arqTK2000 arq;

	if (argc < 2)
		mostraUso(argv[0]);

	// Interpreta linha de comando
	while (c < argc) {
		if (argv[c][0] == '-' || argv[c][0] == '/') {
			if (c + 1 == argc) {
				if (argv[c][1] == 'o' || argv[c][1] == 'q' || argv[c][1] == 'w')
				fprintf(stderr, "Falta parametro para a opcao %s", argv[c]);
				return 1;
			}
			switch (argv[c][1]) {

			case 'o':
				arqCt2 = argv[++c];
				break;

			case 's':
				separado = 1;
				break;

			case 'c':
				ics = 1;
				break;

			case 'n':
				incompleto = 1;
				break;

			case 'l':
				logs = 1;
				break;

			case 'k':
				loga = 1;
				break;

			case 'b':
				binarios = 1;
				break;

			case 'i':
				ign = 1;
				break;

			case 'q':
				limiar_sync = atoi(argv[++c]);
				break;

			case 'w':
				limiar_bit = atoi(argv[++c]);
				break;

			default:
				fprintf(stderr, "Opcao invalida: %s\n", argv[c]);
				return 1;
				break;
			} // switch
		} else
			arqWav = argv[c];
		c++;
	}

	if (!arqWav) {
		fprintf(stderr, "Falta nome do arquivo.\n");
		return 1;
	}

	if (!(fileWav = fopen(arqWav, "rb"))) {
		fprintf(stderr, "Erro ao abrir arquivo %s\n", arqWav);
		return -1;
	}
	fread(&waveCab, 1, sizeof(TWaveCab), fileWav);

	if (loga) {
		sprintf(temp, "%s.log", arqWav);
		fileLog = fopen(temp, "w");
		if (!fileLog)
			fileLog = stdout;
	}

	if (strncmp((const char *)waveCab.GroupID, "RIFF", 4)) {
		fclose(fileWav);
		fprintf(stderr, "Formato nao reconhecido.\n");
		return -1;
	}

	if (waveCab.NumChannels != 1) {
		fclose(fileWav);
		fprintf(stderr, "Audio deve ser mono.\n");
		exit(-1);
	}
	if (waveCab.SamplesPerSec != 44100) {
		fclose(fileWav);
		fprintf(stderr, "Formato deve ser 44100 Samples per second.\n");
		exit(-1);
	}
	if (waveCab.BitsPerSample != 16) {
		fclose(fileWav);
		fprintf(stderr, "Formato deve ser 16 bits\n");
		exit(-1);
	}

	fprintf(fileLog, "Decodificando '%s'\n", arqWav);
	fprintf(fileLog, "Calculando comprimentos...\n");
	tamLens = getLens(fileWav);
	fclose(fileWav);

	memset(&arq, 0, sizeof(struct arqTK2000));
	nome[6] = 0;
	while (1) {
		if (!nR) {
			if (sincroniza())
				break;
		}
		posSync = posLens;

		ct = 0xFF;
		// ler 8 bytes (cabecalho)
		cb = lerBloco(temp, 8, &ct);
		if (cb < 8) {
			if (logs)
				fprintf(fileLog, "Cabecalho nao achado\n");
			break;
		}

		memcpy(&tkcab, temp, sizeof(TTKCab));
		ni = 0;
		for (i=0; i<6; i++) {
			nome[i] = tkcab.Nome[i] & 0x7F;
			if (nome[i] < ' ' || nome[i] > 'Z')
				ni = 1;
		}
		if (ni) {
			if (nR < 8) {
				if (!nR && logs)
					fprintf(fileLog, "Erro de decodificacao, tentando auto-recuperar.\n");
				nR++;
				posLens = posSync-1;
			} else {
				if (logs)
					fprintf(fileLog, "Erro ao tentar auto-recuperar\n");
				nR = 0;
			}
			continue;
		}
		nR = 0;
		if (strlen(arq.nome) == 0) {
			strcpy(arq.nome, nome);
		}
		if (strcmp(nome, arq.nome)) {
			if (ign) {
				if (logs)
					fprintf(fileLog, "Achado outro arquivo: '%s', arquivo atual '%s', ignorando.\n", nome, arq.nome);
				continue;
			} else {
				if (arq.dados)
					free(arq.dados);
				if (arq.mapa)
					free(arq.mapa);
				memset(&arq, 0, sizeof(struct arqTK2000));
				da = 0;
				strcpy(arq.nome, nome);
			}
		}

		if (tkcab.BlocoAtual == 0) {
			if (logs)
				fprintf(fileLog, "%s 0x%.2X 0x%.2X", nome, tkcab.TotalBlocos, tkcab.BlocoAtual);
			cb = lerBloco(temp, sizeof(TTKEnd), &ct);
			memcpy(&tkend, temp, sizeof(TTKEnd));
			da = tkend.EndFinal - tkend.EndInicial + 1;
			if (logs)
				fprintf(fileLog, " de 0x%.4X a 0x%.4X, %d bytes\n", tkend.EndInicial, tkend.EndFinal, da);
			if (da > 0) {
				memset(&arq, 0, sizeof(struct arqTK2000));
				arq.endInicial  = tkend.EndInicial;
				arq.endFinal    = tkend.EndFinal;
				arq.totalBlocos = tkcab.TotalBlocos;
				arq.mapa        = (char *)malloc(tkcab.TotalBlocos+1);
				arq.tamanho     = da;
				arq.dados       = (char *)malloc(da+1);
				memset(arq.mapa, 0, tkcab.TotalBlocos);
				memset(arq.dados, 0, da);
			} else {
				if (logs)
					fprintf(fileLog, "Erro na decodificacao dos enderecos, ignorando.\n");
				if (arq.dados)
					free(arq.dados);
				if (arq.mapa)
					free(arq.mapa);
				memset(&arq, 0, sizeof(struct arqTK2000));
				da = 0;
				continue;
			}
		} else if (tkcab.BlocoAtual <= tkcab.TotalBlocos) {
			if (logs)
				fprintf(fileLog, "%s 0x%.2X 0x%.2X\n", nome, tkcab.TotalBlocos, tkcab.BlocoAtual);
			if (da == 0) {
				if (logs)
					fprintf(fileLog, "Falta bloco zero, ignorando.\n");
				if (arq.dados)
					free(arq.dados);
				if (arq.mapa)
					free(arq.mapa);
				memset(&arq, 0, sizeof(struct arqTK2000));
				continue;
			}
			if (tkcab.TotalBlocos != arq.totalBlocos) {
				if (logs)
					fprintf(fileLog, "Total de blocos não corresponde ao arquivo atual, ignorando.\n");
				if (arq.dados)
					free(arq.dados);
				if (arq.mapa)
					free(arq.mapa);
				memset(&arq, 0, sizeof(struct arqTK2000));
				da = 0;
				continue;
			}
			if (da > 256)
				cb = 256;
			else
				cb = da;
			da -= cb;
			cb = lerBloco(temp, cb, &ct);
		} else {
			if (logs)
				fprintf(fileLog, "Bloco atual maior do que total de blocos, ignorando.\n");
			if (arq.dados)
				free(arq.dados);
			if (arq.mapa)
				free(arq.mapa);
			memset(&arq, 0, sizeof(struct arqTK2000));
			da = 0;
			continue;
		}
		// ler caracter de checksum
		lerBloco(temp+cb, 1, &ct);
		if (ct) {
			if (logs)
				fprintf(fileLog, "Erro de checksum: %02X, lido %d bytes\n", ct, cb);
		}
		if (tkcab.BlocoAtual && (cb > 0) && arq.dados && arq.mapa) {
			b = 256*(tkcab.BlocoAtual-1);
			p = arq.dados + b;
			if ((b + cb) > arq.tamanho)
				cb = arq.tamanho - b;
			if (cb > 0)
				memmove(p, temp, cb);
			if (ct == 0) {		// Se checksum está correto
				arq.mapa[tkcab.BlocoAtual-1] = 1;
			} else if (ics) {	// Checksum está errado, mas usuário escolheu ignorá-lo
				arq.mapa[tkcab.BlocoAtual-1] = 2;
			}
		}
		// Término do arquivo
		if (tkcab.BlocoAtual == tkcab.TotalBlocos) {
			arqs[contArqs] = (struct arqTK2000 *)malloc(sizeof(struct arqTK2000));
			memcpy(arqs[contArqs], &arq, sizeof(struct arqTK2000));
			contArqs++;
			memset(&arq, 0, sizeof(struct arqTK2000));
		}
	}
	fclose(fileWav);
	if (contArqs)
		salvaArquivosCT2();
	fprintf(fileLog, "\n");
	return 0;
}
