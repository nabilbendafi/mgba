/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef ARM_CACHE_H
#define ARM_CACHE_H

#include "decoder.h"
#include "isa-arm.h"
#include "isa-thumb.h"

#include "util/common.h"
#include "util/table.h"

struct ARMCacheBlockARM {
	ARMInstruction* instruction;
	uint32_t* data;
};

struct ARMCacheBlockThumb {
	ThumbInstruction* instruction;
	uint16_t* data;
};

struct ARMCacheBlock {
	union {
		struct ARMCacheBlockARM arm;
		struct ARMCacheBlockThumb thumb;
	};
	struct ARMCacheBlock* nextBlock;
	struct ARMCacheBlock* branch;
	uint32_t branchAddress;
};

struct ARMCache {
	struct ARMCacheBlock* block;
	union {
		struct ARMCacheBlockARM arm;
		struct ARMCacheBlockThumb thumb;
	};
	struct Table armCache;
	struct Table thumbCache;
	bool active;
};

void ARMCacheInit(struct ARMCore* cpu);
void ARMCacheDeinit(struct ARMCore* cpu);

void ARMCacheFindBlockARM(struct ARMCore* cpu, uint32_t addr);
void ARMCacheFindBlockThumb(struct ARMCore* cpu, uint32_t addr);

#endif
