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
#include <wav.h>
#include <ct2.h>

// Defines

// Structs

// Variables
static struct Ct2File *ct2File;

// Private functions

/*****************************************************************************/
static void listCommand() {
    int i;
    struct Tk2kBinary *binary;

    for (i = 0; i < ct2File->numOfBinaries; i++) {
        binary = ct2File->binaries[i];
        printf("%2d '%s' of 0x%.2X blocks, ", i+1, 
            binary->name, binary->numberOfBlocks);
	    printf("from 0x%.4X to 0x%.4X\n", 
            binary->initialAddr, binary->endAddr);
    }
}

/*****************************************************************************/
static void usage(char *progName) {
	fprintf(stderr, "\n");
	fprintf(stderr, "%s - Utility to manipulate CT2 file (TK2000 cassete). Version %s\n\n", progName, VERSION);
	fprintf(stderr, " Usage:\n");
	fprintf(stderr, "   %s <filename> <command/options>\n\n", progName);
	fprintf(stderr, " Commands:\n");
	fprintf(stderr, "  -l        - List binaries.\n");
	fprintf(stderr, "  -e [-a]   - Extract binaries.\n");
	fprintf(stderr, " Options for extract command:\n");
	fprintf(stderr, "  -a         - Binary format in TKDOS format.\n");
	fprintf(stderr, " Generic options:\n");
	fprintf(stderr, "  -h or -?   - Show this usage.\n");
	fprintf(stderr, "\n");
}

// Public functions

/*****************************************************************************/
int main(int argc, char *argv[]) {
    char *filename = NULL, command;
    int c = 2, i, applesoftFormat = 0;

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
                case 'l':
                    command = 'l';
                    break;

                case 'e':
                    command = 'e';
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
                        default:
                            fprintf(stderr, "Invalid command/option: %s\n", argv[c]);
                            usage(argv[0]);
                            return 0;
                    }
			} // switch
		} else {
			//
        }
		c++;
	}


    ct2File = readCt2FromFile(filename);
    if (NULL == ct2File) {
        fprintf(stderr, "Error reading file '%s'.\n", filename);
        return 1;
    }
    if (command == 'l') {
        listCommand();
    } else if (command == 'e') {
        
    } else {
        usage(argv[0]);
		return 1;
    }
    // Free memory
    for (i = 0; i < ct2File->numOfBinaries; i++) {
        struct Tk2kBinary *binary = ct2File->binaries[i];
        free(binary->data);
        free(binary);
    }
    free(ct2File);
	return 0;
}
