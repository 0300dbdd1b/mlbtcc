#include "include/bitcoin-structs.h"
#include "include/leveldb.h"
#include "include/mlbtcc-internals.h"
#include "mlbtcc.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

FileList gBlkFiles;
IndexRecords gIndexRecords;

void PrintBlockIndexRecord(const BlockIndexRecord *record);

BlockIndexRecord GetBlockIndexRecord(const char *key, size_t keyLen, const char *value, size_t valLen)
{
	(void)keyLen;
	BlockIndexRecord blockIndexRecord;
	size_t offset = 0;

	//NOTE: Storing the blockHash in the BlockIndexRecord
	memcpy(blockIndexRecord.blockHash, key + 1, SHA256_HASH_SIZE);
 	ReverseString(blockIndexRecord.blockHash, SHA256_HASH_SIZE);
	
	offset += DecodeVarint128(value, &blockIndexRecord.version);					// Client version
	offset += DecodeVarint128(value + offset, &blockIndexRecord.height);			// Block Height
	offset += DecodeVarint128(value + offset, (uint64_t *)&blockIndexRecord.validationStatus);	// Validation Status
	offset += DecodeVarint128(value + offset, &blockIndexRecord.txCount);			// Number of transaction
	offset += DecodeVarint128(value + offset, &blockIndexRecord.blockFile);			// in which .ldb file block is stored
	offset += DecodeVarint128(value + offset, &blockIndexRecord.blockOffset);		// location of the block in the .ldb file
	
	if (offset < (valLen - BLOCKHEADER_SIZE))
	{
		offset += DecodeVarint128(value + offset, &blockIndexRecord.undoFile);
		offset += DecodeVarint128(value + offset, &blockIndexRecord.undoOffset);
	}

	if (valLen >= BLOCKHEADER_SIZE)
		blockIndexRecord.blockHeader = DecodeBlockHeader((const uint8_t *)value + (valLen - BLOCKHEADER_SIZE));
	return blockIndexRecord;
}

FileInformationRecord GetFileInformationRecord(const char *key, size_t keyLen, const char *value, size_t valLen)
{
	(void)key;
	(void)keyLen;
	(void)valLen;
	FileInformationRecord fileInformationRecord;
	size_t offset = 0;

	offset += DecodeVarint128(value + offset, &fileInformationRecord.blockCount);
	offset += DecodeVarint128(value + offset, &fileInformationRecord.dataFileSize);
	offset += DecodeVarint128(value + offset, &fileInformationRecord.revFileSize);
	offset += DecodeVarint128(value + offset, &fileInformationRecord.lowestHeight);
	offset += DecodeVarint128(value + offset, &fileInformationRecord.highestHeight);
	offset += DecodeVarint128(value + offset, &fileInformationRecord.lowestTimestamp);
	offset += DecodeVarint128(value + offset, &fileInformationRecord.highestTimestamp);
	return fileInformationRecord;
}

uint32_t GetLastBlockFileNumberUsed(const char *value, size_t valLen)
{
	(void)value;
	(void)valLen;
	return 0;
}

uint8_t GetIsDatabaseReindexing(const char *value, size_t valLen)
{
	(void)value;
	(void)valLen;
	return 0;
}

// NOTE: directory must be the datadir/blocks/ directory
void BuildBlockIndexRecords(char *directory)
{
	gIndexRecords.blockIndexRecordCount = 0;
	gIndexRecords.fileInformationRecordCount = 0;

	/* uint32_t				lastBlockFileNumber; */
	/* uint8_t					isReindexing; */

	const char *			key;
	const char *			value;

	size_t keyLen;
	size_t valLen;

	size_t counts[2];
	int		count = 0;
	(void)count;
	LDB_Instance instance = LDB_InitOpen(directory);
	LDB_CountEntriesForPrefixes(instance, "bf", 2, counts);
	gIndexRecords.blockIndexRecord = malloc(sizeof(BlockIndexRecord) * counts[0]);
	gIndexRecords.fileInformationRecord = malloc(sizeof(FileInformationRecord) * counts[1]);
	
	LDB_Iterator *iterator = LDB_CreateIterator(instance.db, instance.roptions);
	for (LDB_IterSeekToFirst(iterator); LDB_IterValid(iterator); LDB_IterNext(iterator))
	{
		key = LDB_IterKey(iterator, &keyLen);
		value = LDB_IterValue(iterator, &valLen);
		if (key[0] == 'b')
		{
			BlockIndexRecord blockIndexRecord = GetBlockIndexRecord(key, keyLen, value, valLen);
			gIndexRecords.blockIndexRecord[blockIndexRecord.height] = blockIndexRecord;
			gIndexRecords.blockIndexRecordCount++;
		}
	  	else if (key[0] == 'f')
		{
			FileInformationRecord fileInformationRecord = GetFileInformationRecord(key, keyLen, value, valLen);
			(void)fileInformationRecord;
			//  BUG: indexRecords.fileInformationRecord[fileInformationRecord.filenumber] = fileInformationRecord;
			gIndexRecords.fileInformationRecordCount++;
		}
	  	else if (key[0] == 'l')
		{
			continue;
		}
		else if (key[0] == 'R')
		{
			continue;
		}
		count++;
	}
	LDB_IterDestroy(iterator);
	LDB_Close(instance.db);
}


int ExtractBlockNumber(const char *filename)
{
    const char *blkPtr = strstr(filename, "blk");
    if (blkPtr)
    {
        blkPtr += 3; // Skip past "blk"
        return atoi(blkPtr); // Convert the numeric part to an integer
    }
    return 0;
}

// Comparison function for qsort that compares file numbers
int CompareFileInfo(const void *a, const void *b)
{
    FileInfo *fileA = (FileInfo *)a;
    FileInfo *fileB = (FileInfo *)b;

    int numA = ExtractBlockNumber(fileA->filepath);
    int numB = ExtractBlockNumber(fileB->filepath);

    return (numA - numB); // Sort by numeric value of blk*.dat
}

// Function to sort the FileList based on the number in blk*.dat filenames
void SortFiles(FileList *fileList)
{
    qsort(fileList->files, fileList->count, sizeof(FileInfo), CompareFileInfo);
}


CoinRecord GetCoinRecord(const char *key, size_t keyLen, const char *value, size_t valLen)
{
    CoinRecord cr = {0};
    size_t offset = 0;
	(void)keyLen;
	(void)valLen;
    // --- decode key: 0x43 ('C') || 32-byte TXID (little endian in DB)
    memcpy(cr.txid, key + 1, SHA256_HASH_SIZE);
    // flip to big-endian
    ReverseString(cr.txid, SHA256_HASH_SIZE);

    // --- decode header code
    uint64_t code = 0;
    offset += DecodeVarint128(value + offset, &code);
    cr.height   = (int32_t)(code >> 1);
    cr.coinbase = (code & 1);
    uint64_t nUnspent = 0;
    offset += DecodeVarint128(value + offset, &nUnspent);
    return cr;
}
void BuildCoinRecords(char *directory)
{
	gIndexRecords.coinRecordCount = 0;

	/* uint32_t				lastBlockFileNumber; */
	/* uint8_t					isReindexing; */

	const char *			key;
	const char *			value;

	size_t keyLen;
	size_t valLen;

	size_t counts[2];
	int		count = 0;
	(void)count;
	LDB_Instance instance = LDB_InitOpen(directory);
	LDB_CountEntriesForPrefixes(instance, "CB", 2, counts);
	gIndexRecords.coinRecord = malloc(sizeof(CoinRecord) * counts[0]);
	
	LDB_Iterator *iterator = LDB_CreateIterator(instance.db, instance.roptions);
	for (LDB_IterSeekToFirst(iterator); LDB_IterValid(iterator); LDB_IterNext(iterator))
	{
		key = LDB_IterKey(iterator, &keyLen);
		value = LDB_IterValue(iterator, &valLen);
		if (key[0] == 'C')
		{
			CoinRecord coinRecord = GetCoinRecord(key, keyLen, value, valLen);
			gIndexRecords.coinRecord[gIndexRecords.coinRecordCount] = coinRecord;
			gIndexRecords.coinRecordCount++;
		}
		count++;
	}
	LDB_IterDestroy(iterator);
	LDB_Close(instance.db);
}


int IsBitcoindRunning(void)
{
	FILE *fp = popen("ps -A | grep '[b]itcoind'", "r");
	if (!fp) return 0;

	char buffer[256];
	int found = 0;

	if (fgets(buffer, sizeof(buffer), fp) != NULL)
		found = 1;

	pclose(fp);
	return found;
}


void IndexCoreDatadir(char *path)
{
	char datadir[MAX_PATH_LENGTH];
	char currentDirectory[MAX_PATH_LENGTH];
	char blkDatDir[MAX_PATH_LENGTH];
	char blkIndexesDir[MAX_PATH_LENGTH];
	char chainstateDir[MAX_PATH_LENGTH];
	char coinstatsDir[MAX_PATH_LENGTH];
	char txIndexDir[MAX_PATH_LENGTH];

	memcpy(datadir, path, strlen(path) + 1);
	printf("%s\n", datadir);
	SanitizeDirString(datadir);

	/* if (!IsDirectory(datadir)) */
	/* 	GetBitcoinDatadir(datadir); */
	// WARN: This won't work on windows.
	// INFO: Might want to use something faster than snprintf
	getcwd(currentDirectory, sizeof(currentDirectory));
    SanitizeDirString(currentDirectory);

	// WARN: This code is unsafe if datadir is of size MAX_PATH_LENGTH
	snprintf(blkDatDir, sizeof(blkDatDir), "%sblocks/", datadir);
    snprintf(blkIndexesDir, sizeof(blkIndexesDir), "%sblocks/index/", datadir);
    snprintf(chainstateDir, sizeof(chainstateDir), "%schainstate/", datadir);
    snprintf(coinstatsDir, sizeof(coinstatsDir), "%sindexes/coinstats/", datadir);
    snprintf(txIndexDir, sizeof(txIndexDir), "%sindexes/txindex/", datadir);


	//NOTE: We can make a better estimate of the number of blk.dat files 4k is dumb.
	gBlkFiles = ListFiles(blkDatDir, "blk*.dat", 4000);		//NOTE: Init Global Variables -- Array of blk.dat FileInfo
	SortFiles(&gBlkFiles);									//NOTE: We sort that array so that array[0] == blk00000.dat
	printf("Bitcoin running %d\n", IsBitcoindRunning());
	if (!IsBitcoindRunning())
	{
		BuildBlockIndexRecords(blkIndexesDir);					//NOTE: Init Global Variables -- IndexRecords.BlockIndexRecord
	}
	else
	{
		char tmpBlkIndexesDir[MAX_PATH_LENGTH] = "/tmp/block-index";
		CopyDirectory(blkIndexesDir, tmpBlkIndexesDir);
		BuildBlockIndexRecords(tmpBlkIndexesDir);
		DeleteDirectory(tmpBlkIndexesDir);
	}
	// BuildCoinRecords(chainstateDir);
}

