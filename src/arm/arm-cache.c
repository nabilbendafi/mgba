/* Copyright (c) 2013-2014 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "arm.h"

#include "isa-inlines.h"

#define BASE_LENGTH 8

static inline void _runARM(struct ARMCore* cpu, uint32_t opcode) {
	unsigned condition = opcode >> 28;
	if (condition != 0xE) {
		bool conditionMet = false;
		switch (condition) {
		case 0x0:
			conditionMet = ARM_COND_EQ;
			break;
		case 0x1:
			conditionMet = ARM_COND_NE;
			break;
		case 0x2:
			conditionMet = ARM_COND_CS;
			break;
		case 0x3:
			conditionMet = ARM_COND_CC;
			break;
		case 0x4:
			conditionMet = ARM_COND_MI;
			break;
		case 0x5:
			conditionMet = ARM_COND_PL;
			break;
		case 0x6:
			conditionMet = ARM_COND_VS;
			break;
		case 0x7:
			conditionMet = ARM_COND_VC;
			break;
		case 0x8:
			conditionMet = ARM_COND_HI;
			break;
		case 0x9:
			conditionMet = ARM_COND_LS;
			break;
		case 0xA:
			conditionMet = ARM_COND_GE;
			break;
		case 0xB:
			conditionMet = ARM_COND_LT;
			break;
		case 0xC:
			conditionMet = ARM_COND_GT;
			break;
		case 0xD:
			conditionMet = ARM_COND_LE;
			break;
		default:
			break;
		}
		if (!conditionMet) {
			cpu->cycles += ARM_PREFETCH_CYCLES;
			++cpu->cache.arm.instruction;
			return;
		}
	}
	ARMInstruction instruction = *cpu->cache.arm.instruction;
	++cpu->cache.arm.instruction;
	instruction(cpu, opcode);
 }

static inline void _runThumb(struct ARMCore* cpu, uint16_t opcode) {
	ThumbInstruction instruction = *cpu->cache.thumb.instruction;
	++cpu->cache.thumb.instruction;
	instruction(cpu, opcode);
}

static void _endBlockARM(struct ARMCore* cpu, uint32_t opcode) {
	if (!cpu->cache.block->nextBlock) {
		struct ARMCacheBlock* block = cpu->cache.block;
		ARMCacheFindBlockARM(cpu, cpu->gprs[ARM_PC] - WORD_SIZE_ARM * 2);
		block->nextBlock = cpu->cache.block;
	} else {
		cpu->cache.block = cpu->cache.block->nextBlock;
		cpu->cache.arm = cpu->cache.block->arm;
	}
	cpu->cache.arm.data += 3;
	_runARM(cpu, opcode);
}

static void _endBlockThumb(struct ARMCore* cpu, uint16_t opcode) {
	if (!cpu->cache.block->nextBlock) {
		struct ARMCacheBlock* block = cpu->cache.block;
		ARMCacheFindBlockThumb(cpu, cpu->gprs[ARM_PC] - WORD_SIZE_THUMB * 2);
		block->nextBlock = cpu->cache.block;
	} else {
		cpu->cache.block = cpu->cache.block->nextBlock;
		cpu->cache.thumb = cpu->cache.block->thumb;
	}
	cpu->cache.thumb.data += 3;
	_runThumb(cpu, opcode);
}

static inline void ARMCacheStepARM(struct ARMCore* cpu) {
	uint32_t opcode = cpu->prefetch[0];
	cpu->gprs[ARM_PC] += WORD_SIZE_ARM;
	cpu->prefetch[0] = cpu->prefetch[1];
	cpu->prefetch[1] = *cpu->cache.arm.data;
	++cpu->cache.arm.data;
	_runARM(cpu, opcode);
}

static inline void ARMCacheStepThumb(struct ARMCore* cpu) {
	uint32_t opcode = cpu->prefetch[0];
	cpu->gprs[ARM_PC] += WORD_SIZE_THUMB;
	cpu->prefetch[0] = cpu->prefetch[1];
	cpu->prefetch[1] = *cpu->cache.thumb.data;
	++cpu->cache.thumb.data;
	_runThumb(cpu, opcode);
}

void ARMCacheRunLoop(struct ARMCore* cpu) {
	if (cpu->cache.active) {
		if (cpu->executionMode == MODE_THUMB) {
			while (cpu->cycles < cpu->nextEvent) {
				ARMCacheStepThumb(cpu);
			}
		} else {
			while (cpu->cycles < cpu->nextEvent) {
				ARMCacheStepARM(cpu);
			}
		}
		cpu->irqh.processEvents(cpu);
	} else {
		ARMRunLoop(cpu);
	}
}

void ARMCacheInit(struct ARMCore* cpu) {
	TableInit(&cpu->cache.armCache, 0x100, free);
	TableInit(&cpu->cache.thumbCache, 0x100, free);
	cpu->cache.active = false;
	cpu->cache.block = 0;
}

void ARMCacheDeinit(struct ARMCore* cpu) {
	TableDeinit(&cpu->cache.armCache);
	TableDeinit(&cpu->cache.thumbCache);
}

void ARMCacheFindBlockARM(struct ARMCore* cpu, uint32_t addr) {
	struct ARMCacheBlock* block = TableLookup(&cpu->cache.armCache, addr >> 2);
	if (!block) {
		ARMInstruction* ichain = malloc(sizeof(ARMInstruction) * BASE_LENGTH);
		uint32_t* dchain = malloc(sizeof(uint32_t) * BASE_LENGTH);
		size_t chainUsed = 0;
		size_t chainSize = BASE_LENGTH;
		struct ARMInstructionInfo info;
		int blockEnded = 0;
		while (true) {
			if (chainUsed == chainSize) {
				chainSize *= 2;
				ichain = realloc(ichain, sizeof(ARMInstruction) * chainSize);
				dchain = realloc(dchain, sizeof(uint32_t) * chainSize);
			}
			LOAD_32(dchain[chainUsed], (addr + WORD_SIZE_ARM * chainUsed) & cpu->memory.activeMask, cpu->memory.activeRegion);
			uint32_t opcode = dchain[chainUsed];
			if (!blockEnded) {
				ichain[chainUsed] = _armTable[((opcode >> 16) & 0xFF0) | ((opcode >> 4) & 0x00F)];
				ARMDecodeARM(opcode, &info);
				if (info.branchType || info.traps) {
					blockEnded = 1;
				}
			} else if (blockEnded == 1) {
				ichain[chainUsed] = _endBlockARM;
				unsigned condition = opcode >> 28;
				if (condition == 0xE) {
					blockEnded = 2;
				}
			} else {
				++blockEnded;
				if (blockEnded == 4) {
					// Prefetch full
					break;
				}
			}
			++chainUsed;
		}
		block = malloc(sizeof(*block));
		block->arm.instruction = ichain;
		block->arm.data = dchain;
		block->nextBlock = 0;
		block->branch = 0;
		block->branchAddress = 0xFFFFFFFF;
		TableInsert(&cpu->cache.armCache, addr >> 2, block);
	}
	cpu->cache.block = block;
	cpu->cache.arm = block->arm;
}

void ARMCacheFindBlockThumb(struct ARMCore* cpu, uint32_t addr) {
	struct ARMCacheBlock* block = TableLookup(&cpu->cache.thumbCache, addr >> 1);
	if (!block) {
		ThumbInstruction* ichain = malloc(sizeof(ThumbInstruction) * BASE_LENGTH);
		uint16_t* dchain = malloc(sizeof(uint16_t) * BASE_LENGTH);
		size_t chainUsed = 0;
		size_t chainSize = BASE_LENGTH;
		struct ARMInstructionInfo info;
		int blockEnded = 0;
		while (true) {
			if (chainUsed == chainSize) {
				chainSize *= 2;
				ichain = realloc(ichain, sizeof(ThumbInstruction) * chainSize);
				dchain = realloc(dchain, sizeof(uint16_t) * chainSize);
			}
			LOAD_16(dchain[chainUsed], (addr + WORD_SIZE_THUMB * chainUsed) & cpu->memory.activeMask, cpu->memory.activeRegion);
			uint32_t opcode = dchain[chainUsed];
			if (!blockEnded) {
				ichain[chainUsed] = _thumbTable[opcode >> 6];
				ARMDecodeThumb(opcode, &info);
				if (info.branchType || info.traps) {
					blockEnded = 1;
				}
			} else if (blockEnded) {
				ichain[chainUsed] = _endBlockThumb;
				++blockEnded;
				if (blockEnded == 4) {
					// Prefetch full
					break;
				}
			}
			++chainUsed;
		}
		block = malloc(sizeof(*block));
		block->thumb.instruction = ichain;
		block->thumb.data = dchain;
		block->nextBlock = 0;
		block->branch = 0;
		block->branchAddress = 0xFFFFFFFF;
		TableInsert(&cpu->cache.thumbCache, addr >> 1, block);
	}
	cpu->cache.block = block;
	cpu->cache.thumb = block->thumb;
}
