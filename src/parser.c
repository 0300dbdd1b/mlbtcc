#include "include/bitcoin-structs.h"
#include "include/mlbtcc-internals.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>


const char *defaultDatadirs[] =
{
	"/home/bitcoin/.bitcoin/",
	"/home/$USER/.bitcoin/",
	"/Users/$USER/Library/Application Support/Bitcoin/",
	"/root/.bitcoin/",
	"/var/lib/bitcoind/",
	"/mnt/data/bitcoin/",
	NULL
};

void ExpandUserDir(char *out, const char *path)
{
	const char *home = getenv("HOME");
	if (!home) home = "/";

	if (strstr(path, "$USER") || strstr(path, "~"))
	{
		const char *user = getenv("USER");
		if (user)
		{
			char tmp[MAX_PATH_LENGTH];
			strcpy(tmp, path);
			char *dollar = strstr(tmp, "$USER");
			if (dollar)
			{
				*dollar = '\0';
				snprintf(out, MAX_PATH_LENGTH, "%s%s%s", tmp, user, dollar + 5);
			}
			else
			{
				strcpy(out, path);
			}
		}
		else
		{
			strcpy(out, path);
		}
	}
	else
	{
		strcpy(out, path);
	}
}

U8 SearchDefaultDatadirs(char *out)
{
	for (int i = 0; defaultDatadirs[i] != NULL; i++)
	{
		char expanded[MAX_PATH_LENGTH];
		ExpandUserDir(expanded, defaultDatadirs[i]);

		if (IsDirectory(expanded))
		{
			strcpy(out, expanded);
			return 1;
		}
	}
	return 0;
}

void ParseCoreDatadir(char *path)
{
	char datadir[MAX_PATH_LENGTH];

	if (IsDirectory(path))
	{
		strncpy(datadir, path, MAX_PATH_LENGTH);
	}
	else if (!SearchDefaultDatadirs(datadir))
	{
		fprintf(stderr, "❌ Could not find a valid Bitcoin datadir.\n");
		exit(1);
	}

	// Set global environment
	strncpy(gEnv.dataDir, datadir, MAX_PATH_LENGTH);
	snprintf(gEnv.blocksDir, sizeof(gEnv.blocksDir), "%sblocks/", datadir);
	snprintf(gEnv.indexesDir, sizeof(gEnv.indexesDir), "%sindexes/", datadir);

	if (!IsDirectory(gEnv.blocksDir))
	{
		fprintf(stderr, "❌ '%s' is missing or invalid.\n", gEnv.blocksDir);
		exit(1);
	}
	else
	{
		SET_BIT(gEnv.status, HAS_DATADIR);
		if (IsDirectory(gEnv.indexesDir))
		{
			char	txindex[MAX_PATH_LENGTH];
			char	coinstat[MAX_PATH_LENGTH];
			snprintf(txindex, sizeof(txindex), "%s/txindex", gEnv.indexesDir);
			snprintf(coinstat, sizeof(coinstat), "%s/coinstat", gEnv.indexesDir);
			SET_BIT(gEnv.status, HAS_INDEXES);
			if (IsDirectory(txindex))
			{
				SET_BIT(gEnv.status, HAS_TXINDEX);
			}
			if (IsDirectory(coinstat))
			{
				SET_BIT(gEnv.status, HAS_COINSTAT);
			}
		}
	}
}
