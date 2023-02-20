#ifndef BASEEXTRA_H
#define BASEEXTRA_H

// any extra entities created in this project that aren't
// monsters will go here

//=========================================================
// MonsterMaker - this ent creates monsters during the game.
//=========================================================
class CMMonsterMaker : public CMBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData* pkvd);
	void EXPORT ToggleUse ( edict_t *pActivator, edict_t *pCaller, USE_TYPE useType, float value );
	void EXPORT CyclicUse ( edict_t *pActivator, edict_t *pCaller, USE_TYPE useType, float value );
	void EXPORT MakerThink ( void );
	void DeathNotice ( entvars_t *pevChild );// monster maker children use this to tell the monster maker that they have died.
	void MakeMonster( void );
	
	int m_iMonsterIndex;// index of the monster(s) that will be created.
	
	int	 m_cNumMonsters;// max number of monsters this ent can create
	int	 m_iMaxLiveChildren;// max number of monsters that this maker may have out at one time.

	int  m_cLiveChildren;// how many monsters made by this monster maker that are currently alive

	float m_flGround; // z coord of the ground under me, used to make sure no monsters are under the maker when it drops a new child

	BOOL m_fActive;
	BOOL m_fFadeChildren;// should we make the children fadeout?
};

#endif // BASEEXTRA_H
