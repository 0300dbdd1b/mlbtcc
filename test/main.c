
#include "../src/mlbtcc.h"

Transaction GetTransactionFromTxId(const char *hexTxid);

#include <stdio.h>
#include <stdint.h>
#include <time.h>

uint64_t GetTimeMs(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)(ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL);
}

static inline uint64_t ReadTSC(void)
{
	unsigned int lo, hi;
	__asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
}

int main(void)
{
	uint64_t t0 = GetTimeMs();
	uint64_t c0 = ReadTSC();

	InitMLBTCC("/Volumes/LaCie/Bitcoin/BitcoinCore/");
	// InitMLBTCC("/Users/leslie/bitcoin-tmp-datadir/");

	uint64_t t1 = GetTimeMs();
	uint64_t c1 = ReadTSC();

	printf("Indexing Done in %llu ms (%llu cycles)\n", t1 - t0, c1 - c0);

	Block b = GetBlock(0);
	PrintBlock(&b, stdout);

	uint64_t t2 = GetTimeMs();
	uint64_t c2 = ReadTSC();

	// Blocks blocks = GetBlocksOfDay(1262304000);
	Blocks blocks = GetBlocksOfDay(1672531200);

	uint64_t t3 = GetTimeMs();
	uint64_t c3 = ReadTSC();

	printf("GetBlocksOfDay done in %llu ms (%llu cycles)\n", t3 - t2, c3 - c2);
	printf("BlocksOfDay Count %lu\n", blocks.count);
	// PrintBlocks(blocks, stdout);

	return 0;
}

