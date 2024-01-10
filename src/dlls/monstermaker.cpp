/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// Monster Maker - this is an entity that creates monsters
// in the game.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cmbase.h"
#include "cmbasemonster.h"
#include "cmbaseextra.h"
#include "monsters.h"

// Monstermaker spawnflags
#define	SF_MONSTERMAKER_START_ON	1 // start active ( if has targetname )
#define	SF_MONSTERMAKER_CYCLIC		4 // drop one monster every time fired.
#define SF_MONSTERMAKER_MONSTERCLIP	8 // Children are blocked by monsterclip

extern monster_type_t monster_types[];
extern edict_t* spawn_monster(int monster_type, Vector origin, Vector angles, int spawnflags, pKVD *keyvalue);


// ========================================================
void CMMonsterMaker :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq(pkvd->szKeyName, "monstercount") )
	{
		m_cNumMonsters = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "m_imaxlivechildren") )
	{
		m_iMaxLiveChildren = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "monstertype") )
	{
		// Process monster_index
		int mIndex;
		for (mIndex = 0; monster_types[mIndex].name[0]; mIndex++)
		{
			if (strcmp(pkvd->szValue, monster_types[mIndex].name) == 0)
			{
				m_iMonsterIndex = mIndex;
				break; // grab the first entry we find
			}
		}
		if (monster_types[mIndex].name[0] == 0)
		{
			ALERT ( at_logged, "[MONSTER] MonsterMaker - %s is not a valid monster type!\n", pkvd->szValue );
			m_iMonsterIndex = -1;
		}
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "new_model") )
	{
		m_iszCustomModel = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "bloodcolor") )
	{
		m_iMonsterBlood = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "respawn_as_playerally") )
	{
		if (atoi(pkvd->szValue))
			m_iClassifyOverride = CLASS_PLAYER_ALLY;
		pkvd->fHandled = TRUE;
	}
	// These are to keep consistency with Sven Co-op's squadmaker entity.
	// CMBaseMonster::KeyValue will process TriggerCondition/TriggerTarget
	// keyvalues in the same way.
	else if ( FStrEq(pkvd->szKeyName, "trigger_condition") )
	{
		m_iTriggerCondition = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "trigger_target") )
	{
		m_iszTriggerTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CMBaseMonster::KeyValue( pkvd );
}


void CMMonsterMaker :: Spawn( )
{
	// likely omitted keyvalue, but it could truly be an alien grunt spawn
	if ( m_iMonsterIndex == 0 )
	{
		if ( !monster_types[0].need_to_precache )
		{
			// monstertype was not defined
			ALERT ( at_logged, "[MONSTER] Spawned a monstermaker entity without a monstertype! targetname: \"%s\"\n", STRING(pev->targetname) );
			m_iMonsterIndex = -1;
		}
	}

	pev->solid = SOLID_NOT;
	
	m_cLiveChildren = 0;
	Precache();
	if ( !FStringNull ( pev->targetname ) )
	{
		if ( pev->spawnflags & SF_MONSTERMAKER_CYCLIC )
		{
			SetUse ( &CMMonsterMaker::CyclicUse );// drop one monster each time we fire
		}
		else
		{
			SetUse ( &CMMonsterMaker::ToggleUse );// so can be turned on/off
		}

		if ( FBitSet ( pev->spawnflags, SF_MONSTERMAKER_START_ON ) )
		{// start making monsters as soon as monstermaker spawns
			m_fActive = TRUE;
			SetThink ( &CMMonsterMaker::MakerThink );
		}
		else
		{// wait to be activated.
			m_fActive = FALSE;
			SetThink ( &CMMonsterMaker::SUB_DoNothing );
		}
	}
	else
	{// no targetname, just start.
			pev->nextthink = gpGlobals->time + m_flDelay;
			m_fActive = TRUE;
			SetThink ( &CMMonsterMaker::MakerThink );
	}
	
	// always fade
	m_fFadeChildren = TRUE;
	
	m_flGround = 0;
	pev->classname = MAKE_STRING("monstermaker");
}

void CMMonsterMaker :: Precache( void )
{
	CMBaseMonster::Precache();
	// choosen monster is auto-precached
}

//=========================================================
// MakeMonster-  this is the code that drops the monster
//=========================================================
void CMMonsterMaker::MakeMonster( void )
{
	// monstermaker incorrectly setup or intentionally empty
	if ( m_iMonsterIndex == -1 )
	{
		ALERT ( at_console, "[MONSTER] NULL Ent in MonsterMaker!\n" );
		return;
	}

	edict_t *pent;
	pKVD keyvalue[MAX_KEYVALUES]; // sometimes, i don't know what am i doing. -Giegue
	int createSF = SF_MONSTER_FALL_TO_GROUND;

	if ( m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren )
	{// not allowed to make a new one yet. Too many live ones out right now.
		return;
	}

	if ( !m_flGround )
	{
		// set altitude. Now that I'm activated, any breakables, etc should be out from under me. 
		TraceResult tr;

		UTIL_TraceLine ( pev->origin, pev->origin - Vector ( 0, 0, 2048 ), ignore_monsters, ENT(pev), &tr );
		m_flGround = tr.vecEndPos.z;
	}

	Vector mins = pev->origin - Vector( 34, 34, 0 );
	Vector maxs = pev->origin + Vector( 34, 34, 0 );
	maxs.z = pev->origin.z;
	mins.z = m_flGround;

	edict_t *pList[2];
	int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_CLIENT|FL_MONSTER );
	if ( count )
	{
		// don't build a stack of monsters!
		return;
	}
	
	// Should children hit monsterclip brushes?
	if ( pev->spawnflags & SF_MONSTERMAKER_MONSTERCLIP )
		createSF |= SF_MONSTER_HITMONSTERCLIP;

	/* KEYVALUES */
	// Monster is to have a custom model?
	if ( !FStringNull( m_iszCustomModel ) )
	{
		// setup model keyvalue
		strcpy(keyvalue[0].key, "model");
		strcpy(keyvalue[0].value, STRING( m_iszCustomModel ));
	}
	// Override monster blood color?
	if ( m_iMonsterBlood )
	{
		// setup blood keyvalue
		strcpy(keyvalue[1].key, "bloodcolor");
		sprintf(keyvalue[1].value, "%i", m_iMonsterBlood);
	}
	// Trigger conditions set?
	if ( !FStringNull( m_iszTriggerTarget ) )
	{
		// setup trigger keyvalues
		strcpy(keyvalue[2].key, "TriggerCondition");
		sprintf(keyvalue[2].value, "%i", m_iTriggerCondition);
		strcpy(keyvalue[3].key, "TriggerTarget");
		strcpy(keyvalue[3].value, STRING( m_iszTriggerTarget ));
	}
	// Weapons (For hgrunt/massn/rgrunt/etc...)
	if ( pev->weapons )
	{
		strcpy(keyvalue[4].key, "weapons");
		sprintf(keyvalue[4].value, "%i", pev->weapons);
	}
	// Monster's Name
	if ( !FStringNull( m_szMonsterName ) )
	{
		strcpy(keyvalue[5].key, "displayname");
		strcpy(keyvalue[5].value, STRING( m_szMonsterName ));
	}
	// Classify override
	if ( m_iClassifyOverride )
	{
		strcpy(keyvalue[6].key, "classify");
		sprintf(keyvalue[6].value, "%i", m_iClassifyOverride);
	}
	// Custom health
	if (pev->health)
	{
		strcpy(keyvalue[7].key, "health");
		sprintf(keyvalue[7].value, "%f", pev->health);
	}

	// Attempt to spawn monster
	pent = spawn_monster(m_iMonsterIndex, pev->origin, pev->angles, createSF, keyvalue);
	if ( pent == NULL )
	{
		ALERT ( at_console, "[MONSTER] MonsterMaker - failed to spawn monster! targetname: \"%s\"\n", STRING(pev->targetname) );
		return;
	}
	
	// If I have a target, fire!
	if ( !FStringNull ( pev->target ) )
	{
		// delay already overloaded for this entity, so can't call SUB_UseTargets()
		FireTargets( STRING(pev->target), this->edict(), this->edict(), USE_TOGGLE, 0 );
	}
	
	pent->v.owner = edict();

	if ( !FStringNull( pev->netname ) )
	{
		// if I have a netname (overloaded), give the child monster that name as a targetname
		pent->v.targetname = pev->netname;
	}
	
	// Pass parent's rendering effects to child
	pent->v.rendermode = pev->rendermode;
	pent->v.renderfx = pev->renderfx;
	pent->v.renderamt = pev->renderamt;
	pent->v.rendercolor = pev->rendercolor;

	// Soundlist isn't "exactly" a keyvalue so pass it here
	if ( m_srSoundList != NULL )
	{
		// it needs to be allocated first
		CMBaseMonster *pChild = GetClassPtr((CMBaseMonster *)VARS(pent));
		pChild->m_srSoundList = (REPLACER::REPLACER*)calloc(MAX_REPLACEMENTS, sizeof(*pChild->m_srSoundList));

		memcpy(pChild->m_srSoundList, m_srSoundList, sizeof(REPLACER::REPLACER));
		pChild->m_isrSounds = m_isrSounds;
	}

	m_cLiveChildren++;// count this monster
	m_cNumMonsters--;

	if ( m_cNumMonsters == 0 )
	{
		// Disable this forever.  Don't kill it because it still gets death notices
		SetThink( NULL );
		SetUse( NULL );
	}
}

//=========================================================
// CyclicUse - drops one monster from the monstermaker
// each time we call this.
//=========================================================
void CMMonsterMaker::CyclicUse ( edict_t *pActivator, edict_t *pCaller, USE_TYPE useType, float value )
{
	MakeMonster();
}

//=========================================================
// ToggleUse - activates/deactivates the monster maker
//=========================================================
void CMMonsterMaker :: ToggleUse ( edict_t *pActivator, edict_t *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_fActive ) )
		return;

	if ( m_fActive )
	{
		m_fActive = FALSE;
		SetThink ( NULL );
	}
	else
	{
		m_fActive = TRUE;
		SetThink ( &CMMonsterMaker::MakerThink );
	}

	pev->nextthink = gpGlobals->time;
}

//=========================================================
// MakerThink - creates a new monster every so often
//=========================================================
void CMMonsterMaker :: MakerThink ( void )
{
	pev->nextthink = gpGlobals->time + m_flDelay;

	MakeMonster();
}


//=========================================================
//=========================================================
void CMMonsterMaker :: DeathNotice ( entvars_t *pevChild )
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	m_cLiveChildren--;

	if ( !m_fFadeChildren )
	{
		pevChild->owner = NULL;
	}
}
