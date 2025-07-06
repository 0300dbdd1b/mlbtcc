#include "mlbtcc.h"
#include "include/mlbtcc-internals.h"
#include "string.h"
MLBTCC_ENV gEnv;


void InitMLBTCC(const char *path)
{
	memset(&gEnv, 0, sizeof(gEnv));
	ParseCoreDatadir((char*)path);
	IndexCoreDatadir();
}
