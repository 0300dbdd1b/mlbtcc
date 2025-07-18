#include "include/leveldb.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// WARN: DOUBLE-CHECK - May not be portable

LDB_Instance LDB_Init(const char *path)
{
	LDB_Instance db;
	db.errors = NULL;
	db.dbPath = path;
	db.options = LDB_OptionsCreate();
	LDB_OptionsSetCreateIfMissing(db.options, 0);
	LDB_OptionsSetErrorIfExists(db.options, 0);

	db.roptions = LDB_ReadOptionsCreate();
	LDB_ReadOptionsSetFillCache(db.roptions, 0);

	db.woptions = LDB_WriteOptionsCreate();
	return db;
}


LDB_Instance LDB_InitOpen(const char *path)
{
	LDB_Instance instance = LDB_Init(path);
	instance.db = LDB_Open(instance.options, instance.dbPath, &instance.errors);
	if (instance.errors != NULL)
	{
		//WARN: Better error handling system needed
		printf("Failed to open LevelDB: %s\n", instance.errors);
		LDB_Free(instance.errors);
		exit(EXIT_FAILURE);
	}
	return instance;
}

size_t LDB_CountEntries(LDB_Instance instance, const char keyPrefix)
{
	LDB_Iterator *iterator = LDB_CreateIterator(instance.db, instance.roptions);
	size_t count = 0;

	const char *key;
	size_t  keyLen;

	for (LDB_IterSeekToFirst(iterator); LDB_IterValid(iterator); LDB_IterNext(iterator))
	{
		key = LDB_IterKey(iterator, &keyLen);
		if (key[0] == keyPrefix)
			count++;
	}
	LDB_IterDestroy(iterator);
	return count;
}

void LDB_CountEntriesForPrefixes(LDB_Instance instance, const char* keyPrefixes, size_t prefixCount, size_t* counts)
{
	LDB_Iterator* iterator = LDB_CreateIterator(instance.db, instance.roptions);
	memset(counts, 0, prefixCount * sizeof(size_t));  // Initialize all counts to 0

	const char* key;
	size_t keyLen;

	for (LDB_IterSeekToFirst(iterator); LDB_IterValid(iterator); LDB_IterNext(iterator))
	{
		key = LDB_IterKey(iterator, &keyLen);
		for (size_t i = 0; i < prefixCount; i++)
		{
			if (key[0] == keyPrefixes[i])
				counts[i]++;
		}
	}

	LDB_IterDestroy(iterator);
}

