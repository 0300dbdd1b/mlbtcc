#include "../src/mlbtcc.h"

int main(void)
{
	Indexer("/Volumes/LaCie/Bitcoin/BitcoinCore/");
	Block b = GetBlock(1);
	PrintBlock(&b, stdout);
}

