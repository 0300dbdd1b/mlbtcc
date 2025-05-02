#include "../src/mlbtcc.h"

int main(void)
{
	IndexCoreDatadir("/Volumes/LaCie/Bitcoin/BitcoinCore/");
	Block b = GetBlock(0);
	PrintBlock(&b, stdout);
}

