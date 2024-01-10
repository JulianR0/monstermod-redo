/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Agrunt - Dominant, warlike alien grunt monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cmbase.h"
#include	"cmbasemonster.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"hornet.h"
#include	"skill.h"


//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_AGRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_AGRUNT_THREAT_DISPLAY,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_AGRUNT_SETUP_HIDE_ATTACK = LAST_COMMON_TASK + 1,
	TASK_AGRUNT_GET_PATH_TO_ENEMY_CORPSE,
};

int iAgruntMuzzleFlash;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		AGRUNT_AE_HORNET1	( 1 )
#define		AGRUNT_AE_HORNET2	( 2 )
#define		AGRUNT_AE_HORNET3	( 3 )
#define		AGRUNT_AE_HORNET4	( 4 )
#define		AGRUNT_AE_HORNET5	( 5 )
// some events are set up in the QC file that aren't recognized by the code yet.
#define		AGRUNT_AE_PUNCH		( 6 )
#define		AGRUNT_AE_BITE		( 7 )

#define		AGRUNT_AE_LEFT_FOOT	 ( 10 )
#define		AGRUNT_AE_RIGHT_FOOT ( 11 )

#define		AGRUNT_AE_LEFT_PUNCH ( 12 )
#define		AGRUNT_AE_RIGHT_PUNCH ( 13 )

#define		AGRUNT_MELEE_DIST	100

const char *CMAGrunt::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CMAGrunt::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CMAGrunt::pAttackSounds[] =
{
	"agrunt/ag_attack1.wav",
	"agrunt/ag_attack2.wav",
	"agrunt/ag_attack3.wav",
};

const char *CMAGrunt::pFireSounds[] =
{
	"agrunt/ag_fire1.wav",
	"agrunt/ag_fire2.wav",
	"agrunt/ag_fire3.wav",
};

const char *CMAGrunt::pDieSounds[] =
{
	"agrunt/ag_die1.wav",
	"agrunt/ag_die4.wav",
	"agrunt/ag_die5.wav",
};

const char *CMAGrunt::pPainSounds[] =
{
	"agrunt/ag_pain1.wav",
	"agrunt/ag_pain2.wav",
	"agrunt/ag_pain3.wav",
	"agrunt/ag_pain4.wav",
	"agrunt/ag_pain5.wav",
};

const char *CMAGrunt::pIdleSounds[] =
{
	"agrunt/ag_idle1.wav",
	"agrunt/ag_idle2.wav",
	"agrunt/ag_idle3.wav",
	"agrunt/ag_idle4.wav",
};

const char *CMAGrunt::pAlertSounds[] =
{
	"agrunt/ag_alert1.wav",
	"agrunt/ag_alert3.wav",
	"agrunt/ag_alert4.wav",
	"agrunt/ag_alert5.wav",
};

//=========================================================
// IRelationship - overridden because Human Grunts are 
// Alien Grunt's nemesis.
//=========================================================
int CMAGrunt::IRelationship ( CMBaseEntity *pTarget )
{
	// ditto hgrunt.cpp
	/*
	if ( strcmp(STRING(pTarget->pev->model), "models/hgrunt.mdl") == 0 )
	{
		return R_NM;
	}
	*/
	
	return CMBaseMonster :: IRelationship( pTarget );
}

//=========================================================
// ISoundMask 
//=========================================================
int CMAGrunt :: ISoundMask ( void )
{
	return 0;
}

//=========================================================
// TraceAttack
//=========================================================
void CMAGrunt :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( ptr->iHitgroup == 10 && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB)))
	{
		// hit armor
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,10) < 1) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2) );
			pev->dmgtime = gpGlobals->time;
		}

		if ( RANDOM_LONG( 0, 1 ) == 0 )
		{
			Vector vecTracerDir = vecDir;

			vecTracerDir.x += RANDOM_FLOAT( -0.3, 0.3 );
			vecTracerDir.y += RANDOM_FLOAT( -0.3, 0.3 );
			vecTracerDir.z += RANDOM_FLOAT( -0.3, 0.3 );

			vecTracerDir = vecTracerDir * -512;

			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos );
			WRITE_BYTE( TE_TRACER );
				WRITE_COORD( ptr->vecEndPos.x );
				WRITE_COORD( ptr->vecEndPos.y );
				WRITE_COORD( ptr->vecEndPos.z );

				WRITE_COORD( vecTracerDir.x );
				WRITE_COORD( vecTracerDir.y );
				WRITE_COORD( vecTracerDir.z );
			MESSAGE_END();
		}

		flDamage -= 20;
		if (flDamage <= 0)
			flDamage = 0.1;// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}
	
	CMBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

//=========================================================
// StopTalking - won't speak again for 10-20 seconds.
//=========================================================
void CMAGrunt::StopTalking( void )
{
	m_flNextWordTime = m_flNextSpeakTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);
}

//=========================================================
// ShouldSpeak - Should this agrunt be talking?
//=========================================================
BOOL CMAGrunt::ShouldSpeak( void )
{
	if ( m_flNextSpeakTime > gpGlobals->time )
	{
		// my time to talk is still in the future.
		return FALSE;
	}

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// if gagged, don't talk outside of combat.
			// if not going to talk because of this, put the talk time 
			// into the future a bit, so we don't talk immediately after 
			// going into combat
			m_flNextSpeakTime = gpGlobals->time + 3;
			return FALSE;
		}
	}

	return TRUE;
}

//=========================================================
// PrescheduleThink 
//=========================================================
void CMAGrunt :: PrescheduleThink ( void )
{
	if ( ShouldSpeak() )
	{
		if ( m_flNextWordTime < gpGlobals->time )
		{
			int num = -1;

			do
			{
				num = RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1);
			} while( num == m_iLastWord );

			m_iLastWord = num;

			// play a new sound
			EMIT_SOUND ( ENT(pev), CHAN_VOICE, pIdleSounds[ num ], 1.0, ATTN_NORM );

			// is this word our last?
			if ( RANDOM_LONG( 1, 10 ) <= 1 )
			{
				// stop talking.
				StopTalking();
			}
			else
			{
				m_flNextWordTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 1 );
			}
		}
	}
}

//=========================================================
// DieSound
//=========================================================
void CMAGrunt :: DeathSound ( void )
{
	StopTalking();

	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pDieSounds[RANDOM_LONG(0,ARRAYSIZE(pDieSounds)-1)], 1.0, ATTN_NORM );
}

//=========================================================
// AlertSound
//=========================================================
void CMAGrunt :: AlertSound ( void )
{
	StopTalking();

	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1)], 1.0, ATTN_NORM );
}

//=========================================================
// AttackSound
//=========================================================
void CMAGrunt :: AttackSound ( void )
{
	StopTalking();

	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pAttackSounds[RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1)], 1.0, ATTN_NORM );
}

//=========================================================
// PainSound
//=========================================================
void CMAGrunt :: PainSound ( void )
{
	if ( m_flNextPainTime > gpGlobals->time )
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.6;

	StopTalking();

	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1)], 1.0, ATTN_NORM );
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMAGrunt :: Classify ( void )
{
	if ( m_iClassifyOverride == -1 ) // helper
		return CLASS_NONE;
	else if ( m_iClassifyOverride > 0 )
		return m_iClassifyOverride; // override
	
	return	CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMAGrunt :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 110;
		break;
	default:			ys = 100;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CMAGrunt :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case AGRUNT_AE_HORNET1:
	case AGRUNT_AE_HORNET2:
	case AGRUNT_AE_HORNET3:
	case AGRUNT_AE_HORNET4:
	case AGRUNT_AE_HORNET5:
		{
			// m_vecEnemyLKP should be center of enemy body
			Vector vecArmPos, vecArmDir;
			Vector vecDirToEnemy;
			Vector angDir;

			if (HasConditions( bits_COND_SEE_ENEMY))
			{
				vecDirToEnemy = ( ( m_vecEnemyLKP ) - pev->origin );
				angDir = UTIL_VecToAngles( vecDirToEnemy );
				vecDirToEnemy = vecDirToEnemy.Normalize();
			}
			else
			{
				angDir = pev->angles;
				UTIL_MakeAimVectors( angDir );
				vecDirToEnemy = gpGlobals->v_forward;
			}

			pev->effects = EF_MUZZLEFLASH;

			// make angles +-180
			if (angDir.x > 180)
			{
				angDir.x = angDir.x - 360;
			}

			SetBlending( 0, angDir.x );
			GetAttachment( 0, vecArmPos, vecArmDir );

			vecArmPos = vecArmPos + vecDirToEnemy * 32;
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecArmPos );
				WRITE_BYTE( TE_SPRITE );
				WRITE_COORD( vecArmPos.x );	// pos
				WRITE_COORD( vecArmPos.y );	
				WRITE_COORD( vecArmPos.z );	
				WRITE_SHORT( iAgruntMuzzleFlash );		// model
				WRITE_BYTE( 6 );				// size * 10
				WRITE_BYTE( 128 );			// brightness
			MESSAGE_END();

//jlb			CMBaseEntity *pHornet = CMBaseEntity::Create( "hornet", vecArmPos, UTIL_VecToAngles( vecDirToEnemy ), edict() );
			CMHornet *pHornet = CreateClassPtr((CMHornet *)NULL);

			if (pHornet != NULL)
			{
				pHornet->pev->origin = vecArmPos;
				pHornet->pev->angles = UTIL_VecToAngles( vecDirToEnemy );
				pHornet->pev->owner = edict();

				// Initialize these for entities who don't link to the world
				pHornet->pev->absmin = pHornet->pev->origin - Vector(1,1,1);
				pHornet->pev->absmax = pHornet->pev->origin + Vector(1,1,1);

				pHornet->Spawn();

				UTIL_MakeVectors ( pHornet->pev->angles );
				pHornet->pev->velocity = gpGlobals->v_forward * 300;

				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pFireSounds[RANDOM_LONG(0, ARRAYSIZE(pFireSounds) - 1)], 1.0, ATTN_NORM, 0, 100);

				CMBaseMonster *pHornetMonster = pHornet->MyMonsterPointer();

				if ( pHornetMonster )
				{
					pHornetMonster->m_hEnemy = m_hEnemy;
				}
			}
		}
		break;

	case AGRUNT_AE_LEFT_FOOT:
		switch (RANDOM_LONG(0,1))
		{
		// left foot
		case 0:	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/pl_ladder2.wav", 1, ATTN_NORM, 0, 70 );	break;
		case 1:	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/pl_ladder4.wav", 1, ATTN_NORM, 0, 70 );	break;
		}
		break;
	case AGRUNT_AE_RIGHT_FOOT:
		// right foot
		switch (RANDOM_LONG(0,1))
		{
		case 0:	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/pl_ladder1.wav", 1, ATTN_NORM, 0, 70 );	break;
		case 1:	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/pl_ladder3.wav", 1, ATTN_NORM, 0 ,70);	break;
		}
		break;

	case AGRUNT_AE_LEFT_PUNCH:
		{
			edict_t *pHurt = CheckTraceHullAttack( AGRUNT_MELEE_DIST, gSkillData.agruntDmgPunch, DMG_CLUB );

			if ( pHurt )
			{
				pHurt->v.punchangle.y = -25;
				pHurt->v.punchangle.x = 8;

				// OK to use gpGlobals without calling MakeVectors, cause CheckTraceHullAttack called it above.
				if ( UTIL_IsPlayer(pHurt) )
				{
					// this is a player. Knock him around.
					pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_right * 250;
				}

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				Vector vecArmPos, vecArmAng;
				GetAttachment( 0, vecArmPos, vecArmAng );

				if (UTIL_IsPlayer(pHurt))
					SpawnBlood(vecArmPos, BLOOD_COLOR_RED, 25);
				else if (pHurt->v.euser4 != NULL)
				{
					CMBaseMonster *pMonster = GetClassPtr((CMBaseMonster *)VARS(pHurt));
					SpawnBlood(vecArmPos, pMonster->BloodColor(), 25);// a little surface blood.
				}
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
		}
		break;

	case AGRUNT_AE_RIGHT_PUNCH:
		{
			edict_t *pHurt = CheckTraceHullAttack( AGRUNT_MELEE_DIST, gSkillData.agruntDmgPunch, DMG_CLUB );

			if ( pHurt )
			{
				pHurt->v.punchangle.y = 25;
				pHurt->v.punchangle.x = 8;

				// OK to use gpGlobals without calling MakeVectors, cause CheckTraceHullAttack called it above.
				if ( UTIL_IsPlayer(pHurt) )
				{
					// this is a player. Knock him around.
					pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_right * -250;
				}

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				Vector vecArmPos, vecArmAng;
				GetAttachment( 0, vecArmPos, vecArmAng );

				if (UTIL_IsPlayer(pHurt))
					SpawnBlood(vecArmPos, BLOOD_COLOR_RED, 25);
				else if (pHurt->v.euser4 != NULL)
				{
					CMBaseMonster *pMonster = GetClassPtr((CMBaseMonster *)VARS(pHurt));
					SpawnBlood(vecArmPos, pMonster->BloodColor(), 25);// a little surface blood.
				}
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
		}
		break;

	default:
		CMBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CMAGrunt :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), (!FStringNull( pev->model ) ? STRING( pev->model ) : "models/agrunt.mdl"));
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= !m_bloodColor ? BLOOD_COLOR_YELLOW : m_bloodColor;
	pev->effects		= 0;
	if (!pev->health)	{ pev->health = gSkillData.agruntHealth; }
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= 0;

	m_HackedGunPos		= Vector( 24, 64, 48 );

	m_flNextSpeakTime	= m_flNextWordTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);

	MonsterInit();
	
	pev->classname = MAKE_STRING( "monster_alien_grunt" );
	if ( strlen( STRING( m_szMonsterName ) ) == 0 )
	{
		// default name
		m_szMonsterName = MAKE_STRING( "Alien Grunt" );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMAGrunt :: Precache()
{
	PRECACHE_MODEL("models/agrunt.mdl");

	PRECACHE_SOUND_ARRAY( pAttackHitSounds );
	PRECACHE_SOUND_ARRAY( pAttackHitSounds );
	PRECACHE_SOUND_ARRAY( pAttackMissSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pDieSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );

	iAgruntMuzzleFlash = PRECACHE_MODELINDEX("sprites/muz4.spr");

	CMHornet hornet;
	hornet.Precache();
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Fail Schedule
//=========================================================
Task_t	tlAGruntFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slAGruntFail[] =
{
	{
		tlAGruntFail,
		ARRAYSIZE ( tlAGruntFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"AGrunt Fail"
	},
};

//=========================================================
// Combat Fail Schedule
//=========================================================
Task_t	tlAGruntCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slAGruntCombatFail[] =
{
	{
		tlAGruntCombatFail,
		ARRAYSIZE ( tlAGruntCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"AGrunt Combat Fail"
	},
};

//=========================================================
// Standoff schedule. Used in combat when a monster is 
// hiding in cover or the enemy has moved out of sight. 
// Should we look around in this schedule?
//=========================================================
Task_t	tlAGruntStandoff[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)2					},
};

Schedule_t slAGruntStandoff[] = 
{
	{
		tlAGruntStandoff,
		ARRAYSIZE ( tlAGruntStandoff ),
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_MELEE_ATTACK1		|
		bits_COND_SEE_ENEMY				|
		bits_COND_NEW_ENEMY				|
		bits_COND_HEAR_SOUND,
		0,
		"Agrunt Standoff"
	}
};

//=========================================================
// Suppress
//=========================================================
Task_t	tlAGruntSuppressHornet[] =
{
	{ TASK_STOP_MOVING,		(float)0		},
	{ TASK_RANGE_ATTACK1,	(float)0		},
};

Schedule_t slAGruntSuppress[] =
{
	{
		tlAGruntSuppressHornet,
		ARRAYSIZE ( tlAGruntSuppressHornet ),
		0,
		0,
		"AGrunt Suppress Hornet",
	},
};

//=========================================================
// primary range attacks
//=========================================================
Task_t	tlAGruntRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slAGruntRangeAttack1[] =
{
	{ 
		tlAGruntRangeAttack1,
		ARRAYSIZE ( tlAGruntRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE,
		
		0,
		"AGrunt Range Attack1"
	},
};


Task_t	tlAGruntHiddenRangeAttack1[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_STANDOFF },
	{ TASK_AGRUNT_SETUP_HIDE_ATTACK,	0				},
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_IDEAL,					0				},
	{ TASK_RANGE_ATTACK1_NOTURN,		(float)0		},
};

Schedule_t	slAGruntHiddenRangeAttack[] =
{
	{ 
		tlAGruntHiddenRangeAttack1,
		ARRAYSIZE ( tlAGruntHiddenRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		0,
		"AGrunt Hidden Range Attack1"
	},
};

//=========================================================
// Take cover from enemy! Tries lateral cover before node 
// cover! 
//=========================================================
Task_t	tlAGruntTakeCoverFromEnemy[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slAGruntTakeCoverFromEnemy[] =
{
	{ 
		tlAGruntTakeCoverFromEnemy,
		ARRAYSIZE ( tlAGruntTakeCoverFromEnemy ), 
		bits_COND_NEW_ENEMY,
		0,
		"AGruntTakeCoverFromEnemy"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlAGruntVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_AGRUNT_THREAT_DISPLAY	},
	{ TASK_WAIT,							(float)0.2					},
	{ TASK_AGRUNT_GET_PATH_TO_ENEMY_CORPSE,	(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_CROUCH			},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_STAND			},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_THREAT_DISPLAY	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_CROUCH			},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_STAND			},
};

Schedule_t	slAGruntVictoryDance[] =
{
	{ 
		tlAGruntVictoryDance,
		ARRAYSIZE ( tlAGruntVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"AGruntVictoryDance"
	},
};

//=========================================================
//=========================================================
Task_t	tlAGruntThreatDisplay[] =
{
	{ TASK_STOP_MOVING,			(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_THREAT_DISPLAY	},
};

Schedule_t	slAGruntThreatDisplay[] =
{
	{ 
		tlAGruntThreatDisplay,
		ARRAYSIZE ( tlAGruntThreatDisplay ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"AGruntThreatDisplay"
	},
};

DEFINE_CUSTOM_SCHEDULES( CMAGrunt )
{
	slAGruntFail,
	slAGruntCombatFail,
	slAGruntStandoff,
	slAGruntSuppress,
	slAGruntRangeAttack1,
	slAGruntHiddenRangeAttack,
	slAGruntTakeCoverFromEnemy,
	slAGruntVictoryDance,
	slAGruntThreatDisplay,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMAGrunt, CMBaseMonster );

//=========================================================
// FCanCheckAttacks - this is overridden for alien grunts
// because they can use their smart weapons against unseen
// enemies. Base class doesn't attack anyone it can't see.
//=========================================================
BOOL CMAGrunt :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// CheckMeleeAttack1 - alien grunts zap the crap out of 
// any enemy that gets too close. 
//=========================================================
BOOL CMAGrunt :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( HasConditions ( bits_COND_SEE_ENEMY ) && flDist <= AGRUNT_MELEE_DIST && flDot >= 0.6 && m_hEnemy != NULL )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 
//
// !!!LATER - we may want to load balance this. Several
// tracelines are done, so we may not want to do this every
// server frame. Definitely not while firing. 
//=========================================================
BOOL CMAGrunt :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( gpGlobals->time < m_flNextHornetAttackCheck )
	{
		return m_fCanHornetAttack;
	}

	if ( HasConditions( bits_COND_SEE_ENEMY ) && flDist >= AGRUNT_MELEE_DIST && flDist <= 1024 && flDot >= 0.5 )
	{
		TraceResult	tr;
		Vector	vecArmPos, vecArmDir;

		// verify that a shot fired from the gun will hit the enemy before the world.
		// !!!LATER - we may wish to do something different for projectile weapons as opposed to instant-hit
		UTIL_MakeVectors( pev->angles );
		GetAttachment( 0, vecArmPos, vecArmDir );
//		UTIL_TraceLine( vecArmPos, vecArmPos + gpGlobals->v_forward * 256, ignore_monsters, ENT(pev), &tr);
		UTIL_TraceLine( vecArmPos, UTIL_BodyTarget(m_hEnemy, vecArmPos), dont_ignore_monsters, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 || (tr.pHit == m_hEnemy) )
		{
			m_flNextHornetAttackCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );
			m_fCanHornetAttack = TRUE;
			return m_fCanHornetAttack;
		}
	}
	
	m_flNextHornetAttackCheck = gpGlobals->time + 0.2;// don't check for half second if this check wasn't successful
	m_fCanHornetAttack = FALSE;
	return m_fCanHornetAttack;
}

//=========================================================
// StartTask
//=========================================================
void CMAGrunt :: StartTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_AGRUNT_GET_PATH_TO_ENEMY_CORPSE:
		{
			UTIL_MakeVectors( pev->angles );
			if ( BuildRoute ( m_vecEnemyLKP - gpGlobals->v_forward * 50, bits_MF_TO_LOCATION, NULL ) )
			{
				TaskComplete();
			}
			else
			{
				ALERT ( at_aiconsole, "AGruntGetPathToEnemyCorpse failed!!\n" );
				TaskFail();
			}
		}
		break;

	case TASK_AGRUNT_SETUP_HIDE_ATTACK:
		// alien grunt shoots hornets back out into the open from a concealed location. 
		// try to find a spot to throw that gives the smart weapon a good chance of finding the enemy.
		// ideally, this spot is along a line that is perpendicular to a line drawn from the agrunt to the enemy.

		{
			Vector		vecCenter;
			TraceResult	tr;
			BOOL		fSkip;

			fSkip = FALSE;
			vecCenter = Center();

			UTIL_VecToAngles( m_vecEnemyLKP - pev->origin );

			UTIL_TraceLine( Center() + gpGlobals->v_forward * 128, m_vecEnemyLKP, ignore_monsters, ENT(pev), &tr);
			if ( tr.flFraction == 1.0 )
			{
				MakeIdealYaw ( pev->origin + gpGlobals->v_right * 128 );
				fSkip = TRUE;
				TaskComplete();
			}
			
			if ( !fSkip )
			{
				UTIL_TraceLine( Center() - gpGlobals->v_forward * 128, m_vecEnemyLKP, ignore_monsters, ENT(pev), &tr);
				if ( tr.flFraction == 1.0 )
				{
					MakeIdealYaw ( pev->origin - gpGlobals->v_right * 128 );
					fSkip = TRUE;
					TaskComplete();
				}
			}
			
			if ( !fSkip )
			{
				UTIL_TraceLine( Center() + gpGlobals->v_forward * 256, m_vecEnemyLKP, ignore_monsters, ENT(pev), &tr);
				if ( tr.flFraction == 1.0 )
				{
					MakeIdealYaw ( pev->origin + gpGlobals->v_right * 256 );
					fSkip = TRUE;
					TaskComplete();
				}
			}
			
			if ( !fSkip )
			{
				UTIL_TraceLine( Center() - gpGlobals->v_forward * 256, m_vecEnemyLKP, ignore_monsters, ENT(pev), &tr);
				if ( tr.flFraction == 1.0 )
				{
					MakeIdealYaw ( pev->origin - gpGlobals->v_right * 256 );
					fSkip = TRUE;
					TaskComplete();
				}
			}
			
			if ( !fSkip )
			{
				TaskFail();
			}
		}
		break;

	default:
		CMBaseMonster :: StartTask ( pTask );
		break;
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CMAGrunt :: GetSchedule ( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CMBaseMonster :: GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				return GetScheduleOfType( SCHED_WAKE_ANGRY );
			}

	// zap player!
			if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				AttackSound();// this is a total hack. Should be part of the schedule
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

			if ( HasConditions ( bits_COND_HEAVY_DAMAGE ) )
			{
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}

	// can attack
			if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_CHASE_ENEMY );
		}
	}

	return CMBaseMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CMAGrunt :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return &slAGruntTakeCoverFromEnemy[ 0 ];
		break;
	
	case SCHED_RANGE_ATTACK1:
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
		{
			//normal attack
			return &slAGruntRangeAttack1[ 0 ];
		}
		else
		{
			// attack an unseen enemy
			// return &slAGruntHiddenRangeAttack[ 0 ];
			return &slAGruntRangeAttack1[ 0 ];
		}
		break;

	case SCHED_AGRUNT_THREAT_DISPLAY:
		return &slAGruntThreatDisplay[ 0 ];
		break;

	case SCHED_AGRUNT_SUPPRESS:
		return &slAGruntSuppress[ 0 ];
		break;

	case SCHED_STANDOFF:
		return &slAGruntStandoff[ 0 ];
		break;

	case SCHED_VICTORY_DANCE:
		return &slAGruntVictoryDance[ 0 ];
		break;

	case SCHED_FAIL:
		// no fail schedule specified, so pick a good generic one.
		{
			if ( m_hEnemy != NULL )
			{
				// I have an enemy
				// !!!LATER - what if this enemy is really far away and i'm chasing him?
				// this schedule will make me stop, face his last known position for 2 
				// seconds, and then try to move again
				return &slAGruntCombatFail[ 0 ];
			}

			return &slAGruntFail[ 0 ];
		}
		break;

	}

	return CMBaseMonster :: GetScheduleOfType( Type );
}

