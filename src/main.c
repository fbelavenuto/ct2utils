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
#include "functions.h"
#include "version.h"
#include <ct2.h>
#include <wav.h>

// Defines

// Enums
enum Commands {
    CMD_CREATE = 0,
    CMD_LIST,
    CMD_EXTRACT,
    CMD_DELETE,
    CMD_INSERT,
    CMD_WAVE
};

// Structs

// Variables
static struct Ct2File *ct2Binaries;
static int applesoftFormat = 0, index = 0;
static char *outputFilename = NULL;
static char *tk2kName = NULL;
static char *outputDirectory = "";
static int sampleRate = 44100;
static enum WaveFormat waveFormat = WF_SINE;

// Private functions

/*****************************************************************************/
static void createCommand(char *filename) {
    FILE              *ct2File;

	if (!(ct2File = fopen(filename, "wb"))) {
		fprintf(stderr, "Error writing file '%s'.\n", filename);
		exit(1);
	}
    fwrite(CT2_MAGIC, 1, 4, ct2File);
    fclose(ct2File);
}

/*****************************************************************************/
static void listCommand() {
    int               i;
    struct Tk2kBinary *binary;

    for (i = 0; i < ct2Binaries->numOfBinaries; i++) {
        binary = ct2Binaries->binaries[i];
        printf("%2d '%s' of 0x%.2X blocks, ", i+1, 
            binary->name, binary->numberOfBlocks);
	    printf("from 0x%.4X to 0x%.4X\n", 
            binary->initialAddr, binary->endAddr);
    }
}

/*****************************************************************************/
static void extractCommand() {
    int               i;
    struct Tk2kBinary *binary;
    char              filenameOut[_MAX_PATH];
    char              binaryFmt[_MAX_PATH];
    FILE              *temp;
    unsigned short	  sizeAddr = 0;

    if (index < 0 || index > ct2Binaries->numOfBinaries) {
        fprintf(stderr, "Index invalid.");
        exit(1);
    }
    if (index == 0 && NULL != outputFilename) {
        fprintf(stderr, "To extract all binaries, do not set output filename.");
        exit(1);
    }
    for (i = 0; i < ct2Binaries->numOfBinaries; i++) {
        binary = ct2Binaries->binaries[i];
        if (index == 0 || index == (i+1)) {
            char *p;
            if (NULL == outputFilename) {
                strcpy(binaryFmt, binary->name);
                p = trim(binaryFmt);
                cleanFN(p);
                sprintf(filenameOut, "%s%s#06%04X", outputDirectory, p, binary->initialAddr);
                // Check if filename exists
                if ((temp = fopen(filenameOut, "rb"))) {
                    fclose(temp);
                    sprintf(filenameOut, "%s%s_%d#06%04X", outputDirectory, p, i, binary->initialAddr);
                }
            } else {
                strcpy(filenameOut, outputFilename);
            }
            if (!(temp = fopen(filenameOut, "wb"))) {
                fprintf(stderr, "Error creating file '%s'.", filenameOut);
                exit(1);
            }
            if (applesoftFormat == 1) {
                sizeAddr = binary->initialAddr;
                fwrite(&sizeAddr, 1, sizeof(short), temp);
                sizeAddr = binary->size;
                fwrite(&sizeAddr, 1, sizeof(short), temp);
            }
            fwrite(binary->data, 1, binary->size, temp);
            fclose(temp);
        }
    }
}

/*****************************************************************************/
static void deleteCommand(char *filename) {
    FILE              *ct2File;
    struct Tk2kBinary *binary;
    int               i, bufSize, found;
    char              *buffer;

    if (index == 0 && NULL == tk2kName) {
        fprintf(stderr, "Index and TK2000 name not informed.");
        exit(1);
    }
    if (index < 0 || index > ct2Binaries->numOfBinaries) {
        fprintf(stderr, "Index invalid.");
        exit(1);
    }
    found = 0;
    for (i = 0; i < ct2Binaries->numOfBinaries; i++) {
        binary = ct2Binaries->binaries[i];
        if (memcmp(tk2kName, binary->name, strlen(tk2kName)) == 0) {
            found = 1;
            break;
        }
    }
    if (!found) {
        fprintf(stderr, "TK2000 name not found.");
        exit(1);
    }
	if (!(ct2File = fopen(filename, "wb"))) {
		fprintf(stderr, "Error writing file '%s'\n", filename);
		exit(1);
	}
    fwrite(CT2_MAGIC, 1, 4, ct2File);
    for (i = 0; i < ct2Binaries->numOfBinaries; i++) {
        binary = ct2Binaries->binaries[i];
        if (NULL != tk2kName && memcmp(tk2kName, binary->name, strlen(tk2kName)) == 0) {
            continue;
        }
        if (index == (i+1)) {
            continue;
        }
        bufSize = calcCt2BufferSize(binary->size);
   		buffer = (char *)malloc(bufSize);
		createOneCt2Binary(binary, buffer);
        fwrite(buffer, 1, bufSize, ct2File);
		free(buffer);
    }
}

/*****************************************************************************/
static void insertCommand(char *filename) {
    FILE              *ct2File, *binFile;
    struct Tk2kBinary *binary, *newBinary;
    int               i, bufSize, fileSize;
    char              *buffer;
    unsigned short    initialAddr, length;

    if (index < 0 || index > ct2Binaries->numOfBinaries) {
        fprintf(stderr, "Invalid index.");
        exit(1);
    }
    if (NULL == outputFilename) {
        fprintf(stderr, "Binary file to insert not set.");
        exit(1);
    }
	if (!(binFile = fopen(outputFilename, "rb"))) {
		fprintf(stderr, "Error reading file '%s'\n", outputFilename);
		exit(1);
	}
    fseek(binFile, 0, SEEK_END);
	fileSize = (int)(ftell(binFile));
	fseek(binFile, 0, SEEK_SET);

    // Discover initial address
    if (applesoftFormat == 1) {
        fread(&initialAddr, 1, 2, binFile);
        fread(&length, 1, 2, binFile);
        if (length != (unsigned short)fileSize) {
            fprintf(stderr, "Invalid applesoft format, length do not match.");
            exit(1);
        }
    } else {
        if (strlen(outputFilename) > 6 && outputFilename[strlen(outputFilename)-7] == '#') {
            sscanf(&outputFilename[strlen(outputFilename)-4], "%X", &i);
            initialAddr = (unsigned short)i;
            length = (unsigned short)fileSize;
        } else {
            fprintf(stderr, "Impossible to find initial address.");
            exit(1);
        }
    }
    newBinary = (struct Tk2kBinary *)malloc(sizeof(struct Tk2kBinary));
    newBinary->data = (char *)malloc(fileSize);
    fread(newBinary->data, 1, fileSize, binFile);
    fclose(binFile);
    memset(newBinary->name, 0, 7);
    if (NULL == tk2kName) {
        strncpy(newBinary->name, outputFilename, MIN(6, strlen(outputFilename)));
    } else {
        strncpy(newBinary->name, tk2kName, MIN(6, strlen(tk2kName)));
    }
    newBinary->initialAddr = initialAddr;
    newBinary->endAddr = initialAddr + length - 1;
    newBinary->size = fileSize;
    newBinary->numberOfBlocks = (fileSize - 1) / 256 + 1;
	if (!(ct2File = fopen(filename, "wb"))) {
		fprintf(stderr, "Error writing file '%s'.\n", filename);
		exit(1);
	}
    fwrite(CT2_MAGIC, 1, 4, ct2File);
    for (i = 0; i < ct2Binaries->numOfBinaries; i++) {
        binary = ct2Binaries->binaries[i];
        if (index == (i+1)) {
            // Insert new binary
            bufSize = calcCt2BufferSize(newBinary->size);
            buffer = (char *)malloc(bufSize);
            createOneCt2Binary(newBinary, buffer);
            fwrite(buffer, 1, bufSize, ct2File);
            free(buffer);
        }
        bufSize = calcCt2BufferSize(binary->size);
   		buffer = (char *)malloc(bufSize);
		createOneCt2Binary(binary, buffer);
        fwrite(buffer, 1, bufSize, ct2File);
		free(buffer);
    }
    // If insert on end
    if (index == 0) {
        // Insert new binary
        bufSize = calcCt2BufferSize(newBinary->size);
        buffer = (char *)malloc(bufSize);
        createOneCt2Binary(newBinary, buffer);
        fwrite(buffer, 1, bufSize, ct2File);
        free(buffer);
    }
    free(newBinary->data);
    free(newBinary);
}

/*****************************************************************************/
static void waveCommand(char *filename) {
    int               i, pos;
    FILE              *ct2File;
    char              temp[4], b;
    char              *ofnTemp;
    struct SCh        ch;
    struct STKCab     tkHeader;
	struct STKAddr    tkAddr;

    if (NULL == outputFilename) {
        ofnTemp = withoutExt(filename);
        outputFilename = (char *)malloc(strlen(ofnTemp + 5));
        sprintf(outputFilename, "%s.wav", ofnTemp);
        free(ofnTemp);
    }
	if (!(ct2File = fopen(filename, "rb"))) {
		fprintf(stderr, "Error reading file '%s'.\n", filename);
		exit(1);
	}
	fread(temp, 1, 4, ct2File);
	if (strncmp(temp, CT2_MAGIC, 4)) {
		fclose(ct2File);
		fprintf(stderr, "CT2 file not recognized.\n");
		exit(1);
	}

	wavConfig(waveFormat, sampleRate, 16, 1.0, 0);
	if (createWaveFile(outputFilename) == 0) {
		fprintf(stderr, "Error creating file '%s'.\n", outputFilename);
		exit(1);
	}

    playSilence(DURSILENP);
	while (!feof(ct2File)) {
		fread(&ch, 1, 4, ct2File);
		if (0 == strncmp((const char*)ch.ID, CT2_CAB_A, 4)) {
			playSilence(DURSILENS);
			playTone(TK2000_BIT1, DURCABA, 0.5);
		} else if (0 == strncmp((const char*)ch.ID, CT2_CAB_B, 4)) {
			playTone(TK2000_CABB, 30, 0.5);
			playTone(TK2000_BIT0, 1, 0.5);		// bit 0
		} else if (0 == strncmp((const char*)ch.ID, CT2_DATA, 2)) {
			pos = ftell(ct2File);
			fread(&tkHeader, 1, sizeof(struct STKCab), ct2File);
			if (tkHeader.actualBlock == 0) {
				fread(&tkAddr, 1, sizeof(struct STKAddr), ct2File);
			}
			fseek(ct2File, pos, SEEK_SET);
			for (i = 0; i < ch.size; i++) {
				fread(&b, 1, 1, ct2File);
				tk2kPlayByte(b);
			}
			// Verifica se é o final do arquivo;
			if (tkHeader.actualBlock == tkHeader.numberOfBlocks) {
				playSilence(100);
			}
		}
	}
	finishWaveFile();
    fclose(ct2File);
}

/*****************************************************************************/
static void usage(char *progName) {
	fprintf(stderr, "\n");
	fprintf(stderr, "%s - Utility to manipulate CT2 file (TK2000 cassete). Version %s\n\n", progName, VERSION);
	fprintf(stderr, " Usage:\n");
	fprintf(stderr, "   %s <filename> <command/options>\n\n", progName);
	fprintf(stderr, " Commands:\n");
    fprintf(stderr, "  -c                                 - Create a empty CT2.\n");
	fprintf(stderr, "  -l                                 - List binaries.\n");
	fprintf(stderr, "  -e [-a|-n index|-f file]           - Extract binaries.\n");
    fprintf(stderr, "  -d [-n index|-t name]              - Delete one binary.\n");
    fprintf(stderr, "  -i <-f file> [-a|-n index|-t name] - Insert one binary.\n");
	fprintf(stderr, "  -w <-f file> [-s sr|-q wf]         - Generate WAV file.\n");
	fprintf(stderr, " Options for commands:\n");
	fprintf(stderr, "  -a               - Binary format in TKDOS format.\n");
	fprintf(stderr, "  -n <index>       - Choose binary with <index> index.\n");
    fprintf(stderr, "  -f <file>        - Input/Output filename.\n");
    fprintf(stderr, "  -t <name>        - TK2000 name.\n");
    fprintf(stderr, "  -s <samplerate>  - Wav samplerate (8000-88200).\n");
    fprintf(stderr, "  -q <wave format> - Wave format (Sine or Square).\n");
	fprintf(stderr, " Generic options:\n");
	fprintf(stderr, "  -h or -?         - Show this usage.\n");
    fprintf(stderr, "  -o <directory>   - Save files in this directory.\n");
	fprintf(stderr, "\n");
}

// Public functions

/*****************************************************************************/
int main(int argc, char *argv[]) {
    char          *filename = NULL;
    int           c = 2, i;
    enum Commands command;

    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }
	filename = argv[1];

    // Parse command line
	while (c < argc) {
		if (argv[c][0] == '-' || argv[c][0] == '/') {
            // Test options/commands with no parameter
			switch (argv[c][1]) {
                case 'c':
                    command = CMD_CREATE;
                    break;

                case 'l':
                    command = CMD_LIST;
                    break;

                case 'e':
                    command = CMD_EXTRACT;
                    break;

                case 'd':
                    command = CMD_DELETE;
                    break;

                case 'i':
                    command = CMD_INSERT;
                    break;

                case 'w':
                    command = CMD_WAVE;
                    break;

                case 'a':
                    applesoftFormat = 1;
                    break;

                case 'h':
                case '?':
                    usage(argv[0]);
                    return 0;

                default:
                	// Test options/commands with parameters
					if (c+1 == argc) {
						fprintf(stderr, "Missing parameter for %s", argv[c]);
						return 1;
					}
					switch(argv[c][1]) {
                        case 'n':
                            index = atoi(argv[++c]);
                            break;
                        
                        case 'f':
                            outputFilename = strdup(argv[++c]);
                            break;

                        case 't':
                            tk2kName = argv[++c];
                            break;

                        case 's':
                            sampleRate = MIN(88200, MAX(8000, atoi(argv[++c])));

                        case 'q':
                            ++c;
                            if (strcasecmp(argv[c], "sine") == 0) {
                                waveFormat = WF_SINE;
                            } else if (strcasecmp(argv[c], "square") == 0) {
                                waveFormat = WF_SQUARE;
                            } else {
                                fprintf(stderr, "Invalid wave format: %s\n", argv[c]);
                                usage(argv[0]);
                                return 1;
                            }
                            break;

                        case 'o':
                            outputDirectory = argv[++c];
                            break;

                        default:
                            fprintf(stderr, "Invalid command/option: %s\n", argv[c]);
                            usage(argv[0]);
                            return 1;
                    }
			} // switch
		} else {
			//
        }
		c++;
	}

    if (strlen(outputDirectory) > 0) {
        if (!directoryExists(outputDirectory)) {
            fprintf(stderr, "Output directory '%s' does not exist.", outputDirectory);
            return 1;
        }
        if (outputDirectory[strlen(outputDirectory)-1] != '/' ||
                outputDirectory[strlen(outputDirectory)-1] != '\\') {
            strcat(outputDirectory, "/");
        }
    }
    if (command != CMD_WAVE && command != CMD_CREATE) {
        ct2Binaries = readCt2FromFile(filename);
        if (NULL == ct2Binaries) {
            fprintf(stderr, "Error reading file '%s'.\n", filename);
            return 1;
        }
    }
    switch (command) {
        case CMD_CREATE:
            createCommand(filename);
            break;

        case CMD_LIST:
            listCommand();
            break;

        case CMD_EXTRACT:
            extractCommand();
            break;

        case CMD_DELETE:
            deleteCommand(filename);
            break;

        case CMD_INSERT:
            insertCommand(filename);
            break;

        case CMD_WAVE:
            waveCommand(filename);
            break;

        default:
            usage(argv[0]);
            return 1;
    }
    // Free memory
    if (NULL != outputFilename) {
        free(outputFilename);
    }
    if (command != CMD_WAVE && command != CMD_CREATE) {
        for (i = 0; i < ct2Binaries->numOfBinaries; i++) {
            struct Tk2kBinary *binary = ct2Binaries->binaries[i];
            free(binary->data);
            free(binary);
        }
        free(ct2Binaries);
    }
	return 0;
}
