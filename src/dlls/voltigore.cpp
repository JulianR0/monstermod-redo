// HUGE thanks to DrBeef for his hlsdk-xash3d-opfor repository!

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
// voltigore
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cmbase.h"
#include	"cmbasemonster.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"weapons.h"

#define		VOLTIGORE_SPRINT_DIST	256 // how close the voltigore has to get before starting to sprint and refusing to swerve

#define		VOLTIGORE_MAX_BEAMS		8

#define VOLTIGORE_ZAP_RED 180
#define VOLTIGORE_ZAP_GREEN 16
#define VOLTIGORE_ZAP_BLUE 255
#define VOLTIGORE_ZAP_BEAM "sprites/lgtning.spr"
#define VOLTIGORE_ZAP_NOISE 80
#define VOLTIGORE_ZAP_WIDTH 30
#define VOLTIGORE_ZAP_BRIGHTNESS 255
#define VOLTIGORE_ZAP_DISTANCE 512
#define VOLTIGORE_GLOW_SCALE 0.75f
#define VOLTIGORE_GIB_COUNT 9
#define VOLTIGORE_GLOW_SPRITE "sprites/blueflare2.spr"

//=========================================================
// monster-specific schedule types
//=========================================================


//=========================================================
// monster-specific tasks
//=========================================================

enum {
	TASK_VOLTIGORE_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1
};


//=========================================================
// Purpose:
//=========================================================
void CMVoltigoreEnergyBall::Spawn(void)
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("voltigore_energy_ball");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), VOLTIGORE_GLOW_SPRITE);
	pev->frame = 0;
	pev->scale = VOLTIGORE_GLOW_SCALE;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	m_iBeams = 0;
}

//=========================================================
// Purpose:
//=========================================================
edict_t *CMVoltigoreEnergyBall::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CMVoltigoreEnergyBall *pEnergyBall = CreateClassPtr((CMVoltigoreEnergyBall *)NULL);
	if (pEnergyBall == NULL)
		return NULL;
	
	pEnergyBall->Spawn();
	
	UTIL_SetOrigin(pEnergyBall->pev, vecStart);
	pEnergyBall->pev->velocity = vecVelocity;
	pEnergyBall->pev->owner = ENT(pevOwner);
	
	pEnergyBall->SetTouch(&CMVoltigoreEnergyBall::BallTouch);
	pEnergyBall->SetThink(&CMVoltigoreEnergyBall::FlyThink);
	pEnergyBall->pev->nextthink = gpGlobals->time + 0.1;
	
	return pEnergyBall->edict();
}

//=========================================================
// Purpose:
//=========================================================
void CMVoltigoreEnergyBall::BallTouch(edict_t *pOther)
{
	if (m_timeToDie) {
		return;
	}
	TraceResult tr;
	if (!pOther->v.takedamage)
	{

		// make a splat on the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
		UTIL_DecalTrace(&tr, DECAL_SCORCH1 + RANDOM_LONG(0, 1));
	}
	else
	{
		if ( UTIL_IsPlayer( pOther ) )
			UTIL_TakeDamage( pOther, pev, VARS( pev->owner ), gSkillData.voltigoreDmgBeam, DMG_SHOCK|DMG_ALWAYSGIB );
		else if (pOther->v.euser4 != NULL)
		{
			CMBaseMonster *pMonster = GetClassPtr((CMBaseMonster *)VARS(pOther));
			pMonster->TakeDamage( pev, VARS( pev->owner ), gSkillData.voltigoreDmgBeam, DMG_SHOCK|DMG_ALWAYSGIB );
		}
	}
	pev->velocity = Vector(0,0,0);

	m_timeToDie = gpGlobals->time + 0.3;
	SetTouch( NULL );
}

//=========================================================
// Purpose:
//=========================================================
void CMVoltigoreEnergyBall::FlyThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
	if (m_timeToDie)
	{
		edict_t *pEntity = NULL;
		while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 32)) != NULL)
		{
			if (pEntity->v.takedamage && !FClassnameIs(pEntity, "monster_alien_voltigore") && !FClassnameIs(pEntity, "monster_alien_babyvoltigore"))
			{
				if ( UTIL_IsPlayer( pEntity ) )
					UTIL_TakeDamage( pEntity, pev, pev, gSkillData.voltigoreDmgBeam/5, DMG_SHOCK );
				else if (pEntity->v.euser4 != NULL)
				{
					CMBaseMonster *pMonster = GetClassPtr((CMBaseMonster *)VARS(pEntity));
					pMonster->TakeDamage( pev, pev, gSkillData.voltigoreDmgBeam/5, DMG_SHOCK );
				}
			}
		}
		
		if (m_timeToDie <= gpGlobals->time)
		{
			ClearBeams();
			SetThink(&CMVoltigoreEnergyBall::SUB_Remove);
			pev->nextthink = gpGlobals->time;
		}
	}
	else
	{
		if (m_iBeams)
			UpdateBeams();
		else
			CreateBeams();
	}
}

//=========================================================
// Purpose:
//=========================================================
void CMVoltigoreEnergyBall::CreateBeam(int nIndex, const Vector& vecPos, int width, int brightness)
{
	m_pBeam[nIndex] = CMBeam::BeamCreate(VOLTIGORE_ZAP_BEAM, width);
	if (!m_pBeam[nIndex])
		return;

	m_pBeam[nIndex]->PointEntInit(vecPos, entindex());
	m_pBeam[nIndex]->SetColor(VOLTIGORE_ZAP_RED, VOLTIGORE_ZAP_GREEN, VOLTIGORE_ZAP_BLUE);
	m_pBeam[nIndex]->SetBrightness(brightness);
	m_pBeam[nIndex]->SetNoise(VOLTIGORE_ZAP_NOISE);
	//m_pBeam[nIndex]->SetFlags( SF_BEAM_SHADEIN );
}

//=========================================================
// Purpose:
//=========================================================
void CMVoltigoreEnergyBall::UpdateBeam(int nIndex, const Vector& vecPos, bool show)
{
	if (!m_pBeam[nIndex])
		return;
	m_pBeam[nIndex]->SetBrightness(show ? VOLTIGORE_ZAP_BRIGHTNESS : 0);
	m_pBeam[nIndex]->SetStartPos(vecPos);
	m_pBeam[nIndex]->SetEndEntity(entindex());
	m_pBeam[nIndex]->RelinkBeam();
}

//=========================================================
// Purpose:
//=========================================================
void CMVoltigoreEnergyBall::ClearBeam(int nIndex)
{
	if (m_pBeam[nIndex])
	{
		UTIL_Remove(m_pBeam[nIndex]->edict());
		m_pBeam[nIndex] = NULL;
	}
}

//=========================================================
// CreateBeams - create all beams
//=========================================================
void CMVoltigoreEnergyBall::CreateBeams()
{
	for (int i = 0; i < VOLTIGORE_MAX_BEAMS; ++i)
	{
		CreateBeam(i, pev->origin, VOLTIGORE_ZAP_WIDTH, VOLTIGORE_ZAP_BRIGHTNESS );
	}
	m_iBeams = VOLTIGORE_MAX_BEAMS;
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CMVoltigoreEnergyBall::ClearBeams()
{
	for (int i = 0; i < VOLTIGORE_MAX_BEAMS; ++i)
	{
		ClearBeam( i );
	}
	m_iBeams = 0;
}


void CMVoltigoreEnergyBall::UpdateBeams()
{
	int i, j;

	TraceResult tr;
	const Vector vecSrc = pev->origin;
	const Vector directionVector = pev->velocity.Normalize();
	const int baseDistance = VOLTIGORE_ZAP_DISTANCE;
	for (i = 0; i < m_iBeams; ++i)
	{
		for (j = 0; j < 3; ++j)
		{
			const float randomX = RANDOM_FLOAT(-1, 0.1);
			const float randomY = RANDOM_FLOAT(-1, 0.1);
			//ALERT(at_console, "Randomize: %f %f\n", randomX, randomY);
			Vector vecTarget = vecSrc + Vector(
						directionVector.x * randomX,
						directionVector.y * randomY,
						RANDOM_LONG(0, 1) ? 1 : -1
					) * baseDistance;
			TraceResult tr1;
			UTIL_TraceLine(vecSrc, vecTarget, ignore_monsters, ENT(pev), &tr1);
			if (tr1.flFraction != 1.0f) {
				tr = tr1;
				break;
			}
		}

		// Update the target position of the beam.
		UpdateBeam(i, tr.vecEndPos, tr.flFraction != 1.0f);
	}
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		VOLTIGORE_AE_THROW			( 1 )
#define		VOLTIGORE_AE_PUNCH_BOTH		( 12 )
#define		VOLTIGORE_AE_PUNCH_SINGLE	( 13 )
#define		VOLTIGORE_AE_GIB			( 2002 )

const char* CMVoltigore::pAlertSounds[] =
{
	"voltigore/voltigore_alert1.wav",
	"voltigore/voltigore_alert2.wav",
	"voltigore/voltigore_alert3.wav",
};

const char* CMVoltigore::pAttackMeleeSounds[] =
{
	"voltigore/voltigore_attack_melee1.wav",
	"voltigore/voltigore_attack_melee2.wav",
};

const char* CMVoltigore::pMeleeHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char* CMVoltigore::pMeleeMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* CMVoltigore::pComSounds[] =
{
	"voltigore/voltigore_communicate1.wav",
	"voltigore/voltigore_communicate2.wav",
	"voltigore/voltigore_communicate3.wav",
};


const char* CMVoltigore::pDeathSounds[] =
{
	"voltigore/voltigore_die1.wav",
	"voltigore/voltigore_die2.wav",
	"voltigore/voltigore_die3.wav",
};

const char* CMVoltigore::pFootstepSounds[] =
{
	"voltigore/voltigore_footstep1.wav",
	"voltigore/voltigore_footstep2.wav",
	"voltigore/voltigore_footstep3.wav",
};

const char* CMVoltigore::pIdleSounds[] =
{
	"voltigore/voltigore_idle1.wav",
	"voltigore/voltigore_idle2.wav",
	"voltigore/voltigore_idle3.wav",
};

const char* CMVoltigore::pPainSounds[] =
{
	"voltigore/voltigore_pain1.wav",
	"voltigore/voltigore_pain2.wav",
	"voltigore/voltigore_pain3.wav",
	"voltigore/voltigore_pain4.wav",
};

const char* CMVoltigore::pGruntSounds[] =
{
	"voltigore/voltigore_run_grunt1.wav",
	"voltigore/voltigore_run_grunt2.wav",
};

//=========================================================
// TakeDamage - overridden for voltigore so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CMVoltigore::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	float flDist;
	Vector vecApex;

	// if the voltigore is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if (m_hEnemy != 0 && IsMoving() && pevAttacker == VARS(m_hEnemy))
	{
		flDist = (pev->origin - m_hEnemy->v.origin).Length2D();

		if (flDist > VOLTIGORE_SPRINT_DIST)
		{
			flDist = (pev->origin - m_Route[m_iRouteIndex].vecLocation).Length2D();// reusing flDist. 

			if (FTriangulate(pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist * 0.5, m_hEnemy, &vecApex))
			{
				InsertWaypoint(vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY);
			}
		}
	}
	
	// Ain't something missing here? -Giegue
	
	return CMBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CMVoltigore::CheckRangeAttack1(float flDot, float flDist)
{
	if (IsMoving() && flDist >= 512)
	{
		// voltigore will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if (flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextZapTime)
	{
		if (m_hEnemy != 0)
		{
			if (fabs(pev->origin.z - m_hEnemy->v.origin.z) > 256)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if (IsMoving())
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextZapTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextZapTime = gpGlobals->time + 2;
		}

		return TRUE;
	}

	return FALSE;
}

//=========================================================
//=========================================================
void CMVoltigore::RunAI(void)
{
	CMBaseMonster::RunAI();

	if (m_fShouldUpdateBeam)
	{
		UpdateBeams();
	}

	GlowUpdate();
}

void CMVoltigore::GibMonster()
{
	GibBeamDamage();
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM );
	if( CVAR_GET_FLOAT( "violence_agibs" ) != 0 )	// Should never get here, but someone might call it directly
	{
		CMGib::SpawnRandomGibs( pev, VOLTIGORE_GIB_COUNT, "models/vgibs.mdl", 0 );	// Throw alien gibs
	}
	SetThink( &CMBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// CheckMeleeAttack1 - voltigore is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CMVoltigore::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 120 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMVoltigore::Classify(void)
{
	if ( m_iClassifyOverride == -1 ) // helper
		return CLASS_NONE;
	else if ( m_iClassifyOverride > 0 )
		return m_iClassifyOverride; // override
	
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// IdleSound 
//=========================================================
void CMVoltigore::IdleSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1, ATTN_NORM);
}

//=========================================================
// PainSound 
//=========================================================
void CMVoltigore::PainSound(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM, 0, RANDOM_LONG(85, 120));
}

//=========================================================
// AlertSound
//=========================================================
void CMVoltigore::AlertSound(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1, ATTN_NORM, 0, RANDOM_LONG(140, 160));
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMVoltigore::SetYawSpeed(void)
{
	int ys;

	ys = 0;

	switch (m_Activity)
	{
	case	ACT_WALK:			ys = 90;	break;
	case	ACT_RUN:			ys = 90;	break;
	case	ACT_IDLE:			ys = 90;	break;
	case	ACT_RANGE_ATTACK1:	ys = 90;	break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMVoltigore::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case VOLTIGORE_AE_THROW:
	{
		// SOUND HERE!
		Vector	vecSpitDir;

		UTIL_MakeVectors(pev->angles);

		Vector vecSpitOrigin, vecAngles;
		GetAttachment(3, vecSpitOrigin, vecAngles);
		vecSpitDir = ShootAtEnemy(vecSpitOrigin);

		// do stuff for this event.
		//AttackSound();

		CMVoltigoreEnergyBall::Shoot(pev, vecSpitOrigin, vecSpitDir * 1000);

		// turn the beam glow off.
		DestroyBeams();

		GlowOff();

		m_fShouldUpdateBeam = FALSE;
	}
	break;


	case VOLTIGORE_AE_PUNCH_SINGLE:
	{
		// SOUND HERE!
		edict_t *pHurt = CheckTraceHullAttack(120, gSkillData.voltigoreDmgPunch, DMG_CLUB);
		if (pHurt)
		{
			if (FBitSet(pHurt->v.flags, FL_MONSTER|FL_CLIENT))
			{
				pHurt->v.punchangle.z = -15;
				pHurt->v.punchangle.x = 15;
				pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_right * -150;
				pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_up * 100;
			}

			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeHitSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

			Vector vecArmPos, vecArmAng;
			GetAttachment( 0, vecArmPos, vecArmAng );
			SpawnBlood( vecArmPos, BloodColor(), 25 );// a little surface blood.
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeMissSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
	}
	break;

	case VOLTIGORE_AE_PUNCH_BOTH:
	{
		// SOUND HERE!
		edict_t *pHurt = CheckTraceHullAttack(120, gSkillData.voltigoreDmgPunch, DMG_CLUB);
		if (pHurt)
		{
			if (FBitSet(pHurt->v.flags, FL_MONSTER|FL_CLIENT))
			{
				pHurt->v.punchangle.x = 20;
				pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_forward * 150;
				pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_up * 100;
			}

			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeHitSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

			Vector vecArmPos, vecArmAng;
			GetAttachment( 0, vecArmPos, vecArmAng );
			SpawnBlood( vecArmPos, BloodColor(), 25 );// a little surface blood.
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeMissSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
	}
	break;

	case VOLTIGORE_AE_GIB:
	{
		pev->health = 0;
		GibMonster();
	}
	break;

	default:
		CMBaseMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CMVoltigore::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), (!FStringNull( pev->model ) ? STRING( pev->model ) : "models/voltigore.mdl"));
	UTIL_SetSize(pev, Vector(-80, -80, 0), Vector(80, 80, 90));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.voltigoreHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_flNextZapTime	= gpGlobals->time;


	m_fShouldUpdateBeam = FALSE;
	m_pBeamGlow = NULL;

	GlowOff();

	// Create glow.
	CreateGlow();

	MonsterInit();
	pev->view_ofs		= Vector(0, 0, 84);
	
	pev->classname = MAKE_STRING("monster_alien_voltigore");
	if ( strlen( STRING( m_szMonsterName ) ) == 0 )
	{
		// default name
		m_szMonsterName = MAKE_STRING( "Voltigore" );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMVoltigore::Precache()
{
	PrecacheImpl("models/voltigore.mdl");
	PRECACHE_MODEL("models/vgibs.mdl");
}

void CMVoltigore::PrecacheImpl(char *modelName)
{
	PRECACHE_MODEL(modelName);
	
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pAttackMeleeSounds);
	PRECACHE_SOUND_ARRAY(pMeleeHitSounds);
	PRECACHE_SOUND_ARRAY(pMeleeMissSounds);
	PRECACHE_SOUND_ARRAY(pComSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pFootstepSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pGruntSounds);

	PRECACHE_SOUND("voltigore/voltigore_attack_shock.wav");
	PRECACHE_SOUND("voltigore/voltigore_eat.wav");

	PRECACHE_SOUND("debris/beamstart1.wav");

	m_beamTexture = PRECACHE_MODEL(VOLTIGORE_ZAP_BEAM);
	PRECACHE_MODEL(VOLTIGORE_GLOW_SPRITE);
	
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("sprites/blueflare2.spr");
}

//=========================================================
// DeathSound
//=========================================================
void CMVoltigore::DeathSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1, ATTN_NORM);
}

//=========================================================
// AttackSound
//=========================================================
void CMVoltigore::AttackSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "voltigore/voltigore_attack_shock.wav", 1, ATTN_NORM);
}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlVoltigoreRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slVoltigoreRangeAttack1[] =
{
	{
		tlVoltigoreRangeAttack1,
		ARRAYSIZE(tlVoltigoreRangeAttack1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE |
		// Attack animation is quite long, so it's better to not stop it when enemy hides
		//bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"Voltigore Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlVoltigoreChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_CHASE_ENEMY_FAILED },
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slVoltigoreChaseEnemy[] =
{
	{
		tlVoltigoreChaseEnemy1,
		ARRAYSIZE(tlVoltigoreChaseEnemy1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_SMELL_FOOD |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_TASK_FAILED |
		bits_COND_HEAR_SOUND,
		0,
		"Voltigore Chase Enemy"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t tlVoltigoreVictoryDance[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_WAIT, (float)0.2 },
	{ TASK_VOLTIGORE_GET_PATH_TO_ENEMY_CORPSE,	(float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_STAND },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, (float)ACT_STAND },
};

Schedule_t slVoltigoreVictoryDance[] =
{
	{
		tlVoltigoreVictoryDance,
		ARRAYSIZE( tlVoltigoreVictoryDance ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"VoltigoreVictoryDance"
	},
};

DEFINE_CUSTOM_SCHEDULES(CMVoltigore)
{
	slVoltigoreRangeAttack1,
	slVoltigoreChaseEnemy,
	slVoltigoreVictoryDance
};

IMPLEMENT_CUSTOM_SCHEDULES(CMVoltigore, CMBaseMonster)

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CMVoltigore::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CMBaseMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
		}

		if( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
		{
			return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}

		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		return GetScheduleOfType(SCHED_CHASE_ENEMY);

		break;
	}
	}

	return CMBaseMonster::GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CMVoltigore::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return &slVoltigoreRangeAttack1[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slVoltigoreChaseEnemy[0];
		break;
	case SCHED_VICTORY_DANCE:
		return &slVoltigoreVictoryDance[0];
		break;
	}

	return CMBaseMonster::GetScheduleOfType(Type);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for voltigore because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CMVoltigore::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	GlowOff();
	DestroyBeams();
	m_fShouldUpdateBeam = FALSE;

	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
		{
			CreateBeams();

			GlowOn( 255 );
			m_fShouldUpdateBeam = TRUE;

			// Play the beam 'glow' sound.
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "debris/beamstart1.wav", 1, ATTN_NORM, 0, PITCH_HIGH);

			CMBaseMonster::StartTask(pTask);
		}
		break;
	case TASK_GET_PATH_TO_ENEMY:
		{
			if (BuildRoute(m_hEnemy->v.origin, bits_MF_TO_ENEMY, m_hEnemy))
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			else
			{
				ALERT(at_aiconsole, "GetPathToEnemy failed!!\n");
				TaskFail();
			}
		}
		break;
	case TASK_VOLTIGORE_GET_PATH_TO_ENEMY_CORPSE:
		UTIL_MakeVectors( pev->angles );
		if( BuildRoute( m_vecEnemyLKP - gpGlobals->v_forward * 50, bits_MF_TO_LOCATION, NULL ) )
		{
			TaskComplete();
		}
		else
		{
			ALERT( at_aiconsole, "VoltigoreGetPathToEnemyCorpse failed!!\n" );
			TaskFail();
		}
	default:
		CMBaseMonster::StartTask(pTask);
		break;
	}
}

void CMVoltigore::Killed(entvars_t *pevAttacker, int iGib)
{
	DestroyBeams();
	DestroyGlow();

	int iTimes = 0;
	int iDrawn = 0;
	const int iBeams = VOLTIGORE_MAX_BEAMS;
	while( iDrawn < iBeams && iTimes < ( iBeams * 3 ) )
	{
		TraceResult tr;
		const Vector vecOrigin = Center();
		const Vector vecDest = VOLTIGORE_ZAP_DISTANCE * ( Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) ).Normalize() );
		UTIL_TraceLine( vecOrigin, vecOrigin + vecDest, ignore_monsters, ENT( pev ), &tr );
		if( tr.flFraction != 1.0 )
		{
			// we hit something.
			iDrawn++;
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_BEAMPOINTS );
				WRITE_COORD( vecOrigin.x );
				WRITE_COORD( vecOrigin.y );
				WRITE_COORD( vecOrigin.z );
				WRITE_COORD( tr.vecEndPos.x );
				WRITE_COORD( tr.vecEndPos.y );
				WRITE_COORD( tr.vecEndPos.z );
				WRITE_SHORT( m_beamTexture );
				WRITE_BYTE( 0 ); // framestart
				WRITE_BYTE( 10 ); // framerate
				WRITE_BYTE( RANDOM_LONG( 8, 10 ) ); // life
				WRITE_BYTE( VOLTIGORE_ZAP_WIDTH );  // width
				WRITE_BYTE( VOLTIGORE_ZAP_NOISE );   // noise
				WRITE_BYTE( VOLTIGORE_ZAP_RED );   // r, g, b
				WRITE_BYTE( VOLTIGORE_ZAP_GREEN);   // r, g, b
				WRITE_BYTE( VOLTIGORE_ZAP_BLUE );   // r, g, b
				WRITE_BYTE( VOLTIGORE_ZAP_BRIGHTNESS );	// brightness
				WRITE_BYTE( 35 );		// speed
			MESSAGE_END();
		}
		iTimes++;
	}

	CMBaseMonster::Killed(pevAttacker, iGib);
}

void CMVoltigore::GibBeamDamage()
{
	edict_t *pEntity = NULL;
	// iterate on all entities in the vicinity.
	const float attackRadius = gSkillData.voltigoreDmgBeam * 10;
	float flAdjustedDamage = gSkillData.voltigoreDmgBeam/2;
	while( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, attackRadius ) ) != NULL )
	{
		if( pEntity->v.takedamage != DAMAGE_NO )
		{
			if( pEntity->v.classname != pev->classname && !FClassnameIs( pEntity, "monster_alien_babyvoltigore" ) )
			{
				// voltigores don't hurt other (baby) voltigores on death
				const float flDist = ( UTIL_Center( pEntity ) - pev->origin ).Length();

				flAdjustedDamage -= ( flDist / attackRadius ) * flAdjustedDamage;

				if( !UTIL_FVisible( pEntity, this->edict() ) )
				{
					if( UTIL_IsPlayer( pEntity ) )
					{
						// if this entity is a client, and is not in full view, inflict half damage. We do this so that players still 
						// take the residual damage if they don't totally leave the voltigore's effective radius. We restrict it to clients
						// so that monsters in other parts of the level don't take the damage and get pissed.
						flAdjustedDamage *= 0.5;
					}
					else if( !FClassnameIs( pEntity, "func_breakable" ) && !FClassnameIs( pEntity, "func_pushable" ) ) 
					{
						// do not hurt nonclients through walls, but allow damage to be done to breakables
						flAdjustedDamage = 0;
					}
				}

				if( flAdjustedDamage > 0 )
				{
					if ( UTIL_IsPlayer( pEntity ) )
						UTIL_TakeDamage( pEntity, pev, pev, flAdjustedDamage, DMG_SHOCK );
					else if ( pEntity->v.euser4 != NULL )
					{
						CMBaseMonster *pMonster = GetClassPtr((CMBaseMonster *)VARS(pEntity));
						pMonster->TakeDamage( pev, pev, flAdjustedDamage, DMG_SHOCK );
					}
				}
			}
		}
	}
}

void CMVoltigore::CreateBeams()
{
	Vector vecStart, vecEnd, vecAngles;
	GetAttachment(3, vecStart, vecAngles);

	for (int i = 0; i < 3; i++)
	{
		m_pBeam[i] = CMBeam::BeamCreate(VOLTIGORE_ZAP_BEAM, VOLTIGORE_ZAP_WIDTH);
		if (!m_pBeam[i])
			return;

		GetAttachment(i, vecEnd, vecAngles);

		m_pBeam[i]->PointsInit(vecStart, vecEnd);
		m_pBeam[i]->SetColor(VOLTIGORE_ZAP_RED, VOLTIGORE_ZAP_GREEN, VOLTIGORE_ZAP_BLUE);
		m_pBeam[i]->SetBrightness(VOLTIGORE_ZAP_BRIGHTNESS);
		m_pBeam[i]->SetNoise(VOLTIGORE_ZAP_NOISE);
	}
}

void CMVoltigore::DestroyBeams()
{
	for (int i = 0; i < 3; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]->edict());
			m_pBeam[i] = NULL;
		}
	}
}

void CMVoltigore::UpdateBeams()
{
	Vector vecStart, vecEnd, vecAngles;
	GetAttachment(3, vecStart, vecAngles);

	for (int i = 0; i < 3; i++)
	{
		if (!m_pBeam[i]) {
			continue;
		}
		GetAttachment(i, vecEnd, vecAngles);
		m_pBeam[i]->SetStartPos(vecStart);
		m_pBeam[i]->SetEndPos(vecEnd);
		m_pBeam[i]->RelinkBeam();
	}
}

void CMVoltigore::CreateGlow()
{
	m_pBeamGlow = CMSprite::SpriteCreate(VOLTIGORE_GLOW_SPRITE, pev->origin, FALSE);
	m_pBeamGlow->SetTransparency(kRenderTransAdd, 255, 255, 255, 0, kRenderFxNoDissipation);
	m_pBeamGlow->SetAttachment(edict(), 4);
	m_pBeamGlow->SetScale(VOLTIGORE_GLOW_SCALE);
}

void CMVoltigore::DestroyGlow()
{
	if (m_pBeamGlow)
	{
		UTIL_Remove(m_pBeamGlow->edict());
		m_pBeamGlow = NULL;
	}
}

void CMVoltigore::GlowUpdate()
{
	if (m_pBeamGlow)
	{
		m_pBeamGlow->pev->renderamt = UTIL_Approach(m_glowBrightness, m_pBeamGlow->pev->renderamt, 100);
		if (m_pBeamGlow->pev->renderamt == 0)
			m_pBeamGlow->pev->effects |= EF_NODRAW;
		else
			m_pBeamGlow->pev->effects &= ~EF_NODRAW;
		UTIL_SetOrigin(m_pBeamGlow->pev, pev->origin);
	}
}

void CMVoltigore::GlowOff(void)
{
	m_glowBrightness = 0;
}

void CMVoltigore::GlowOn(int level)
{
	m_glowBrightness = level;
}


//=========================================================
// CBabyAlienVoltigore
//=========================================================

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BABY_VOLTIGORE_AE_RUN			( 14 )

//=========================================================
// Spawn
//=========================================================
void CMBabyVoltigore::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), (!FStringNull( pev->model ) ? STRING( pev->model ) : "models/baby_voltigore.mdl"));
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 36));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.babyVoltigoreHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_flNextZapTime	= gpGlobals->time;

	MonsterInit();
	pev->view_ofs		= Vector(0, 0, 32);
	
	pev->classname = MAKE_STRING("monster_alien_babyvoltigore");
	if ( strlen( STRING( m_szMonsterName ) ) == 0 )
	{
		// default name
		m_szMonsterName = MAKE_STRING( "Baby Voltigore" );
	}
}

//=========================================================
//=========================================================
void CMBabyVoltigore::Precache(void)
{
	PrecacheImpl("models/baby_voltigore.mdl");
}

void CMBabyVoltigore::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BABY_VOLTIGORE_AE_RUN:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pFootstepSounds), RANDOM_FLOAT(0.5, 0.6), ATTN_NORM, 0, RANDOM_LONG(85, 120));
		break;

	case VOLTIGORE_AE_PUNCH_SINGLE:
	{
		edict_t *pHurt = CheckTraceHullAttack(70, gSkillData.babyVoltigoreDmgPunch, DMG_CLUB | DMG_ALWAYSGIB);
		if (pHurt)
		{
			if (FBitSet(pHurt->v.flags, FL_MONSTER|FL_CLIENT))
			{
				pHurt->v.punchangle.z = -10;
				pHurt->v.punchangle.x = 10;
				pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_right * -100;
				pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_up * 50;
			}

			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeHitSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

			Vector vecArmPos, vecArmAng;
			GetAttachment( 0, vecArmPos, vecArmAng );
			SpawnBlood( vecArmPos, BloodColor(), 25 );// a little surface blood.
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeMissSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
	}
	break;

	case VOLTIGORE_AE_PUNCH_BOTH:
	{
		edict_t *pHurt = CheckTraceHullAttack(70, gSkillData.babyVoltigoreDmgPunch, DMG_CLUB | DMG_ALWAYSGIB);
		if (pHurt)
		{
			if (FBitSet(pHurt->v.flags, FL_MONSTER|FL_CLIENT))
			{
				pHurt->v.punchangle.x = 15;
				pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_forward * 100;
				pHurt->v.velocity = pHurt->v.velocity + gpGlobals->v_up * 50;
			}

			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeHitSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

			Vector vecArmPos, vecArmAng;
			GetAttachment( 0, vecArmPos, vecArmAng );
			SpawnBlood( vecArmPos, BloodColor(), 25 );// a little surface blood.
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeMissSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
	}
	break;
	default:
		CMVoltigore::HandleAnimEvent(pEvent);
		break;
	}
}

BOOL CMBabyVoltigore::CheckMeleeAttack1(float flDot, float flDist)
{
	return CMBaseMonster::CheckMeleeAttack1(flDot, flDist);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for voltigore because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CMBabyVoltigore::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK1:
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackMeleeSounds), RANDOM_FLOAT(0.5, 0.6), ATTN_NONE, 0, RANDOM_LONG(110, 120));
		CMBaseMonster::StartTask(pTask);
	}
	break;
	default:
		CMBaseMonster::StartTask(pTask);
		break;
	}
}

void CMBabyVoltigore::Killed(entvars_t* pevAttacker, int iGib)
{
	DestroyBeams();
	CMBaseMonster::Killed(pevAttacker, iGib);
}

void CMBabyVoltigore::GibMonster()
{
	CMBaseMonster::GibMonster();
}

BOOL CMBabyVoltigore::CheckRangeAttack1(float flDot, float flDist)
{
	return FALSE;
}

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CMBabyVoltigore::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CMBaseMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		return GetScheduleOfType(SCHED_CHASE_ENEMY);

		break;
	}
	}

	return CMBaseMonster::GetSchedule();
}

Schedule_t *CMBabyVoltigore::GetScheduleOfType(int Type)
{
	switch (Type) {
	// For some cryptic reason baby voltigore tries to start the range attack even though its model does not have sequence with range attack activity. 
	// This hack is for preventing baby voltigore to do this.
	case SCHED_RANGE_ATTACK1:
		return &slVoltigoreChaseEnemy[0];
		break;
	default:
		return CMVoltigore::GetScheduleOfType(Type);
		break;
	}
}
