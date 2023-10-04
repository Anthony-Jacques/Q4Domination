//----------------------------------------------------------------
// GameState.h
//
// Copyright 2002-2005 Raven Software
//----------------------------------------------------------------

#ifndef __DOMINATION_GAMESTATE_H__
#define __DOMINATION_GAMESTATE_H__

#include "../GameState.h"
#include "DominationPoint.h"


/*
===============================================================================

ajDominationGameState

Game state info for Domination

===============================================================================
*/
class ajDominationGameState : public rvGameState 
{
public:
	ajDominationGameState(bool allocPrevious = true);

	virtual void Run(void);
	virtual void NewState(mpGameState_t newState);

	virtual void SendInitialState(int clientNum);
	virtual void GameStateChanged(void);
	virtual void UnpackState(const idBitMsg& inMsg);
	virtual void PackState(idBitMsg& outMsg);
	virtual void ReceiveState(const idBitMsg& msg);
	virtual void SendState(int clientNum);
	virtual void Clear(void);

	ajDominationGameState& operator=(const ajDominationGameState& rhs);
	bool operator==(const ajDominationGameState& rhs) const;

	team_t	m_pointState[MAX_DOMINATION_POINTS];

};



#endif
