#include "include/bitcoin-structs.h"
#include "include/mlbtcc-internals.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>


#ifndef MAX_BLOCK_BUFFER_MEMORY
#	define MAX_BLOCK_BUFFER_MEMORY 4 * Mib
#endif

Block GetBlock(int height)
{
	Block block;
	block = ReadBlockFromBlkDatFile(&gBlkFiles.files[gIndexRecords.blockIndexRecord[height].blockFile], gIndexRecords.blockIndexRecord[height].blockOffset);
	block.height = height;
	return block;
}

Blocks GetBlocks(int height, int count)
{
	if (count > (int)(MAX_BLOCK_BUFFER_MEMORY / Mib))
		return (Blocks){0};
	Blocks blocks;
	blocks.count = 0;
	blocks.blocks = malloc(sizeof(Block) * count);
	for (int i = 0; i < count; i++)
	{
		blocks.blocks[i] = GetBlock(height + i);
		blocks.count++;
	}
	return blocks;
}

BlockHeader GetBlockHeader(int height)
{
	//NOTE: We get the blockheader from the leveldb index as it is faster. Might want to add a fallback in case leveldb is not available.
	return gIndexRecords.blockIndexRecord[height].blockHeader;
}

Block	GetFirstBlockOfDate(U32 timestamp)
{
	U32	start = timestamp - (timestamp % SECONDS_PER_DAY);
	U32 end		= start + SECONDS_PER_DAY - 1;
	for (U64 i = 0; i < gIndexRecords.blockIndexRecordCount; i++)
	{
		U32 blockTime = gIndexRecords.blockIndexRecord[i].blockHeader.time;
		if (blockTime >= start && blockTime <= end)
			return ( GetBlock(gIndexRecords.blockIndexRecord[i].height));
	}
	return (Block){0};
}

Block	GetLastBlockOfDate(U32 timestamp)
{
	U32	start = timestamp - (timestamp % SECONDS_PER_DAY);
	U32 end		= start + SECONDS_PER_DAY - 1;

	for (U64 i = gIndexRecords.blockIndexRecordCount; i > 0; i--)
	{
		U32 blockTime = gIndexRecords.blockIndexRecord[i].blockHeader.time;
		if (blockTime >= start && blockTime <= end)
			return ( GetBlock(gIndexRecords.blockIndexRecord[i].height));
	}
	return (Block){0};
}

BlockStats	GetBlockStats(Block *block)
{
	BlockStats stats;
	memset(&stats, 0, sizeof(Block));
	stats.height	= block->height;
	stats.time		= block->header.time;
	memcpy(stats.blockHash, block->hash, sizeof(stats.blockHash));
	for (int i = 0; i < block->txCount; i++)
	{
		Transaction tx = block->transactions[i];
		stats.ins	+= tx.inputCount;
		stats.outs	+= tx.outputCount;
		stats.totalSize += tx.size;
		for (int j = 0; j < tx.outputCount; j++)
		{
			stats.totalOut += tx.outputs[j].amount;
		}
	}

	return (stats);
}
