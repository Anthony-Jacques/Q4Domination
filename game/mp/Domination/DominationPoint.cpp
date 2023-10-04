#include "../../../idlib/precompiled.h"
#pragma hdrstop

#include "../../Game_local.h"
#include "DominationPoint.h"
#include "DominationGameState.h"


CLASS_DECLARATION( idItem, ajDominationPoint )
END_CLASS


int ajDominationPoint::m_countPoints = 0;

ajDominationPoint::ajDominationPoint()
{
	m_pointIndex = m_countPoints++;
	m_team = TEAM_NEUTRAL;
}

ajDominationPoint::~ajDominationPoint()
{
	m_countPoints--;
}

void ajDominationPoint::Spawn()
{
	m_team = TEAM_NEUTRAL;

	m_teamSkin[TEAM_MARINE] = declManager->FindSkin( "skins/domination_point_marine", false );
	m_teamSkin[TEAM_STROGG] = declManager->FindSkin( "skins/domination_point_strogg", false );
	m_teamSkin[TEAM_NEUTRAL] = declManager->FindSkin( "skins/domination_point_neutral", false );

	IndicateCurrentState();
}

bool ajDominationPoint::Pickup( idPlayer *player )
{
	if (player->team == m_team)
		return false;

	if ((gameLocal.mpGame.GetGameState()->GetMPGameState() == WARMUP) ||
		(gameLocal.mpGame.GetGameState()->GetMPGameState() == COUNTDOWN))
		return false;

	m_team = (team_t)player->team;

	// record the next time we need to increase the score
	int time = gameLocal.time;
	m_nextScoreTime =  time - (time % 2000) + 2000;

	// this only really matters
	IndicateStateChange();

	gameLocal.mpGame.AddPlayerScore(player, 1);

	if (gameLocal.isServer)
		WriteDominationPointChange(true);

	return true;
}

void ajDominationPoint::ReadDominationPointChange(const idBitMsg &msg)
{
	int theEnt = msg.ReadLong();
	int theTeam = msg.ReadBits(3);
	bool playSound = (bool)msg.ReadBits(1);

	for(int i = 0; i < MAX_GENTITIES; i++ ) 
	{
		if (gameLocal.entities[i] && gameLocal.entities[i]->entityNumber == theEnt) 
		{
			team_t oldTeam = ((ajDominationPoint*)gameLocal.entities[i])->m_team;
			((ajDominationPoint*)gameLocal.entities[i])->m_team = (team_t)theTeam;

			if (playSound && (oldTeam != theTeam))
				((ajDominationPoint*)gameLocal.entities[i])->IndicateStateChange();
			else
				((ajDominationPoint*)gameLocal.entities[i])->IndicateCurrentState();

			return;
		}
	}
}

void ajDominationPoint::WriteDominationPointChange(bool playSound)
{
	assert(gameLocal.isServer);

	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_DOMPOINT_STATECHANGE );

	outMsg.WriteLong(entityNumber);
	outMsg.WriteBits((int)m_team, 3);
	outMsg.WriteBits((int)playSound, 1);

	networkSystem->ServerSendReliableMessage(-1, outMsg );
}


void ajDominationPoint::IndicateStateChange()
{
	IndicateCurrentState();

	if (m_team == TEAM_STROGG)
		soundSystem->PlayShaderDirectly ( SOUNDWORLD_GAME, "sound/ambience/alarms_buzzers/1shot_alarm01.ogg" );		
	else if (m_team == TEAM_MARINE)
		soundSystem->PlayShaderDirectly ( SOUNDWORLD_GAME, "sound/ambience/alarms_buzzers/1shot_alarm02.ogg" );		

	Event_RespawnFx();
}

void ajDominationPoint::IndicateCurrentState()
{
	SetSkin( m_teamSkin[m_team] );

	idPlayer* player = gameLocal.GetLocalPlayer();
	if(player && player->mphud) 
	{
		idStr domPointName;
		spawnArgs.GetString("DomPointName", "Domination Point", domPointName);

		player->mphud->SetStateString("domPointName", domPointName.c_str());
		player->mphud->SetStateInt("domPoint", m_pointIndex);
		player->mphud->SetStateInt("team", m_team);
		player->mphud->HandleNamedEvent("DominationPointCaptured");
	}
}

void ajDominationPoint::Think()
{
	idItem::Think();

	if (gameLocal.isServer)
	{
		// increase scores, but only on the server as even a tiny bit of lag makes it look ugleh
		while (gameLocal.time > m_nextScoreTime)
		{
			m_nextScoreTime += 2000;

			if (m_team < TEAM_MAX)
				gameLocal.mpGame.AddTeamScore(m_team, 1);
		}
	}
}

