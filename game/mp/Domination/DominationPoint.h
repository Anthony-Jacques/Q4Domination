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

	virtual bool			Pickup( idPlayer *player );
	virtual void			Think( void );

	void					IndicateStateChange();
	void					IndicateCurrentState();

	static void				ReadDominationPointChange(const idBitMsg &msg);
	void					WriteDominationPointChange(bool playSound);

	team_t					GetTeam() { return m_team; }
	void					SetTeam(team_t newVal) { m_team = newVal; }

protected:
	const idDeclSkin*		m_teamSkin[TEAM_MAX+1];

	int						m_pointIndex;
	int						m_nextScoreTime;

	static					int m_countPoints;

	team_t					m_team;
};