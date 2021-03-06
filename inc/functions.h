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

#pragma once

#include "ct2.h"


// Prototipes
char *trim(char *str);
char *withoutExt(const char *s);
char *onlyPath(const char *s);
char *cleanFN(char *s);
int _httoi(char *value);
void showInf(struct STKCab *tkcab, struct STKAddr *tkend);
int directoryExists(char *pathname);
