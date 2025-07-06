#include "../src/mlbtcc.h"

Transaction GetTransactionFromTxId(const char *hexTxid);
int main(void)
{
	InitMLBTCC("/Volumes/LaCie/Bitcoin/BitcoinCore/");
	printf("Indexing Done\n");
	// Block b = GetBlock(0);
	// PrintBlock(&b, stdout);
	// Transaction t = GetTransactionFromTxId("1024cb12a576b69defa67dbc2f1899700ab58e5ad3d5e058edefb907f59865bc");
	// PrintTransaction(&t, stdout);
	Blocks blocks = GetBlocksOfDay(1577836800);
	printf("Done\n");
	// PrintBlocks(blocks, stdout);
}

