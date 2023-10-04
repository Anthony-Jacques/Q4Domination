#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../Game_local.h"
#include "DominationPoint.h"
#include "DominationGameState.h"


CLASS_DECLARATION( idItem, ajDominationPoint )
END_CLASS


ajDominationPoint* ajDominationPoint::m_allPoints[MAX_DOMINATION_POINTS];
int ajDominationPoint::m_countPoints = 0;

ajDominationPoint::ajDominationPoint()
{
	m_pointIndex = m_countPoints++;
	m_allPoints[m_pointIndex] = this;
}

ajDominationPoint::~ajDominationPoint()
{
	m_allPoints[m_pointIndex] = NULL;

	m_countPoints--;
}

void ajDominationPoint::Spawn()
{
	//idItem::Spawn();

	m_nextScoreTime = 0;

	m_teamSkin[TEAM_MARINE] = declManager->FindSkin( "skins/domination_point_marine", false );
	m_teamSkin[TEAM_STROGG] = declManager->FindSkin( "skins/domination_point_strogg", false );
	m_teamSkin[TEAM_MAX] = declManager->FindSkin( "skins/domination_point_neutral", false );
}

bool ajDominationPoint::Pickup( idPlayer *player )
{
	// fidle with  pickupSkin
	//return idItem::Pickup(player);

	ajDominationGameState *theState = (ajDominationGameState*)gameLocal.mpGame.GetGameState();
	if (player->team == theState->m_pointState[m_pointIndex])
		return false;

	// record start time

	// Tell the rest of the world it just changed
	theState->m_pointState[m_pointIndex] = (team_t)player->team;

	gameLocal.mpGame.AddPlayerScore(player, 1);

	return true;
}

void ajDominationPoint::IndicateStateChange()
{
	team_t theTeam = ((ajDominationGameState*)gameLocal.mpGame.GetGameState())->m_pointState[m_pointIndex];
	SetSkin( m_teamSkin[theTeam] );

	if (theTeam != TEAM_NEUTRAL)
	{
 		if (gameLocal.isMultiplayer && spawnArgs.GetBool( "globalAcquireSound")) 
		{
			gameLocal.mpGame.PlayGlobalItemAcquireSound(entityDefNumber);
		} else 
		{
			StartSound("snd_acquire", SND_CHANNEL_ITEM, 0, false, NULL);
		}
	}

	Event_RespawnFx();

	// record the next time we need to increase the score
	int time = gameLocal.time;
	m_nextScoreTime =  time - (time % 2000) + 2000;

	idPlayer* player = gameLocal.GetLocalPlayer();
	if(player && player->mphud) 
	{
		player->mphud->SetStateInt("domPoint", m_pointIndex);
		player->mphud->SetStateInt("team", theTeam);
		player->mphud->HandleNamedEvent("DominationPointCaptured");

		/*idStr capturedString;
		if (this->team == (team_t)player->team) 
			capturedString = "Your team captures point " + idStr::FormatNumber(m_pointIndex);
		else
			capturedString = "The other team captures point " + idStr::FormatNumber(m_pointIndex);

		player->mphud->SetStateString("main_notice_text", capturedString);

		player->mphud->HandleNamedEvent("main_notice");
*/	}
}

void ajDominationPoint::Think()
{
	idItem::Think();

	// increase scores

	while (gameLocal.time > m_nextScoreTime)
	{
		m_nextScoreTime += 2000;

		team_t theTeam = ((ajDominationGameState*)gameLocal.mpGame.GetGameState())->m_pointState[m_pointIndex];
		gameLocal.mpGame.AddTeamScore(theTeam, 1);
	}

}

bool ajDominationPoint::Collide( const trace_t &collision, const idVec3 &velocity )
{
	return idItem::Collide(collision, velocity);
}

