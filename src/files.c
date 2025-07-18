#include "mlbtcc.h"
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>

#define PATH_SEPARATOR '/'

long GetFileSize(const char *filename)
{
	struct stat st;

	if (stat(filename, &st) == 0)
		return st.st_size;
	else
	{
		perror("stat");
		return -1;
	}
}


int WildcardMatch(const char *str, const char *pattern)
{
	const char *p = pattern;
	const char *s = str;
	const char *star = NULL;
	const char *ss = NULL;

	// WARN: Trust the process... (Don't)
	while (*s) {
		if (*p == '?' || *p == *s) {
			p++;
			s++;
		} else if (*p == '*') {
			star = p++;
			ss = s;
		} else if (star) {
			p = star + 1;
			s = ++ss;
		} else {
			return 0;
		}
	}

	while (*p == '*') {
		p++;
	}

	return *p == '\0';
}

// Function to copy a directory
int CopyDirectory(const char *src_dir, const char *dest_dir)
{
	char command[1024];
	snprintf(command, sizeof(command), "cp -r %s %s", src_dir, dest_dir);
	int result = system(command);

	if (result != 0)
	{
		fprintf(stderr, "Error copying directory from '%s' to '%s'\n", src_dir, dest_dir);
		return -1;
	}

	return 0;
}

// Function to delete a directory
int DeleteDirectory(const char *dir_path)
{
	char command[1024];
	snprintf(command, sizeof(command), "rm -rf %s", dir_path);
	int result = system(command);

	if (result != 0)
	{
		fprintf(stderr, "Error deleting directory '%s'\n", dir_path);
		return -1;
	}

	return 0;
}

int IsDirectory(const char *path)
{
	struct stat statbuf;
	if (stat(path, &statbuf) != 0)
		return 0;
	return S_ISDIR(statbuf.st_mode);
}


void SanitizeDirString(char *dir)
{
	size_t len = strlen(dir);
	if (len > 0 && dir[len - 1] != (char)PATH_SEPARATOR)
	{
		dir[len] = PATH_SEPARATOR;
		dir[len + 1] = '\0';
	}
}



FileList ListFiles(const char *directory, const char *pattern, uint16_t allocationSize)
{
	FileList fileList;
	fileList.files = (FileInfo *)malloc(sizeof(FileInfo) * allocationSize);
	fileList.count = 0;
	fileList.totalSize = allocationSize;

	struct dirent *entry;
	DIR *dp = opendir(directory);

	if (dp == NULL)
	{
		perror("opendir");
		return (FileList){NULL, 0, 0};
	}

	while ((entry = readdir(dp)) != NULL)
	{
		if (WildcardMatch(entry->d_name, pattern))
		{
			if (fileList.count >= fileList.totalSize)
			{
				fileList.files = (FileInfo *)realloc(fileList.files, sizeof(FileInfo) * (fileList.totalSize * 2));
				fileList.totalSize *= 2; // Efficient memory resizing
			}

			// Allocate memory for the filepath and build the full path
			fileList.files[fileList.count].filepath = (char *)malloc(sizeof(char) * (strlen(directory) + strlen(entry->d_name) + 2)); // +1 for '/' and +1 for '\0'
			strcpy(fileList.files[fileList.count].filepath, directory);
			strcat(fileList.files[fileList.count].filepath, "/");
			strcat(fileList.files[fileList.count].filepath, entry->d_name);

			// Mark the file as not opened
			fileList.files[fileList.count].isOpen = 0;
			fileList.files[fileList.count].file = NULL; // File pointer remains NULL until the file is opened later

			fileList.count++;
		}
	}

	closedir(dp);
	return fileList;
}


int IsBitcoinDatadir(const char *path)
{
	if (IsDirectory(path))
	{
		return 1;
	}
	return (0);
}

void TryDefaultBitcoinDatadirs(char *datadir) {
	const char *home = getenv("HOME");
	const char *defaultLocations[] = {
		"/usr/local/bitcoin",
		"/opt/bitcoin",
		"/var/lib/bitcoin",
		"/mnt/data/bitcoin",
		"/home/bitcoin/.bitcoin",
		home ? strcat(strdup(home), "/.bitcoin") : NULL,
		NULL // End of array marker
	};

	for (int i = 0; defaultLocations[i] != NULL; i++)
	{
		if (IsBitcoinDatadir(defaultLocations[i])) {
			strncpy(datadir, defaultLocations[i], 512);
			printf("Using Bitcoin datadir: %s\n", datadir);
			return;
		}
	}

	fprintf(stderr, "No valid Bitcoin datadir found!\n");
	exit(1);
}

// Function to get the Bitcoin datadir
void GetBitcoinDatadir(char *datadir)
{
	if (datadir == NULL || strlen(datadir) == 0 || !IsBitcoinDatadir(datadir)) {
		printf("Provided datadir is invalid. Trying default locations...\n");
		TryDefaultBitcoinDatadirs(datadir);
	}
	else
{
		printf("Provided datadir is valid: %s\n", datadir);
	}
}
