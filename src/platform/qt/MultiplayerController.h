/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef QGBA_MULTIPLAYER_CONTROLLER
#define QGBA_MULTIPLAYER_CONTROLLER

#include <QObject>

extern "C" {
#include "gba/sio/lockstep.h"
}

namespace QGBA {

class GameController;

class MultiplayerController : public QObject {
Q_OBJECT

public:
	MultiplayerController();
	~MultiplayerController();

	bool attachGame(GameController*);
	void detachGame(GameController*);

	int attached();
	int playerId(GameController*);

signals:
	void gameAttached();
	void gameDetached();

private:
	GBASIOLockstep m_lockstep;
};

}
#endif
