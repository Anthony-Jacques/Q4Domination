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
ajDominationGameState::ajDominationGameState( bool allocPrevious ) : rvGameState( allocPrevious ) {
/*	Clear();

	if( allocPrevious ) {
		previousGameState = new ajDominationGameState( false );
	} else {
		previousGameState = NULL;
	}

	trackPrevious = allocPrevious;

	type = GS_DOMINATION;*/
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
		for(int i = 0; i < MAX_GENTITIES; i++ ) 
		{
			if (gameLocal.entities[i] && gameLocal.entities[i]->IsType(ajDominationPoint::GetClassType())) 
			{
				((ajDominationPoint*)gameLocal.entities[i])->SetTeam(TEAM_NEUTRAL);
				((ajDominationPoint*)gameLocal.entities[i])->IndicateCurrentState();
			}
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

					gameLocal.mpGame.PrintMessageEvent( -1, MSG_DOMLIMIT, team );

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
					gameLocal.mpGame.PrintMessageEvent( -1, MSG_DOMLIMIT, team );
				}
			} else if ( fragLimitTimeout ) {
				gameLocal.mpGame.PrintMessageEvent( -1, MSG_HOLYSHIT );
				fragLimitTimeout = 0;
			} 

			break;
		}
	}
}

