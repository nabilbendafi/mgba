/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "rr.h"

#include "util/vfs.h"

void GBARRInitRecord(struct GBA* gba) {
	if (!gba || !gba->rr) {
		return;
	}

	if (gba->rr->initFrom & INIT_FROM_SAVEGAME) {
		if (gba->rr->savedata) {
			gba->rr->savedata->close(gba->rr->savedata);
		}
		gba->rr->savedata = gba->rr->openSavedata(gba->rr, O_TRUNC | O_CREAT | O_WRONLY);
		GBASavedataClone(&gba->memory.savedata, gba->rr->savedata);
		gba->rr->savedata->close(gba->rr->savedata);
		gba->rr->savedata = gba->rr->openSavedata(gba->rr, O_RDONLY);
		GBASavedataMask(&gba->memory.savedata, gba->rr->savedata);
	} else {
		GBASavedataMask(&gba->memory.savedata, 0);
	}

	if (gba->rr->initFrom & INIT_FROM_SAVESTATE) {
		struct VFile* vf = gba->rr->openSavestate(gba->rr, O_TRUNC | O_CREAT | O_RDWR);
		GBASaveStateNamed(gba, vf, false);
		vf->close(vf);
	} else {
		ARMReset(gba->cpu);
	}
}

void GBARRInitPlay(struct GBA* gba) {
	if (!gba || !gba->rr) {
		return;
	}

	if (gba->rr->initFrom & INIT_FROM_SAVEGAME) {
		if (gba->rr->savedata) {
			gba->rr->savedata->close(gba->rr->savedata);
		}
		gba->rr->savedata = gba->rr->openSavedata(gba->rr, O_RDONLY);
		GBASavedataMask(&gba->memory.savedata, gba->rr->savedata);
	} else {
		GBASavedataMask(&gba->memory.savedata, 0);
	}

	if (gba->rr->initFrom & INIT_FROM_SAVESTATE) {
		struct VFile* vf = gba->rr->openSavestate(gba->rr, O_RDONLY);
		GBALoadStateNamed(gba, vf);
		vf->close(vf);
	} else {
		ARMReset(gba->cpu);
	}
}

void GBARRDestroy(struct GBARRContext* rr) {
	if (rr->isPlaying(rr)) {
		rr->stopPlaying(rr);
	}
	if (rr->isRecording(rr)) {
		rr->stopRecording(rr);
	}
	if (rr->savedata) {
		rr->savedata->close(rr->savedata);
		rr->savedata = 0;
	}
	rr->destroy(rr);
}
