//----------------------------------------------------------------
// GameState.cpp
//
// Copyright 2002-2005 Raven Software
//----------------------------------------------------------------

#include "../../../idlib/precompiled.h"
#pragma hdrstop

#include "DominationGameState.h"

/*
===============================================================================

rvTeamDMGameState

Game state info for Team DM

===============================================================================
*/
ajDominationGameState::ajDominationGameState( bool allocPrevious ) : rvGameState( false ) {
	Clear();

	if( allocPrevious ) {
		previousGameState = new ajDominationGameState( false );
	} else {
		previousGameState = NULL;
	}

	trackPrevious = allocPrevious;

	type = GS_DOMINATION;
}
void ajDominationGameState::NewState( mpGameState_t newState ) 
{
	switch( newState ) 
	{
	case COUNTDOWN:
	case NEXTGAME:
	case GAMEON:
	case WARMUP:
	case GAMEREVIEW:
		for( int i = 0; i < MAX_DOMINATION_POINTS; i++ ) {
			m_pointState[i] = TEAM_NEUTRAL;
		}
		if ( gameLocal.localClientNum >= 0 ) {
			GameStateChanged();
		}
		break;
	}
	rvGameState::NewState(newState);
}

void ajDominationGameState::Run( void ) 
{
	rvGameState::Run();

	switch( currentState ) {
		case GAMEON: {
			int team = ( ( gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) >= gameLocal.serverInfo.GetInt( "si_dominationlimit" ) ) ? TEAM_MARINE : ( ( gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG ) >= gameLocal.serverInfo.GetInt( "si_dominationlimit" ) ) ? TEAM_STROGG : -1 ) );
			if( gameLocal.serverInfo.GetInt( "si_dominationlimit" ) <= 0 ) {
				// no fraglimit
				team = -1;
			}
			bool tiedForFirst = gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) == gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG );

			// rjohnson: 9920
			int		numPlayers = 0;
			for( int i = 0; i < gameLocal.numClients; i++ ) {
				idEntity *ent = gameLocal.entities[ i ];
				if ( ent && ent->IsType( idPlayer::GetClassType() ) ) {
					numPlayers++;
				}
			}

			if ( team >= 0 && !tiedForFirst ) {
				if ( !fragLimitTimeout ) {
					common->DPrintf( "enter FragLimit timeout, team %d is leader\n", team );
					fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
				}
				if ( gameLocal.time > fragLimitTimeout ) {
					NewState( GAMEREVIEW );

					gameLocal.mpGame.PrintMessageEvent( -1, MSG_FRAGLIMIT, team );

				}
			} else if ( fragLimitTimeout ) {
				// frag limit was hit and cancelled. means the two teams got even during FRAGLIMIT_DELAY
				// enter sudden death, the next frag leader will win
				//
				// jshepard: OR it means that the winner killed himself during the fraglimit delay, and the
				// game needs to roll on.
				if( tiedForFirst )	{
					//this is a tie
					if( gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) >= gameLocal.serverInfo.GetInt( "si_dominationlimit" ) )	{
						//and it's tied at the fraglimit.
						NewState( SUDDENDEATH ); 
					}
					//not a tie, game on.
					fragLimitTimeout = 0;
				}
			} else if ( gameLocal.mpGame.TimeLimitHit() ) {
				gameLocal.mpGame.PrintMessageEvent( -1, MSG_TIMELIMIT );
				// rjohnson: 9920
				if( tiedForFirst && numPlayers ) {
					// if tied at timelimit hit, goto sudden death
					fragLimitTimeout = 0;
					NewState( SUDDENDEATH );
				} else {
					// or just end the game
					NewState( GAMEREVIEW );
				}
			} else if( tiedForFirst && team >= 0 ) {
				// check for the rare case that two teams both hit the fraglimit the same frame
				// two people tied at fraglimit, advance to sudden death after a delay
				fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
			}
			break;
		}

		case SUDDENDEATH: {
			int team = gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) > gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG ) ? TEAM_MARINE : TEAM_STROGG;
			bool tiedForFirst = false;
			if( gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) == gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG ) ) {
				team = -1;
				tiedForFirst = true;
			}

			if ( team >= 0 && !tiedForFirst ) {
				if ( !fragLimitTimeout ) {
					common->DPrintf( "enter sudden death FragLeader timeout, team %d is leader\n", team );
					fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
				}
				if ( gameLocal.time > fragLimitTimeout ) {
					NewState( GAMEREVIEW );
					gameLocal.mpGame.PrintMessageEvent( -1, MSG_FRAGLIMIT, team );
				}
			} else if ( fragLimitTimeout ) {
				gameLocal.mpGame.PrintMessageEvent( -1, MSG_HOLYSHIT );
				fragLimitTimeout = 0;
			} 

			break;
		}
	}
}


/*
================
ajDominationGameState::Clear
================
*/
void ajDominationGameState::Clear( void ) {
	rvGameState::Clear();

	// mekberg: clear previous game state.
	if ( previousGameState ) {
		previousGameState->Clear( );
	}		

	for( int i = 0; i < MAX_DOMINATION_POINTS; i++ ) {
		m_pointState[i] = TEAM_NEUTRAL;
	}
}

/*
================
ajDominationGameState::SendState
================
*/

// ADJ - erm, why isnt this in some base, with WriteState as a virtual? 
void ajDominationGameState::SendState( int clientNum ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	assert( gameLocal.isServer && trackPrevious && type == GS_DOMINATION );

	if (!previousGameState)
		return;

	if( clientNum == -1 && (ajDominationGameState&)(*this) == (ajDominationGameState&)(*previousGameState) ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_GAMESTATE );

	WriteState( outMsg );

	networkSystem->ServerSendReliableMessage( clientNum, outMsg );

	// don't update the state if we are working for a single client
	if ( clientNum == -1 ) {
		outMsg.ReadByte(); // pop off the msg ID
		ReceiveState( outMsg );
	}
}



/*
================
ajDominationGameState::ReceiveState
================
*/

// ADJ - hmmm, seems like shouldnt need to do this. Maybe if you provide the correct operator= the base
// implementation will do whats needed? 
void ajDominationGameState::ReceiveState( const idBitMsg& msg ) {
	assert( type == GS_DOMINATION );

	UnpackState( msg );

	if ( gameLocal.localClientNum >= 0 ) {
		GameStateChanged();
	}

	(ajDominationGameState&)(*previousGameState) = (ajDominationGameState&)(*this);
}

/*
================
ajDominationGameState::PackState
================
*/
void ajDominationGameState::PackState( idBitMsg& outMsg ) {
	// first of all write out base-class state
	rvGameState::PackState( outMsg );

	// use indexing to pack in info
	int index = 0;

	for( int i = 0; i < MAX_DOMINATION_POINTS; i++ ) {
		if( m_pointState[i] != ((ajDominationGameState*)previousGameState)->m_pointState[i]) {
			outMsg.WriteByte( index );
			outMsg.WriteByte( m_pointState[i] );
		}
		index++;
	}
}

/*
================
ajDominationGameState::UnpackState
================
*/
void ajDominationGameState::UnpackState( const idBitMsg& inMsg ) {
	rvGameState::UnpackState( inMsg );

	while( inMsg.GetRemainingData() ) {
		int index = inMsg.ReadByte();

		if( index >= 0 && index < MAX_DOMINATION_POINTS ) {
			m_pointState[index] = (team_t)inMsg.ReadByte();
		} else {
			gameLocal.Error( "ajDominationGameState::UnpackState() - Unknown data identifier '%d'\n", index );
		}
	}
}

/*
================
ajDominationGameState::SendInitialState
================
*/
// again, do we need this? Surely an operator= solves it?
void ajDominationGameState::SendInitialState( int clientNum ) {
	assert( type == GS_DOMINATION );

	ajDominationGameState* previousState = (ajDominationGameState*)previousGameState;

	ajDominationGameState invalidState;

	previousGameState = &invalidState;

	SendState( clientNum );

	previousGameState = previousState;
}

/*
================
rvCTFGameState::GameStateChanged
================
*/
void ajDominationGameState::GameStateChanged( void ) {
	// detect any base state changes
	rvGameState::GameStateChanged();

	// Domination specific stuff
	for( int i = 0; i < MAX_DOMINATION_POINTS; i++ ) 
	{
		if( m_pointState[i] == ((ajDominationGameState*)previousGameState)->m_pointState[i] ) 
			continue;

		// don't play flag messages when flag state changes as a result of the gamestate changing
		if( currentState != ((ajDominationGameState*)previousGameState)->currentState && 
			((ajDominationGameState*)previousGameState)->currentState != INACTIVE)
			continue;

		if (((ajDominationGameState*)previousGameState)->currentState == INACTIVE)
			continue;

		ajDominationPoint::m_allPoints[i]->IndicateStateChange();
	}
}


/*
================
m_pointState::operator==
================
*/
bool ajDominationGameState::operator==( const ajDominationGameState& rhs ) const 
{
	if ((rvGameState&)(*this) != (rvGameState&)rhs )
		return false;

	for(int i = 0; i < MAX_DOMINATION_POINTS; i++) 
	{
		if( m_pointState[i] != rhs.m_pointState[i]) 
			return false;
	}

	return true;
}

/*
================
m_pointState::operator=
================
*/
ajDominationGameState& ajDominationGameState::operator=( const ajDominationGameState& rhs ) {
	(rvGameState&)(*this) = (rvGameState&)rhs;

	for (int i = 0; i < MAX_DOMINATION_POINTS; i++) 
	{
		m_pointState[i] = rhs.m_pointState[i];
	}

	return (*this);
}

