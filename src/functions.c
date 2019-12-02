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
#include <ctype.h>
#include <string.h>
#include "ct2.h"
#include "functions.h"

// Defines
#define hexMapL 16

// Structs
struct CHexMap {
	char chr;
	int value;
};

// Constants
const struct CHexMap hexMap[hexMapL] = {
	{ '0', 0 }, { '1', 1 }, { '2', 2 }, { '3', 3 },
	{ '4', 4 }, { '5', 5 }, { '6', 6 }, { '7', 7 },
	{ '8', 8 }, { '9', 9 }, { 'A', 10 }, { 'B', 11 },
	{ 'C', 12 }, { 'D', 13 }, {'E', 14 }, { 'F', 15 }
};

// Public functions

/*****************************************************************************/
char *trim(char *s) {
	char *ptr;
	if (!s)
		return NULL; // handle NULL string
	if (!*s)
		return s; // handle empty string
	for (ptr = s + strlen(s) - 1; (ptr >= s) && (*ptr == ' '); --ptr)
		;
	ptr[1] = '\0';
	return s;
}

/*****************************************************************************/
char *withoutExt(const char *s) {
	char *o = (char *)malloc(strlen(s) + 1);
	strcpy(o, s);
	char *p = (char *)(o + strlen(o));
	while(--p > o) {
		if (*p == '.') {
			*p = '\0';
			break;
		}
	}
	return o;
}

/*****************************************************************************/
char *onlyPath(const char *s) {
	char *p = (char *)(s + strlen(s));
	while(--p > s) {
		if (*p == '/' || *p == '\\') {
			++p;
			break;
		}
	}
	unsigned int l = p - s;
	char *o = (char *)malloc(l+1);
	memset(o, 0, l+1);
	if (l > 0) {
		memcpy(o, s, l);
	}
	return o;
}

/*****************************************************************************/
char *cleanFN(char *s) {
	char *ptr = s;
	while (*ptr) {
		if (*ptr == '\\' || *ptr == '/' || *ptr == ':' || *ptr == '*' || *ptr
				== '?' || *ptr == '"' || *ptr == '<' || *ptr == '>' || *ptr
				== '|' || *ptr == '.') {
			*ptr = '_';
		}
		ptr++;
	}
	return s;
}

/*****************************************************************************/
int _httoi(char *value) {
	int result = 0;
	char *temp = strdup((const char *)value);
	char *s = temp;

	while(*s) {
		*s = toupper(*s);
		s++;
	}
	s = temp;
	if (*s == '0' && *(s + 1) == 'X')
		s += 2;
	while (*s) {
		int found = 0, i;
		for (i = 0; i < hexMapL; i++) {
			if (*s == hexMap[i].chr) {
				result <<= 4;
				result |= hexMap[i].value;
				found = 1;
				break;
			}
		}
		if (!found)
			break;
		s++;
	}
	free(temp);
	return result;
}

/*****************************************************************************/
void showInf(struct STKCab *tkcab, struct STKAddr *tkend) {
	int  i;
	char name[7];

	for (i=0; i<6; i++)
		name[i] = tkcab->name[i] & 0x7F;
	name[6] = 0;
	printf("'%s' of 0x%.2X blocks, ", name, tkcab->numberOfBlocks);
	printf("from 0x%.4X to 0x%.4X\n", tkend->initialAddr, tkend->endAddr);
}
