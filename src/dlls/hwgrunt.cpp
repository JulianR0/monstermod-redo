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
// Heavy Weapons Grunt
//=========================================================

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cmbase.h"
#include	"cmbasemonster.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"cmtalkmonster.h"
#include	"effects.h"
#include	"customentity.h"

//=========================================================
// monster-specific DEFINE's
//=========================================================
// Weapon flags
#define HWGRUNT_MINIGUN				0

#define GUN_GROUP					1

// Gun values
#define GUN_MINIGUN					0

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_HWGRUNT_ESTABLISH_LINE_OF_FIRE = LAST_COMMON_SCHEDULE + 1,// move to a location to set up an attack against the enemy.
	SCHED_HWGRUNT_REPEL,
	SCHED_HWGRUNT_REPEL_LAND,
	SCHED_HWGRUNT_WAIT_FACE_ENEMY,
	SCHED_HWGRUNT_TAKECOVER_FAILED,// force analysis of conditions and pick the best possible schedule to recover from failure.
	SCHED_HWGRUNT_ELOF_FAIL,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_MEMORY_HWGRUNT_SPINUP	( bits_MEMORY_CUSTOM1 )

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HWGRUNT_AE_DEATH		( 11 )
#define		HWGRUNT_AE_MINIGUN		( 5001 )

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMHWGrunt::Classify(void)
{
	if ( m_iClassifyOverride == -1 ) // helper
		return CLASS_NONE;
	else if ( m_iClassifyOverride > 0 )
		return m_iClassifyOverride; // override
	
	return CLASS_HUMAN_MILITARY;
}


//=========================================================
// CheckRangeAttack1 - HWGrunt doesn't care about melee
//=========================================================
BOOL CMHWGrunt :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 )
	{
		TraceResult	tr;

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, UTIL_BodyTarget(m_hEnemy, vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - HWGrunt does not kick
//=========================================================
BOOL CMHWGrunt :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - HWGrunt has no grenades
//=========================================================
BOOL CMHWGrunt :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}

//=========================================================
// Shoot
//=========================================================
void CMHWGrunt::Minigun(void)
{
	if (m_hEnemy == 0)
	{
		return;
	}
	
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
	
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_12MM); // shoot +-5 degrees
	
	pev->effects |= EF_MUZZLEFLASH;
	
	// Minigunners have infinite ammo
	//m_cAmmoLoaded--;// take away a bullet!
	
	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// TraceAttack - hwgrunts do not wear helmets
//=========================================================
void CMHWGrunt :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CMBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for hwgrunts.
// They are meant to be aggresive, never take cover.
//=========================================================
int CMHWGrunt :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CMBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMHWGrunt::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case HWGRUNT_AE_DEATH:
		break; // don't get rid of gun

	case HWGRUNT_AE_MINIGUN:
	{
		// Sven Co-op uses a modified hassault/hw_gun4.wav for it's fire sound
		Minigun();
		
		// We don't want looping WAVs. Pick a different sound and change pitch on it
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5));
	}
	break;
	
	default:
		CMHGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CMHWGrunt::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), (!FStringNull( pev->model ) ? STRING( pev->model ) : "models/hwgrunt.mdl"));
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = !m_bloodColor ? BLOOD_COLOR_RED : m_bloodColor;
	pev->effects = 0;
	pev->health = gSkillData.hwgruntHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	//m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	//m_flMinigunSpinTime = 0; // be able to spin up/down minigun right away
	m_iSentence = -1;
	m_fStanding = TRUE;

	//m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_afCapability = bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	
	//m_fEnemyEluded = FALSE;
	m_fFirstEncounter = FALSE;// false because hwgrunt does not send signals of any kind

	m_HackedGunPos = Vector(0, 0, 55);
	
	// Don't setup pev->weapons, always minigun if not specified
	/*
	if (FBitSet(pev->weapons, HWGRUNT_MINIGUN))
	{
		SetBodygroup(GUN_GROUP, GUN_MINIGUN);
		m_cClipSize = 1;
	}
	*/
	m_cAmmoLoaded = 99;
	m_cClipSize = 99;

	CMTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
	
	pev->classname = MAKE_STRING( "monster_hwgrunt" );
	if ( strlen( STRING( m_szMonsterName ) ) == 0 )
	{
		// default name
		m_szMonsterName = MAKE_STRING( "Heavy Weapons Grunt" );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMHWGrunt::Precache()
{
	PRECACHE_MODEL("models/hwgrunt.mdl");
	
	PRECACHE_SOUND("hassault/hw_shoot1.wav");
	PRECACHE_SOUND("hassault/hw_spinup.wav");
	PRECACHE_SOUND("hassault/hw_spindown.wav");

	// get voice pitch
	if (RANDOM_LONG(0, 1))
		m_voicePitch = 102 + RANDOM_LONG(0, 7);
	else
		m_voicePitch = 93; // slight voice change for hwgrunt

	CMHGrunt hgrunt;
	hgrunt.Precache();
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Fail
//=========================================================
Task_t	tlHWGruntFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slHWGruntFail[] =
{
	{
		tlHWGruntFail,
		ARRAYSIZE ( tlHWGruntFail ),
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"HWGrunt Fail"
	},
};

//=========================================================
// Combat Fail
//=========================================================
Task_t	tlHWGruntCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slHWGruntCombatFail[] =
{
	{
		tlHWGruntCombatFail,
		ARRAYSIZE ( tlHWGruntCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"HWGrunt Combat Fail"
	},
};

//=========================================================
// Not really victory dance
//=========================================================
Task_t	tlHWGruntVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
//	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slHWGruntVictoryDance[] =
{
	{ 
		tlHWGruntVictoryDance,
		ARRAYSIZE ( tlHWGruntVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"HWGrunt Victory Dance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlHWGruntEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_HWGRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
//	{ TASK_GRUNT_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slHWGruntEstablishLineOfFire[] =
{
	{ 
		tlHWGruntEstablishLineOfFire,
		ARRAYSIZE ( tlHWGruntEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_HEAR_SOUND,
		0,
		"HWGrunt Establish Line Of Fire"
	},
};

//=========================================================
// wait in cover - we don't allow danger or the ability to
// attack to break a grunt's run to cover schedule, but when
// a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlHWGruntWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slHWGruntWaitInCover[] =
{
	{ 
		tlHWGruntWaitInCover,
		ARRAYSIZE ( tlHWGruntWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"HWGrunt Wait In Cover"
	},
};

//=========================================================
// run to cover.
//=========================================================
Task_t	tlHWGruntTakeCover[] =
{
	{ TASK_STOP_MOVING,				(float)0								},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_HWGRUNT_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2								},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0								},
//	{ TASK_GRUNT_SPEAK_SENTENCE,	(float)0								},
	{ TASK_RUN_PATH,				(float)0								},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0								},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER				},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HWGRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slHWGruntTakeCover[] =
{
	{ 
		tlHWGruntTakeCover,
		ARRAYSIZE ( tlHWGruntTakeCover ), 
		0,
		0,
		"HWGrunt Take Cover"
	},
};

//=========================================================
// minigun spinup
//=========================================================
Task_t	tlHWGruntMinigunSpinUp[] =
{
	{ TASK_STOP_MOVING,					(float)0							},
	{ TASK_SET_ACTIVITY,				(float)ACT_THREAT_DISPLAY			},
	{ TASK_WAIT_FACE_ENEMY,				(float)1							},
	{ TASK_REMEMBER,					(float)bits_MEMORY_HWGRUNT_SPINUP	},
};

Schedule_t	slHWGruntMinigunSpinUp[] =
{
	{ 
		tlHWGruntMinigunSpinUp,
		ARRAYSIZE ( tlHWGruntMinigunSpinUp ), 
		0, // nothing should interrupt this
		0,
		"HWGrunt Minigun Spin Up"
	},
};

//=========================================================
// minigun attack
//=========================================================
Task_t	tlHWGruntMinigunAttack[] =
{
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_RANGE_ATTACK1	},
	{ TASK_RANGE_ATTACK1,				(float)0					},
};

Schedule_t	slHWGruntMinigunAttack[] =
{
	{ 
		tlHWGruntMinigunAttack,
		ARRAYSIZE ( tlHWGruntMinigunAttack ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND,
		0,
		"HWGrunt Minigun Attack"
	},
};

//=========================================================
// repel 
//=========================================================
Task_t	tlHWGruntRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_FACE_IDEAL,			(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slHWGruntRepel[] =
{
	{ 
		tlHWGruntRepel,
		ARRAYSIZE ( tlHWGruntRepel ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		0,
		"HWGrunt Repel"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlHWGruntRepelLand[] =
{
	{ TASK_STOP_MOVING,						(float)0		},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,		(float)0		},
	{ TASK_RUN_PATH,						(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0		},
	{ TASK_CLEAR_LASTPOSITION,				(float)0		},
};

Schedule_t	slHWGruntRepelLand[] =
{
	{ 
		tlHWGruntRepelLand,
		ARRAYSIZE ( tlHWGruntRepelLand ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		0,
		"HWGrunt Repel Land"
	},
};

//=========================================================
// Chase enemy failure
//=========================================================
Task_t	tlHWGruntChaseEnemyFailed[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
//	{ TASK_TURN_LEFT,				(float)179					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1					},
};

Schedule_t	slHWGruntChaseEnemyFailed[] =
{
	{ 
		tlHWGruntChaseEnemyFailed,
		ARRAYSIZE ( tlHWGruntChaseEnemyFailed ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_HEAR_SOUND,
		0,
		"HWGrunt Chase Enemy Failed"
	},
};


DEFINE_CUSTOM_SCHEDULES( CMHWGrunt )
{
	slHWGruntFail,
	slHWGruntCombatFail,
	slHWGruntVictoryDance,
	slHWGruntEstablishLineOfFire,
	slHWGruntWaitInCover,
	slHWGruntTakeCover,
	slHWGruntMinigunSpinUp,
	slHWGruntMinigunAttack,
	slHWGruntRepel,
	slHWGruntRepelLand,
	slHWGruntChaseEnemyFailed,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMHWGrunt, CMBaseMonster );

//=========================================================
// SetActivity 
//=========================================================
void CMHWGrunt :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity )
	{
	case ACT_RANGE_ATTACK1:
		iSequence = LookupSequence( "attack" );
		break;
	case ACT_RUN:
		iSequence = LookupSequence( "run" );
		break;
	case ACT_WALK:
		iSequence = LookupSequence( "creeping_walk" );
		break;
	default:
		iSequence = LookupActivity( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CMHWGrunt :: GetSchedule( void )
{
	// clear old sentence
	m_iSentence = -1; // we don't care about sounds for now.

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_HWGRUNT_REPEL_LAND );
		}
		else
		{
			// can not attack while holding a minigun in rapel
			return GetScheduleOfType ( SCHED_HWGRUNT_REPEL );
		}
	}

	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// was attacking, spin down
				if ( HasMemory( bits_MEMORY_HWGRUNT_SPINUP ) )
				{
					Forget( bits_MEMORY_HWGRUNT_SPINUP );
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "hassault/hw_spindown.wav", 0.8, ATTN_NORM);
				}

				// call base class, all code to handle dead enemies is centralized there.
				return CMBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				// none of this should take place as CSquadMonster functions were completely stripped. -Giegue
				/*
				{
					{
						//!!!KELLY - the leader of a squad of grunts has just seen the player or a 
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight" 
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if ((m_hEnemy != NULL) && UTIL_IsPlayer(m_hEnemy))
								// player
								SENTENCEG_PlayRndSz( ENT(pev), "HG_ALERT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);

							else if ((m_hEnemy != NULL) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) && 
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) && 
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "HG_MONST", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);

							JustSpoke();
						}
						
						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
				*/
			}

// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// we don't want the monster to take cover when hurt while attacking, clear this
				ClearConditions( bits_COND_LIGHT_DAMAGE );
			}
// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				// can fire? shoot. destroy without a care
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				// do sound
				if ( HasMemory( bits_MEMORY_HWGRUNT_SPINUP ) )
				{
					Forget( bits_MEMORY_HWGRUNT_SPINUP );
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "hassault/hw_spindown.wav", 0.8, ATTN_NORM);
				}

				// then go kamikaze and chase the enemy
				return GetScheduleOfType( SCHED_HWGRUNT_ESTABLISH_LINE_OF_FIRE );
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_HWGRUNT_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CMBaseMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CMHWGrunt :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slHWGruntTakeCover[ 0 ];
		}
	case SCHED_HWGRUNT_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_HWGRUNT_ELOF_FAIL:
		{
			// unable to move to a position that allows attacking the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_HWGRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			return &slHWGruntEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// minigun should spin up first
			if ( !HasMemory( bits_MEMORY_HWGRUNT_SPINUP ) )
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hassault/hw_spinup.wav", 0.8, ATTN_NORM);
				return &slHWGruntMinigunSpinUp[ 0 ];
			}
			else
				return &slHWGruntMinigunAttack[ 0 ];
		}
	case SCHED_HWGRUNT_WAIT_FACE_ENEMY:
		{
			return &slHWGruntWaitInCover[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slHWGruntVictoryDance[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slHWGruntCombatFail[ 0 ];
			}

			return &slHWGruntFail[ 0 ];
		}
	case SCHED_HWGRUNT_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slHWGruntRepel[ 0 ];
		}
	case SCHED_HWGRUNT_REPEL_LAND:
		{
			return &slHWGruntRepelLand[ 0 ];
		}
	case SCHED_CHASE_ENEMY_FAILED:
		{
			// add missing schedule from squadmonster.cpp
			return &slHWGruntChaseEnemyFailed[ 0 ];
		}
	default:
		{
			return CMBaseMonster :: GetScheduleOfType ( Type );
		}
	}
}

