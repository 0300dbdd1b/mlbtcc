#include "include/mlbtcc-internals.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>


Block GetBlock(int height)
{
	Block block;
	block = ReadBlockFromBlkDatFile(&gBlkFiles.files[gIndexRecords.blockIndexRecord[height].blockFile], gIndexRecords.blockIndexRecord[height].blockOffset);
	block.height = height;
	return block;
}


// WARN: UNSAFE FUNCTION -- can bloat memory
// FIXME: Add some sort of assertion in case the count is too high
Blocks GetBlocks(int height, int count)
{
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

BlockStats	GetBlockStats(int height)
{
	BlockStats stats;
	Block block		= GetBlock(height);
	memset(&stats, 0, sizeof(Block));
	stats.height	= height;
	for (int i = 0; i < block.txCount; i++)
	{
		stats.ins += block.transactions[i].inputCount;
		stats.outs += block.transactions[i].outputCount;
	}

	return (stats);
}
