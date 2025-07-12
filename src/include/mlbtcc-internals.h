#ifndef MLBTCC_CONTEXT_H
#define MLBTCC_CONTEXT_H

#include "bitcoin-structs.h"
#define MAX_PATH_LENGTH 512
#define SECONDS_PER_DAY 60 * 60 * 24

#define IS_BIT_SET(value, bit) (((value) >> (bit)) & 1)
#define SET_BIT(value, bit) ((value) |= (1ULL << (bit)))
#define CLEAR_BIT(value, bit) ((value) &= ~(1ULL << (bit)))

typedef enum MLBTCC_RETURN_CODES
{
	MAIN_DATADIR_FOUND,
	MAIN_DATADIR_NOT_FOUND,

} MLBTCC_RETURN_CODES;

typedef enum MLBTCC_STATUS
{
	HAS_DATADIR				= 0,
	HAS_BLOCK_INDEX_RECORD	= 1 << 0,
	HAS_CHAINSTATE			= 1 << 1,
	HAS_INDEXES				= 1 << 2,
	HAS_TXINDEX				= 1 << 3,
	HAS_COINSTAT			= 1 << 4,
} MLBTCC_STATUS;

typedef struct MLBTCC_ENV
{
	char			dataDir[MAX_PATH_LENGTH];
	char			blocksDir[MAX_PATH_LENGTH];
	char			indexesDir[MAX_PATH_LENGTH];
	char			chainstateDir[MAX_PATH_LENGTH];
	MLBTCC_STATUS	status;
	FileList		blkFiles;
	IndexRecords	indexRecords;
	U8				xorKey[8];
} MLBTCC_ENV;
extern MLBTCC_ENV gEnv;

#ifndef MLBTCC_MAX_MEMORY
	#define MLBTCC_MAX_MEMORY	4 * Gib
#endif




Block ReadBlockFromBlkDatFile(FileInfo *fileInfo, size_t offset);

BlockIndexRecord		GetBlockIndexRecord(const char *key, size_t keyLen, const char *value, size_t valLen);
FileInformationRecord	GetFileInformationRecord(const char *key, size_t keyLen, const char *value, size_t valLen);
uint32_t				GetLastBlockFileNumberUsed(const char *value, size_t valLen);
uint8_t					GetIsDatabaseReindexing(const char *value, size_t valLen);
void					BuildBlockIndexRecords(char *directory);
void					BuildCoinRecords(char *directory);
int						ExtractBlockNumber(const char *filename);
int						CompareFileInfo(const void *a, const void *b);
void					SortFiles(FileList *fileList);



void ReverseString(U8 *str, size_t size);
void SHA256(const U8 *data, size_t datasize, U8 *hash);
void DoubleSHA256(const U8 *data, size_t datasize, U8 *hash);
void ConvertTimestampToString(unsigned int timestamp, char *buffer, size_t bufferSize);
BlockHeader DecodeBlockHeader(const U8 *rawBlockHeader);

void PrintBlockHeader(const BlockHeader *header, FILE *output);
void PrintInput(const Input *input, FILE *output);
void PrintOutput(const Output *output, FILE *outputFile);
void PrintTransaction(const Transaction *tx, FILE *output);
void PrintBlock(const Block *block, FILE *output);
void PrintBlocks(const Blocks blocks, FILE *output);
void PrintBlockIndexRecord(const BlockIndexRecord *record);

void InitBlockHeader(BlockHeader *header);
void InitInput(Input *input);
void InitOutput(Output *output);
void InitStackItem(StackItem *item);
void InitWitness(Witness *witness);
void InitTransaction(Transaction *tx);
void InitBlock(Block *block);


size_t VarintDecode(const U8 *input, U32 *output);
void PrintByteString(const U8 *bytes, size_t size, FILE *output);
void ConvertUint32ToUint8Array(U32 value, U8 *outputArray);
size_t EncodeVarint128(U64 value, U8* data);
size_t DecodeVarint128(const void *data, U64 *value);
U32 ChangeEndiannessUint32(U32 value);
size_t CompactSizeEncode(U64 value, U8* output);
size_t CompactSizeDecode(const U8 *data, size_t dataSize, U64 *value);


long GetFileSize(const char *filename);
int WildcardMatch(const char *str, const char *pattern);
int CopyDirectory(const char *src_dir, const char *dest_dir);
int DeleteDirectory(const char *dir_path);
int IsDirectory(const char *path);
void SanitizeDirString(char *dir);
FileList ListFiles(const char *directory, const char *pattern, uint16_t allocationSize);
int IsBitcoinDatadir(const char *path);
void TryDefaultBitcoinDatadirs(char *datadir);
void GetBitcoinDatadir(char *datadir);


void FreeInput(Input *input);
void FreeOutput(Output *output);
void FreeWitness(Witness *witness);
void FreeTransaction(Transaction *transaction);
void FreeBlock(Block *block);
void FreeFileList(FileList *fileList);


void ParseCoreDatadir(char *path);

Transaction *ReadTxn(const U8 *blockBuffer, U16 txCount);
U8 HexToBytes(U8 *out, const char *hex);
void BytesToHex(const U8 *bytes, size_t length, char *output);
size_t FReadXor(void *dest, size_t size, size_t count, FILE *file);
#endif
