#ifndef MLIBBTC_H
#define MLIBBTC_H

#ifndef NO_BITCOIN_STRUCTS
	#include "include/bitcoin-structs.h"
#endif

#include <stdio.h>

void		InitMLBTCC(const char *datadir);
void		IndexCoreDatadir(void);

Block		GetBlock(int height);
Blocks		GetBlocks(int height, int count);
BlockHeader GetBlockHeader(int height);
Block		GetFirstBlockOfDate(U32 timestamp);
Block		GetLastBlockOfDate(U32 timestamp);
BlockStats	GetBlockStats(int height);
Blocks		GetBlocksOfDay(U32 timestamp);

void FreeBlock(Block *block);
void FreeBlocks(Blocks *blocks);

void PrintBlockHeader(const BlockHeader *header, FILE *output);
void PrintInput(const Input *input, FILE *output);
void PrintOutput(const Output *output, FILE *outputFile);
void PrintTransaction(const Transaction *tx, FILE *output);
void PrintBlock(const Block *block, FILE *output);
void PrintBlocks(const Blocks blocks, FILE *output);
void PrintBlockIndexRecord(const BlockIndexRecord *record);

#endif
