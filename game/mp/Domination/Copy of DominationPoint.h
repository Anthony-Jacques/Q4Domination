#pragma once

#define MAX_DOMINATION_POINTS	64

#define TEAM_NEUTRAL TEAM_MAX


class ajDominationPoint : public idItem 
{
public:
	CLASS_PROTOTYPE( ajDominationPoint );
	
							ajDominationPoint();
							~ajDominationPoint();
							
	void					Spawn();
//	virtual bool			GiveToPlayer ( idPlayer* player );
	virtual bool			Pickup( idPlayer *player );
	
	virtual void			Think( void );

	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );

	void					IndicateStateChange();
private:
	const idDeclSkin*		m_teamSkin[TEAM_MAX+1];

	int						m_pointIndex;
	int						m_nextScoreTime;

public:
	static ajDominationPoint* m_allPoints[MAX_DOMINATION_POINTS];
	static					int m_countPoints;
};