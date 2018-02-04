#ifndef SAVE_H_INCLUDED
#define SAVE_H_INCLUDED

#include "mmu.h"

void save_game(char* file_name, memory *mem);

void load_game(char* file_name, memory *mem);

#endif // SAVE_H_INCLUDED
