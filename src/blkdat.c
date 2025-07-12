#include "include/bitcoin-structs.h"
#include "include/mlbtcc-internals.h"
#include "mlbtcc.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "extern/sha256/sha256.h"

extern IndexRecords gIndexRecords;
extern FileList gBlkFiles;
static OpenFileCache openFiles[MAX_OPEN_FILES];

void InitBlockHeader(BlockHeader *header);
void InitInput(Input *input);
void InitOutput(Output *output);
void InitStackItem(StackItem *item);
void InitWitness(Witness *witness);
void InitTransaction(Transaction *tx);
void InitBlock(Block *block);

Transaction *ReadTxn(const U8 *blockBuffer, U16 txCount)
{
	// NOTE: To get the TXID we need to keep track of the raw transaction data without the witness fields.
	// WARN:Even though it works for 90% of transactions, MUST double check for edge cases.
	Transaction *transactions = (Transaction *)malloc(sizeof(Transaction) * txCount);
	SHA256_CTX ctx;
	(void)ctx;
	size_t bufferOffset = 0;
	U8 isSegwit = 0;
	size_t startPosOffset;
	for (U16 i = 0; i < txCount; i++)
	{
		Transaction transaction;

		startPosOffset = bufferOffset;
		memcpy(&transaction, blockBuffer + bufferOffset, sizeof(transaction.version) + sizeof(transaction.marker) + sizeof(transaction.flag)); // 6 bytes
		bufferOffset += sizeof(transaction.version);
		if (transaction.marker == 0x00 && transaction.flag >= 0x01)
		{
			bufferOffset += sizeof(transaction.marker) + sizeof(transaction.flag);
			isSegwit = 1;
		}
		else
		{
			transaction.marker = 0;
			transaction.flag = 0;
			isSegwit = 0;
		}
		bufferOffset += CompactSizeDecode(blockBuffer + bufferOffset, MAX_COMPACT_SIZE_BYTES, (U64 *)&transaction.inputCount);
		transaction.inputs = (Input *)malloc(sizeof(Input) * transaction.inputCount);

		for (U16 inputIndex = 0; inputIndex < transaction.inputCount; inputIndex++)
		{
			Input input;

			memcpy(&input, blockBuffer + bufferOffset, SHA256_HASH_SIZE + sizeof(input.vout));								//NOTE: Get the input TXID and VOUT
			bufferOffset += SHA256_HASH_SIZE + sizeof(input.vout);
			bufferOffset += CompactSizeDecode(blockBuffer + bufferOffset, MAX_COMPACT_SIZE_BYTES, (U64 *)&input.scriptSigSize);	//NOTE: Get the ScriptSig Size

			input.scriptSig = (U8 *)malloc(sizeof(U8) * input.scriptSigSize);

			memcpy(input.scriptSig, blockBuffer + bufferOffset, input.scriptSigSize);										//NOTE: Get the ScriptSig
			bufferOffset += input.scriptSigSize;
			memcpy(&input.sequence, blockBuffer + bufferOffset, sizeof(input.sequence) ); 									//NOTE: Get Sequence
			bufferOffset += sizeof(input.sequence);
			transaction.inputs[inputIndex] = input;
		}

		bufferOffset += CompactSizeDecode(blockBuffer + bufferOffset, MAX_COMPACT_SIZE_BYTES, (U64 *)&transaction.outputCount);
		transaction.outputs = (Output *)malloc(sizeof(Output) * transaction.outputCount);
		for (U16 outputIndex = 0; outputIndex < transaction.outputCount; outputIndex++)
		{
			Output output;

			memcpy(&output.amount, blockBuffer + bufferOffset, sizeof(output.amount));
			bufferOffset += sizeof(output.amount);
			bufferOffset += CompactSizeDecode(blockBuffer + bufferOffset, MAX_COMPACT_SIZE_BYTES, (U64 *)&output.scriptPubKeySize);
			output.scriptPubKey = (U8 *)malloc(sizeof(U8) * output.scriptPubKeySize);
			memcpy(output.scriptPubKey, blockBuffer + bufferOffset, output.scriptPubKeySize);
			bufferOffset += output.scriptPubKeySize;
			transaction.outputs[outputIndex] = output;
		}

		if (isSegwit)
		{
			transaction.witnesses = malloc(sizeof(Witness) * transaction.inputCount);
			for (U16 witnessIndex = 0; witnessIndex < transaction.inputCount; witnessIndex++)
			{
				Witness witness;
				bufferOffset += CompactSizeDecode(blockBuffer + bufferOffset, MAX_COMPACT_SIZE_BYTES, (U64 *)&witness.stackItemsCount);
				witness.stackItems = (StackItem *)malloc(sizeof(StackItem) * witness.stackItemsCount);
				for (U16 stackItemIndex = 0; stackItemIndex < witness.stackItemsCount; stackItemIndex++)
				{
					StackItem stackItem;

					bufferOffset += CompactSizeDecode(blockBuffer + bufferOffset, MAX_COMPACT_SIZE_BYTES, (U64 *)&stackItem.size);
					stackItem.item = (U8 *)malloc(sizeof(U8) * stackItem.size);
					memcpy(stackItem.item, blockBuffer + bufferOffset, stackItem.size);
					bufferOffset += stackItem.size;
					witness.stackItems[stackItemIndex] = stackItem;
				}
				transaction.witnesses[witnessIndex] = witness;
			}
		}
		memcpy(&transaction.locktime, blockBuffer + bufferOffset, sizeof(transaction.locktime));		//NOTE: Get Locktime
		bufferOffset += sizeof(transaction.locktime);
		transaction.size = (U16)bufferOffset - startPosOffset;
		U8 *wtxBuffer = (U8 *)malloc(sizeof(U8) * transaction.size);
		memcpy(wtxBuffer, blockBuffer + startPosOffset, transaction.size);
		DoubleSHA256(wtxBuffer, transaction.size, transaction.wtxid);

		memcpy(transaction.txid, transaction.wtxid, sizeof(transaction.wtxid));
		// PrintByteString(transaction.wtxid, 32);
		// printf("\n");
		free(wtxBuffer);
		transactions[i] = transaction;
	}
	return transactions;
}

void CloseLRUFile(void)
{
	size_t lruIndex = 0;
	uint32_t oldestTime = openFiles[0].lastAccessTime;

	// Iterate over open files to find the least recently used (LRU)
	for (size_t i = 1; i < MAX_OPEN_FILES; i++) 
	{
		if (openFiles[i].fileInfo != NULL && openFiles[i].lastAccessTime < oldestTime)
		{
			lruIndex = i;
			oldestTime = openFiles[i].lastAccessTime;
		}
	}

	// Close the least recently used file
	fclose(openFiles[lruIndex].fileInfo->file);
	openFiles[lruIndex].fileInfo->isOpen = 0;
	openFiles[lruIndex].fileInfo = NULL;  // Remove the entry from the cache
}


// WARN: Imagine keeping a cache of open files, just to prevent some open/close calls, but still calling malloc 1000 times - like a fucking retard.
FILE* OpenFile(FileInfo *fileInfo)
{
	// Check if the file is already open in FileInfo itself
	if (fileInfo->isOpen)
	{
		// The file is already open, update access time in cache
		for (size_t i = 0; i < MAX_OPEN_FILES; i++)
		{
			if (openFiles[i].fileInfo == fileInfo)
			{
				openFiles[i].lastAccessTime = (U32)time(NULL); // WARN: Not portable
				return fileInfo->file;  // Return the open file
			}
		}
	}

	// Check if we need to close an LRU file
	int openSlots = 0;
	for (size_t i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (openFiles[i].fileInfo == NULL)
			openSlots++;  // Count available slots
	}

	// If no slots are available, close the least recently used file
	if (openSlots == 0)
	{
		CloseLRUFile();
	}

	// Actually open the file
	fileInfo->file = fopen(fileInfo->filepath, "r");
	if (fileInfo->file == NULL)
	{
		perror("Failed to open file");
		return NULL;
	}
	fileInfo->isOpen = 1;  // Mark the file as open

	// Add the file to the cache in the first available slot
	for (size_t i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (openFiles[i].fileInfo == NULL)
		{
			openFiles[i].fileInfo = fileInfo;
			openFiles[i].lastAccessTime = (uint32_t)time(NULL); //WARN: Not portable
			break;
		}
	}

	return fileInfo->file;
}

Block ReadBlockFromBlkDatFile(FileInfo *fileInfo, size_t offset)
{
	Block block;
	U8 *blockBuffer;
	U8 blockHeaderBuffer[BLOCKHEADER_SIZE];
	U8 rawPrefix[8];		// NOTE: magic (4 bytes) + blockSize (4 bytes)

	U16 bufferOffset = BLOCKHEADER_SIZE;

	fileInfo->file = OpenFile(fileInfo);
	if (fseek(fileInfo->file, offset - 8, SEEK_SET) != 0)
	{
		printf("Failed to seek to the offset");
	}

	// Read first 8 bytes (magic + blockSize)
	if (fread(rawPrefix, 1, 8, fileInfo->file) != 8)
	{
		printf("Failed to read block prefix");
	}

	int isXorActive = 0;
	for (int i = 0; i < 8; i++)
	{
		if (gEnv.xorKey[i])
		{
			isXorActive = 1;
		}
	}
	if (isXorActive)	// NOTE: No need to xor against 0x0000000000000000
	{
		for (int i = 0; i < 8; i++)
		{
			rawPrefix[i] ^= gEnv.xorKey[(offset - 8 + i) % 8];
		}
	}
	memcpy(&block.magic, rawPrefix, 4);
	memcpy(&block.blockSize, rawPrefix + 4, 4);

	if (offset - 8 + block.blockSize > MAX_BLOCKFILE_SIZE)
	{
		printf("Trying to read bytes after the end of the file");
	}

	blockBuffer = malloc(sizeof(U8) * block.blockSize);
	if (!blockBuffer)
	{
		fprintf(stderr, "Failed to allocate block buffer\n");
		exit(1);
	}
	if (fread(blockBuffer, 1, block.blockSize, fileInfo->file) != block.blockSize) // WARN: Dont remember why I did put that in there.
	{
		printf("Cannot Read.");
	}
	if (ferror(fileInfo->file))
	{
		printf("Error while reading the file");
	}

	if (isXorActive)	// NOTE: No need to xor against 0x0000000000000000
	{
		for (size_t i = 0; i < block.blockSize; i++)
		{
			blockBuffer[i] ^= gEnv.xorKey[(offset + i) % 8];
		}
	}

	memcpy(blockHeaderBuffer, blockBuffer, BLOCKHEADER_SIZE);
	block.header = DecodeBlockHeader(blockHeaderBuffer);

	bufferOffset += CompactSizeDecode(blockBuffer + bufferOffset, MAX_COMPACT_SIZE_BYTES, (U64 *)&block.txCount);
	block.transactions = ReadTxn(blockBuffer + bufferOffset, block.txCount);

	DoubleSHA256(blockHeaderBuffer, BLOCKHEADER_SIZE, block.hash);
	free(blockBuffer);		// WARN: DUUUUUUH??? Let's use some kind of local allocator.
	return block;
}

