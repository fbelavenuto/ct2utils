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
//#define DEBUG2

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "functions.h"
#include "version.h"
#include <wav.h>
#include <ct2.h>

// Defines
#define THRESHOLD_BIT  30
#define THRESHOLD_BHDR 23

// Structs
struct AddInfo {
	char *map;
	int isComplete;
};

// Variables
static int syncThreshold = THRESHOLD_BHDR;
static int bitThreshold  = THRESHOLD_BIT;
static char saveDifferents = 0, saveBinaries = 0, saveImcomplete = 0;
static char applesoftFormat = 0;
static unsigned int sizeLenghts, posLengths = 0, numberOfBinaries = 0;
static unsigned char *lens;
static struct Tk2kBinary *binaries[100];
static struct AddInfo *addInfos[100];
static char *wavFilename = NULL, *ct2Filename = NULL;
static FILE *fileLog = NULL;

// Private functions
/*****************************************************************************/
static int getLengths(FILE *f) {
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

/*****************************************************************************/
static int syncHeader() {
	int c, b = 0;

	// Wait 16 cycles of header B
	while (posLengths < sizeLenghts) {
		c = lens[posLengths++];
#ifdef DEBUG2
		printf("%d,", c);
#endif
		if (c > syncThreshold) {
			b++;
		} else {
			b = 0;
		}
		if (b > 16)
			break;
	}
	if (posLengths >= sizeLenghts) {
		// Header B not found
		return 0;
	}
	// Wait remaining of B header
	while (posLengths < sizeLenghts) {
		c = lens[posLengths++];
#ifdef DEBUG2
		printf("%d,", c);
#endif
		if (c < syncThreshold)
			break;
	}
	if (posLengths >= sizeLenghts)
		return -1;
	posLengths++;	// skip half cycle
#ifdef DEBUG2
	printf("sync\n");
#endif
	return 1;
}

/*****************************************************************************/
static int readByte() {
	int i, c;
	int r = 0;

	for (i = 0; i < 8; i++) {
		r <<= 1;
		c = lens[posLengths++];
#ifdef DEBUG2
		printf("BIT: %d ", c);
#endif
		c += lens[posLengths++];
#ifdef DEBUG2
		printf("%d = %d\n", lens[posLengths-1], c);
#endif

#ifdef DEBUG1
		printf("%d ", c);
#endif
		if (c > bitThreshold)
			r |= 1;
	}
	return r;
}

/*****************************************************************************/
static int readBlock(char *buffer, int len, int *cs) {
	int cb = 0, bb;
	unsigned char b;

	while (cb < len) {
		bb = readByte();
		if (bb < 0) {
			fprintf(fileLog, "\nError reading byte.\n");
			return -1;
		}
		b = (unsigned char)bb;
		buffer[cb++] = b;
		*cs = *cs ^ b;
	}
	return cb;
}

/*****************************************************************************/
static char *findBinaryName(char *name, unsigned short addr) {
	char *filename = (char *)malloc(_MAX_PATH);
	int na = 0;
	FILE *ft = NULL;

	if (applesoftFormat == 1) {
		sprintf(filename, "%s.b", name);
	} else {
		sprintf(filename, "%s#06%04X", name, addr);
	}
	cleanFN(filename);
	while(1) {
		if ((ft = fopen(filename, "rb"))) {
			na++;
			fclose(ft);
			if (applesoftFormat == 1) {
				sprintf(filename, "%s_%d.b", name, na);
			} else {
				sprintf(filename, "%s_%d#06%04X", name, na, addr);
			}
			cleanFN(filename);
		} else
			break;
	};
	return filename;
}

/*****************************************************************************/
static int saveCT2Files() {
	FILE *fileCt2 = NULL, *fileBin = NULL;
	char outFilename[_MAX_PATH], nameA[7], *binName;
	char *ctfn, mc, *buffer = NULL;
	unsigned short sh;
	unsigned int i;
	int t, tamBuffer;

	fprintf(fileLog, "\nFound %d binaries:\n", numberOfBinaries);

	if (!ct2Filename) {
		char *p = onlyPath(wavFilename);
		char *t = wavFilename + strlen(p);
		ct2Filename = (char *)malloc(strlen(t) + 1);
		strcpy(ct2Filename, t);
		free(p);
	}
	// Remove extension from CT2 file name
	ctfn = ct2Filename;
	ct2Filename = withoutExt(ctfn);
	free(ctfn);

	for (i = 0; i < numberOfBinaries; i++) {
		struct Tk2kBinary *binary = binaries[i];
		struct AddInfo *addInfo = addInfos[i];
		mc = 0;
		if (binary->initialAddr == binary->endAddr) {
			mc = 1;
		} else {
			for(t = 0; t < binary->numberOfBlocks; t++) {
				if (addInfo->map[t] == 0) {
					mc = 1;
					t = binary->numberOfBlocks;
				} else if(addInfo->map[t] == 2) {
					mc = 2;
					t = binary->numberOfBlocks;
				}
			}
		}
		fprintf(fileLog, "'%s' from 0x%.2X blocks, from 0x%.4X to 0x%.4X ", binary->name, binary->numberOfBlocks, binary->initialAddr, binary->endAddr);
		if (mc == 1) {
			fprintf(fileLog, "- INCOMPLETE");
		} else if (mc == 2) {
			fprintf(fileLog, "- CHECKSUM ERROR");
		}
		fprintf(fileLog, "\n");
		if (mc != 1 || saveImcomplete == 1) {
			addInfo->isComplete = 1;
			trim(binary->name);
			if (!strlen(binary->name)) {
				strcpy(binary->name, "NONAME");
			}
			if (saveDifferents) {
				if (strcmp(binary->name, nameA)) {
					strcpy(nameA, binary->name);
					if (fileCt2) {
						fclose(fileCt2);
						fileCt2 = NULL;
					}
					sprintf(outFilename, "%s_%s.ct2", ct2Filename, binary->name);
					cleanFN(outFilename);
					if (!(fileCt2 = fopen(outFilename, "wb"))) {
						fprintf(stderr, "Error creating file '%s'\n", outFilename);
						return -1;
					}
					fwrite(CT2_MAGIC, 1, 4, fileCt2);
				}
			} else if (fileCt2 == NULL) {
				sprintf(outFilename, "%s.ct2", ct2Filename);
				if (!(fileCt2 = fopen(outFilename, "wb"))) {
					fprintf(stderr, "Error creating file '%s'\n", outFilename);
					return -1;
				}
				fwrite(CT2_MAGIC, 1, 4, fileCt2);
			}
			tamBuffer = calcCt2BufferSize(binary->size);
			if (tamBuffer) {
				buffer = (char *)malloc(tamBuffer);
				createOneCt2Binary(binary, buffer);
				fwrite(buffer, 1, tamBuffer, fileCt2);
				free(buffer);
			}
			if (saveBinaries) {
				binName = findBinaryName(binary->name, binary->initialAddr);
				if (!(fileBin = fopen(binName, "wb"))) {
					fprintf(stderr, "Error creating file '%s'\n", binName);
					free(binName);
					return -1;
				}
				free(binName);
				if (applesoftFormat == 1) {
					sh = binary->initialAddr;
					fwrite(&sh, 1, sizeof(short), fileBin);
					sh = binary->size;
					fwrite(&sh, 1, sizeof(short), fileBin);
				}
				fwrite(binary->data, 1, binary->size, fileBin);
				fclose(fileBin);
			}
		}
		free(addInfo->map);
		free(binary->data);
	}

	if (fileCt2)
		fclose(fileCt2);
	return 0;
}

/*****************************************************************************/
static void usage(char *progName) {
	fprintf(stderr, "\n");
	fprintf(stderr, "%s - Utility to convert audio(WAVE) to CT2 file (TK2000 cassete). Version %s\n\n", progName, VERSION);
	fprintf(stderr, "  Usage:\n");
	fprintf(stderr, "    %s [options] <filename>\n\n", progName);
	fprintf(stderr, "  Options:\n");
	fprintf(stderr, "  -o <file>  - Output to this filename.\n");
	fprintf(stderr, "  -s         - Separe files with different names.\n");
	fprintf(stderr, "  -c         - Ignore checksum errors.\n");
	fprintf(stderr, "  -i         - Skip files with name different from first.\n");
	fprintf(stderr, "  -n         - Force save incomplete files.\n");
	fprintf(stderr, "  -l         - More verbose.\n");
	fprintf(stderr, "  -k         - Save log to a file.\n");
	fprintf(stderr, "  -b         - Save each binary found in separate file.\n");
	fprintf(stderr, "  -a         - Binary format in TKDOS format (requires -b).\n");
	fprintf(stderr, "  -q <value> - Set sync time threshold\n");
	fprintf(stderr, "               Standard: %d\n", syncThreshold);
	fprintf(stderr, "  -w <value> - Set bit time threshold\n");
	fprintf(stderr, "               Standard: %d\n", bitThreshold);
	fprintf(stderr, "  -h or -?   - Show this usage.\n");
	fprintf(stderr, "\n");
}

// Public functions

/*****************************************************************************/
int main(int argc, char *argv[]) {
	FILE *fileWav = NULL;
	char c, sR, ics = 0, verbose = 0, logSave = 0;
	int i, b, ct, cb, da = 0, ni, ign = 0, posSync;
	TWaveCab waveCab;
	char temp[1024], name[7], *p;
	struct STKCab tkcab;
	struct STKAddr tkend;
	struct Tk2kBinary actBinary;
	struct AddInfo actAddInfo;

	opterr = 1;
	while((c = getopt(argc, argv, "o:scinlkbq:w:h?")) != -1) {
		switch (c) {
			case 'o':
				ct2Filename = strdup(optarg);
				break;

			case 's':
				saveDifferents = 1;
				break;

			case 'c':
				ics = 1;
				break;

			case 'i':
				ign = 1;
				break;

			case 'n':
				saveImcomplete = 1;
				break;

			case 'l':
				verbose = 1;
				break;

			case 'k':
				logSave = 1;
				break;

			case 'b':
				saveBinaries = 1;
				break;

			case 'a':
				applesoftFormat = 1;
				break;

			case 'q':
				syncThreshold = atoi(optarg);
				break;

			case 'w':
				bitThreshold = atoi(optarg);
				break;

			case 'h':
			case '?':
				usage(argv[0]);
				return 0;

		} // switch
	}

	if(argc - optind != 1) {
		usage(argv[0]);
		return 1;
	}
	wavFilename = argv[optind];
	if (!(fileWav = fopen(wavFilename, "rb"))) {
		fprintf(stderr, "Error opening file '%s'.\n", wavFilename);
		return 1;
	}
	fread(&waveCab, 1, sizeof(TWaveCab), fileWav);

	fileLog = stdout;
	if (logSave) {
		sprintf(temp, "%s.log", wavFilename);
		fileLog = fopen(temp, "w");
		if (!fileLog)
			fileLog = stdout;
	}

	if (strncmp((const char *)waveCab.groupID, "RIFF", 4)) {
		fclose(fileWav);
		fprintf(stderr, "Wave file not recognized.\n");
		return -1;
	}

	if (waveCab.numChannels != 1) {
		fclose(fileWav);
		fprintf(stderr, "Wave file must be mono.\n");
		exit(-1);
	}
	if (waveCab.samplesPerSec != 44100) {
		fclose(fileWav);
		fprintf(stderr, "Wave file must be 44100 sps.\n");
		exit(-1);
	}
	if (waveCab.bitsPerSample != 16) {
		fclose(fileWav);
		fprintf(stderr, "Wave file must be 16 bits\n");
		exit(-1);
	}

	fprintf(fileLog, "Decoding '%s'\n", wavFilename);
	fprintf(fileLog, "Calculating lengths...\n");
	sizeLenghts = getLengths(fileWav);
	fclose(fileWav);

	memset(&actBinary, 0, sizeof(struct Tk2kBinary));
	memset(&actAddInfo, 0, sizeof(struct AddInfo));
	memset(name, 0, 7);
	sR = 0;
	while (1) {
		if (sR == 0) {
			if (syncHeader() == 0) {
				// Error
				break;
			}
		}
		posSync = posLengths;

		ct = 0xFF;
		// Read header data
		cb = readBlock(temp, 8, &ct);
		if (cb < 8) {
			if (verbose)
				fprintf(fileLog, "Header data not found.\n");
			break;
		}

		memcpy(&tkcab, temp, sizeof(struct STKCab));
		ni = 0;
		for (i=0; i<6; i++) {
			name[i] = tkcab.name[i] & 0x7F;
			if (name[i] < ' ' || name[i] > 'Z')
				ni = 1;
		}
		if (ni) {
			if (sR < 8) {
				if (sR == 0 && verbose) {
					fprintf(fileLog, "Decoding error, try self recovering.\n");
				}
				sR++;
				posLengths = posSync-1;
			} else {
				if (verbose) {
					fprintf(fileLog, "Error trying to self recovering.\n");
				}
				sR = 0;
			}
			continue;
		}
		sR = 0;
		if (strlen(actBinary.name) == 0) {
			strcpy(actBinary.name, name);
		}
		if (strcmp(name, actBinary.name)) {
			if (ign) {
				if (verbose)
					fprintf(fileLog, "Found another binary: '%s', actual name: '%s'. Ignoring.\n", name, actBinary.name);
				continue;
			} else {
				if (actBinary.data)
					free(actBinary.data);
				if (actAddInfo.map)
					free(actAddInfo.map);
				memset(&actBinary, 0, sizeof(struct Tk2kBinary));
				memset(&actAddInfo, 0, sizeof(struct AddInfo));
				da = 0;
				strcpy(actBinary.name, name);
			}
		}

		if (tkcab.actualBlock == 0) {
			if (verbose)
				fprintf(fileLog, "%s 0x%.2X 0x%.2X", name, tkcab.numberOfBlocks, tkcab.actualBlock);
			cb = readBlock(temp, sizeof(struct STKAddr), &ct);
			memcpy(&tkend, temp, sizeof(struct STKAddr));
			da = tkend.endAddr - tkend.initialAddr + 1;
			if (verbose)
				fprintf(fileLog, " from 0x%.4X to 0x%.4X, %d bytes\n", tkend.initialAddr, tkend.endAddr, da);
			if (da > 0) {
				memset(&actBinary, 0, sizeof(struct Tk2kBinary));
				memset(&actAddInfo, 0, sizeof(struct AddInfo));
				actBinary.initialAddr    = tkend.initialAddr;
				actBinary.endAddr        = tkend.endAddr;
				actBinary.numberOfBlocks = tkcab.numberOfBlocks;
				actBinary.size           = da;
				actBinary.data           = (char *)malloc(da+1);
				actAddInfo.map            = (char *)malloc(tkcab.numberOfBlocks+1);
				memset(actAddInfo.map, 0, tkcab.numberOfBlocks);
				memset(actBinary.data, 0, da);
			} else {
				if (verbose)
					fprintf(fileLog, "Error decoding addresses, ignoring.\n");
				if (actBinary.data)
					free(actBinary.data);
				if (actAddInfo.map)
					free(actAddInfo.map);
				memset(&actBinary, 0, sizeof(struct Tk2kBinary));
				memset(&actAddInfo, 0, sizeof(struct AddInfo));
				da = 0;
				continue;
			}
		} else if (tkcab.actualBlock <= tkcab.numberOfBlocks) {
			if (verbose)
				fprintf(fileLog, "%s 0x%.2X 0x%.2X\n", name, tkcab.numberOfBlocks, tkcab.actualBlock);
			if (da == 0) {
				if (verbose)
					fprintf(fileLog, "Missing block 0, ignoring.\n");
				if (actBinary.data)
					free(actBinary.data);
				if (actAddInfo.map)
					free(actAddInfo.map);
				memset(&actBinary, 0, sizeof(struct Tk2kBinary));
				memset(&actAddInfo, 0, sizeof(struct AddInfo));
				continue;
			}
			if (tkcab.numberOfBlocks != actBinary.numberOfBlocks) {
				if (verbose)
					fprintf(fileLog, "Total blocks do not match current file, ignoring.\n");
				if (actBinary.data)
					free(actBinary.data);
				if (actAddInfo.map)
					free(actAddInfo.map);
				memset(&actBinary, 0, sizeof(struct Tk2kBinary));
				memset(&actAddInfo, 0, sizeof(struct AddInfo));
				da = 0;
				continue;
			}
			if (da > 256)
				cb = 256;
			else
				cb = da;
			da -= cb;
			cb = readBlock(temp, cb, &ct);
		} else {
			if (verbose)
				fprintf(fileLog, "Current block greater than total blocks, ignoring.\n");
			if (actBinary.data)
				free(actBinary.data);
			if (actAddInfo.map)
				free(actAddInfo.map);
			memset(&actBinary, 0, sizeof(struct Tk2kBinary));
			memset(&actAddInfo, 0, sizeof(struct AddInfo));
			da = 0;
			continue;
		}
		// Read checksum
		readBlock(temp+cb, 1, &ct);
		if (ct) {
			if (verbose)
				fprintf(fileLog, "Checksum error: %02X, read %d bytes\n", ct, cb);
		}
		if (tkcab.actualBlock && (cb > 0) && actBinary.data && actAddInfo.map) {
			b = 256*(tkcab.actualBlock-1);
			p = actBinary.data + b;
			if ((b + cb) > actBinary.size)
				cb = actBinary.size - b;
			if (cb > 0)
				memmove(p, temp, cb);
			if (ct == 0) {		// Checksum ok
				actAddInfo.map[tkcab.actualBlock-1] = 1;
			} else if (ics) {	// Checksum not ok, but user force to ignore
				actAddInfo.map[tkcab.actualBlock-1] = 2;
			}
		}
		// End of binary
		if (tkcab.actualBlock == tkcab.numberOfBlocks) {
			binaries[numberOfBinaries] = (struct Tk2kBinary *)malloc(sizeof(struct Tk2kBinary));
			addInfos[numberOfBinaries] = (struct AddInfo *)malloc(sizeof(struct AddInfo));
			memcpy(binaries[numberOfBinaries], &actBinary, sizeof(struct Tk2kBinary));
			memcpy(addInfos[numberOfBinaries], &actAddInfo, sizeof(struct AddInfo));
			numberOfBinaries++;
			memset(&actBinary, 0, sizeof(struct Tk2kBinary));
			memset(&actAddInfo, 0, sizeof(struct AddInfo));
		}
	}
	fclose(fileWav);
	if (numberOfBinaries) {
		saveCT2Files();
	}
	fprintf(fileLog, "\n");
	return 0;
}
