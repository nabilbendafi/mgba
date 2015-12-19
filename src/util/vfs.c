/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "vfs.h"

#include "util/string.h"

#ifdef PSP
#include "platform/psp/sce-vfs.h"
#endif
#ifdef PSP2
#include "platform/psp2/sce-vfs.h"
#endif
#ifdef _3DS
#include "platform/3ds/3ds-vfs.h"
#endif

struct VFile* VFileOpen(const char* path, int flags) {
#ifdef USE_VFS_FILE
	const char* chflags;
	switch (flags & O_ACCMODE) {
	case O_WRONLY:
		if (flags & O_APPEND) {
			chflags = "ab";
		} else {
			chflags = "wb";
		}
		break;
	case O_RDWR:
		if (flags & O_APPEND) {
			chflags = "a+b";
		} else if (flags & O_TRUNC) {
			chflags = "w+b";
		} else {
			chflags = "r+b";
		}
		break;
	case O_RDONLY:
		chflags = "rb";
		break;
	}
	return VFileFOpen(path, chflags);
#elif defined(PSP)
	return VFileOpenSce(path, flags, 0666);
#elif defined(PSP2)
	int sceFlags = PSP2_O_RDONLY;
	switch (flags & O_ACCMODE) {
	case O_WRONLY:
		sceFlags = PSP2_O_WRONLY;
		break;
	case O_RDWR:
		sceFlags = PSP2_O_RDWR;
		break;
	case O_RDONLY:
		sceFlags = PSP2_O_RDONLY;
		break;
	}

	if (flags & O_APPEND) {
		sceFlags |= PSP2_O_APPEND;
	}
	if (flags & O_TRUNC) {
		sceFlags |= PSP2_O_TRUNC;
	}
	if (flags & O_CREAT) {
		sceFlags |= PSP2_O_CREAT;
	}
	return VFileOpenSce(path, sceFlags, 0666);
#elif defined(USE_VFS_3DS)
	int ctrFlags = FS_OPEN_READ;
	switch (flags & O_ACCMODE) {
	case O_WRONLY:
		ctrFlags = FS_OPEN_WRITE;
		break;
	case O_RDWR:
		ctrFlags = FS_OPEN_READ | FS_OPEN_WRITE;
		break;
	case O_RDONLY:
		ctrFlags = FS_OPEN_READ;
		break;
	}

	if (flags & O_CREAT) {
		ctrFlags |= FS_OPEN_CREATE;
	}
	struct VFile* vf = VFileOpen3DS(&sdmcArchive, path, ctrFlags);
	if (!vf) {
		return 0;
	}
	if (flags & O_TRUNC) {
		vf->truncate(vf, 0);
	}
	if (flags & O_APPEND) {
		vf->seek(vf, vf->size(vf), SEEK_SET);
	}
	return vf;
#else
	return VFileOpenFD(path, flags);
#endif
}

struct VDir* VDirOpenArchive(const char* path) {
	struct VDir* dir = 0;
#if USE_LIBZIP
	if (!dir) {
		dir = VDirOpenZip(path, 0);
	}
#endif
#if USE_LZMA
	if (!dir) {
		dir = VDirOpen7z(path, 0);
	}
#endif
	return dir;
}

ssize_t VFileReadline(struct VFile* vf, char* buffer, size_t size) {
	size_t bytesRead = 0;
	while (bytesRead < size - 1) {
		ssize_t newRead = vf->read(vf, &buffer[bytesRead], 1);
		if (newRead <= 0) {
			break;
		}
		bytesRead += newRead;
		if (buffer[bytesRead - newRead] == '\n') {
			break;
		}
	}
	buffer[bytesRead] = '\0';
	return bytesRead;
}

ssize_t VFileWrite32LE(struct VFile* vf, int32_t word) {
	uint32_t leword;
	STORE_32LE(word, 0, &leword);
	return vf->write(vf, &leword, 4);
}

ssize_t VFileWrite16LE(struct VFile* vf, int16_t hword) {
	uint16_t lehword;
	STORE_16LE(hword, 0, &lehword);
	return vf->write(vf, &lehword, 2);
}

ssize_t VFileRead32LE(struct VFile* vf, void* word) {
	uint32_t leword;
	ssize_t r = vf->read(vf, &leword, 4);
	if (r == 4) {
		STORE_32LE(leword, 0, word);
	}
	return r;
}

ssize_t VFileRead16LE(struct VFile* vf, void* hword) {
	uint16_t lehword;
	ssize_t r = vf->read(vf, &lehword, 2);
	if (r == 2) {
		STORE_16LE(lehword, 0, hword);
	}
	return r;
}

struct VFile* VDirOptionalOpenFile(struct VDir* dir, const char* realPath, const char* prefix, const char* suffix, int mode) {
	char path[PATH_MAX];
	path[PATH_MAX - 1] = '\0';
	struct VFile* vf;
	if (!dir) {
		if (!realPath) {
			return 0;
		}
		char* dotPoint = strrchr(realPath, '.');
		if (dotPoint - realPath + 1 >= PATH_MAX - 1) {
			return 0;
		}
		if (dotPoint > strrchr(realPath, '/')) {
			int len = dotPoint - realPath;
			strncpy(path, realPath, len);
			path[len] = 0;
			strncat(path + len, suffix, PATH_MAX - len - 1);
		} else {
			snprintf(path, PATH_MAX - 1, "%s%s", realPath, suffix);
		}
		vf = VFileOpen(path, mode);
	} else {
		snprintf(path, PATH_MAX - 1, "%s%s", prefix, suffix);
		vf = dir->openFile(dir, path, mode);
	}
	return vf;
}

struct VFile* VDirOptionalOpenIncrementFile(struct VDir* dir, const char* realPath, const char* prefix, const char* infix, const char* suffix, int mode) {
	char path[PATH_MAX];
	path[PATH_MAX - 1] = '\0';
	char realPrefix[PATH_MAX];
	realPrefix[PATH_MAX - 1] = '\0';
	if (!dir) {
		if (!realPath) {
			return 0;
		}
		const char* separatorPoint = strrchr(realPath, '/');
		const char* dotPoint;
		size_t len;
		if (!separatorPoint) {
			strcpy(path, "./");
			separatorPoint = realPath;
			dotPoint = strrchr(realPath, '.');
		} else {
			path[0] = '\0';
			dotPoint = strrchr(separatorPoint, '.');

			if (separatorPoint - realPath + 1 >= PATH_MAX - 1) {
				return 0;
			}

			len = separatorPoint - realPath;
			strncat(path, realPath, len);
			path[len] = '\0';
			++separatorPoint;
		}

		if (dotPoint - realPath + 1 >= PATH_MAX - 1) {
			return 0;
		}

		if (dotPoint >= separatorPoint) {
			len = dotPoint - separatorPoint;
		} else {
			len = PATH_MAX - 1;
		}

		strncpy(realPrefix, separatorPoint, len);
		realPrefix[len] = '\0';

		prefix = realPrefix;
		dir = VDirOpen(path);
	}
	if (!dir) {
		// This shouldn't be possible
		return 0;
	}
	dir->rewind(dir);
	struct VDirEntry* dirent;
	size_t prefixLen = strlen(prefix);
	size_t infixLen = strlen(infix);
	unsigned next = 0;
	while ((dirent = dir->listNext(dir))) {
		const char* filename = dirent->name(dirent);
		char* dotPoint = strrchr(filename, '.');
		size_t len = strlen(filename);
		if (dotPoint) {
			len = (dotPoint - filename);
		}
		const char* separator = strnrstr(filename, infix, len);
		if (!separator) {
			continue;
		}
		len = separator - filename;
		if (len != prefixLen) {
			continue;
		}
		if (strncmp(filename, prefix, prefixLen) == 0) {
			int nlen;
			separator += infixLen;
			snprintf(path, PATH_MAX - 1, "%%u%s%%n", suffix);
			unsigned increment;
			if (sscanf(separator, path, &increment, &nlen) < 1) {
				continue;
			}
			len = strlen(separator);
			if (nlen < (ssize_t) len) {
				continue;
			}
			if (next <= increment) {
				next = increment + 1;
			}
		}
	}
	snprintf(path, PATH_MAX - 1, "%s%s%u%s", prefix, infix, next, suffix);
	path[PATH_MAX - 1] = '\0';
	return dir->openFile(dir, path, mode);
}
