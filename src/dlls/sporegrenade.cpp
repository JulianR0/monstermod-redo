// HUGE thanks to DrBeef for his hlsdk-xash3d-opfor repository!

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


#include "extdll.h"
#include "util.h"
#include "cmbase.h"
#include "cmbasemonster.h"
#include "monsters.h"
#include "weapons.h"
#include "decals.h"
#include "explode.h"

int gSporeExplode, gSporeExplodeC;

void CMSporeGrenade::Precache()
{
	PRECACHE_MODEL("models/spore.mdl");
	PRECACHE_MODEL("sprites/glow02.spr");
	g_sModelIndexTinySpit = PRECACHE_MODELINDEX("sprites/tinyspit.spr");
	gSporeExplode = PRECACHE_MODELINDEX("sprites/spore_exp_01.spr");
	gSporeExplodeC = PRECACHE_MODELINDEX("sprites/spore_exp_c_01.spr");
	PRECACHE_SOUND("weapons/splauncher_bounce.wav");
	PRECACHE_SOUND("weapons/splauncher_impact.wav");
}

void CMSporeGrenade::Explode(TraceResult *pTrace)
{
	if (m_hOwner == NULL)
		pev->owner = NULL;

	pev->solid = SOLID_NOT;// intangible
	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	Vector vecSpraySpot = pTrace->vecEndPos;
	float flSpraySpeed = RANDOM_LONG(10, 15);

	// If the trace is pointing up, then place
	// spawn position a few units higher.
	if (pTrace->vecPlaneNormal.z > 0)
	{
		vecSpraySpot = vecSpraySpot + (pTrace->vecPlaneNormal * 8);
		flSpraySpeed *= 2; // Double the speed to make them fly higher
						   // in the air.
	}

	// Spawn small particles at the explosion origin.
	SpawnExplosionParticles(
		vecSpraySpot,				// position
		pTrace->vecPlaneNormal,			// direction
		g_sModelIndexTinySpit,			// modelindex
		RANDOM_LONG(40, 50),				// count
		flSpraySpeed,					// speed
		RANDOM_FLOAT(600, 640));		// noise

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( RANDOM_LONG( 0, 1 ) ? gSporeExplode : gSporeExplodeC );
		WRITE_BYTE( 25  ); // scale * 10
		WRITE_BYTE( 155  ); // framerate
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD( pev->origin.x );	// X
		WRITE_COORD( pev->origin.y );	// Y
		WRITE_COORD( pev->origin.z );	// Z
		WRITE_BYTE( 12 );		// radius * 0.1
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 180 );		// g
		WRITE_BYTE( 0 );		// b
		WRITE_BYTE( 20 );		// time * 10
		WRITE_BYTE( 20 );		// decay * 0.1
	MESSAGE_END( );

	// Play explode sound.
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/splauncher_impact.wav", 1, ATTN_NORM);

	entvars_t *pevOwner;
	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage(pev, pevOwner, pev->dmg, CLASS_NONE, DMG_BLAST);

	// Place a decal on the surface that was hit.
	UTIL_DecalTrace(pTrace, DECAL_SPIT1 + RANDOM_LONG(0, 1));
	
	UpdateOnRemove();
	UTIL_Remove( this->edict() );
}

void CMSporeGrenade::Detonate()
{
	TraceResult tr;
	Vector vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);

	Explode(&tr);
}


void CMSporeGrenade::BounceSound()
{
	if (m_hOwner == NULL)
		pev->owner = NULL;

	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/splauncher_bounce.wav", 0.25, ATTN_NORM);
}

void CMSporeGrenade::TumbleThink()
{
	if (!IsInWorld())
	{
		UpdateOnRemove();
		UTIL_Remove( this->edict() );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink(&CMSporeGrenade::Detonate);
	}

	// Spawn particles.
	SpawnTrailParticles(
		pev->origin,					// position
		-pev->velocity.Normalize(),		// dir
		g_sModelIndexTinySpit,			// modelindex
		RANDOM_LONG( 2, 4 ),			// count
		RANDOM_FLOAT(10, 15),			// speed
		RANDOM_FLOAT(2, 3) * 100);		// noise ( client will divide by 100 )
}

//
// Contact grenade, explode when it touches something
//
void CMSporeGrenade::ExplodeTouch(edict_t *pOther)
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	pev->enemy = pOther;

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine(vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr);

	Explode(&tr);
}

void CMSporeGrenade::DangerSoundThink()
{
	if (!IsInWorld())
	{
		UpdateOnRemove();
		UTIL_Remove( this->edict() );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.2;

	// Spawn particles.
	SpawnTrailParticles(
		pev->origin,					// position
		-pev->velocity.Normalize(),		// dir
		g_sModelIndexTinySpit,			// modelindex
		RANDOM_LONG( 5, 10),				// count
		RANDOM_FLOAT(10, 15),			// speed
		RANDOM_FLOAT(2, 3) * 100);		// noise ( client will divide by 100 )
}

void CMSporeGrenade::BounceTouch(edict_t *pOther)
{
	if ( !pOther->v.takedamage )
	{
		if (!(pev->flags & FL_ONGROUND)) {
			if (pev->dmg_save < gpGlobals->time) {
				BounceSound();
				pev->dmg_save = gpGlobals->time + 0.1;
			}
		} else {
			pev->velocity = pev->velocity * 0.9;
		}
		if (pev->flags & FL_SWIM)
		{
			pev->velocity = pev->velocity * 0.5;
		}
	}
	else
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		Explode(&tr);
	}
}

void CMSporeGrenade::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("spore");
	pev->movetype = MOVETYPE_BOUNCE;

	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/spore.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	//pev->gravity = 0.5;

	pev->dmg = gSkillData.monDmgSpore;

	m_pSporeGlow = CMSprite::SpriteCreate("sprites/glow02.spr", pev->origin, FALSE);

	if (m_pSporeGlow)
	{
		m_pSporeGlow->SetTransparency(kRenderGlow, 150, 158, 19, 155, kRenderFxNoDissipation);
		m_pSporeGlow->SetAttachment(edict(), 0);
		m_pSporeGlow->SetScale(.75f);
	}
}

CMSporeGrenade* CMSporeGrenade::ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, bool ai)
{
	CMSporeGrenade *pGrenade = CreateClassPtr((CMSporeGrenade *)NULL);
	
	if (pGrenade == NULL)
		return NULL;
	
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->Spawn();
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);
	pGrenade->m_hOwner = ENT(pevOwner);

	pGrenade->SetTouch(&CMSporeGrenade::BounceTouch);	// Bounce if touched

	float lifetime = 2.0;
	if (ai) {
		lifetime = 4.0;
		pGrenade->pev->gravity = 0.5;
		pGrenade->pev->friction = 0.9;
	}
	pGrenade->pev->dmgtime = gpGlobals->time + lifetime;
	pGrenade->SetThink(&CMSporeGrenade::TumbleThink);
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if (lifetime < 0.1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector(0, 0, 0);
	}

	return pGrenade;
}

CMSporeGrenade *CMSporeGrenade::ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CMSporeGrenade *pGrenade = CreateClassPtr((CMSporeGrenade *)NULL);
	
	if (pGrenade == NULL)
		return NULL;
	
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->Spawn();
	pGrenade->pev->movetype = MOVETYPE_FLY;
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);
	pGrenade->m_hOwner = ENT(pevOwner);

	// make monsters afraid of it while in the air
	pGrenade->SetThink(&CMSporeGrenade::DangerSoundThink);
	pGrenade->pev->nextthink = gpGlobals->time;

	// Explode on contact
	pGrenade->SetTouch(&CMSporeGrenade::ExplodeTouch);

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.7;

	return pGrenade;
}

void CMSporeGrenade::SpawnTrailParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise)
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
	WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(origin.x);				// pos
		WRITE_COORD(origin.y);
		WRITE_COORD(origin.z);
		WRITE_COORD(direction.x);			// dir
		WRITE_COORD(direction.y);
		WRITE_COORD(direction.z);
		WRITE_SHORT(modelindex);			// model
		WRITE_BYTE(count);					// count
		WRITE_BYTE(speed);					// speed
		WRITE_BYTE(noise);					// noise ( client will divide by 100 )
	MESSAGE_END();
}

void CMSporeGrenade::SpawnExplosionParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise)
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
		WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(origin.x);				// pos
		WRITE_COORD(origin.y);
		WRITE_COORD(origin.z);
		WRITE_COORD(direction.x);			// dir
		WRITE_COORD(direction.y);
		WRITE_COORD(direction.z);
		WRITE_SHORT(modelindex);			// model
		WRITE_BYTE(count);					// count
		WRITE_BYTE(speed);					// speed
		WRITE_BYTE(noise);					// noise ( client will divide by 100 )
	MESSAGE_END();
}

void CMSporeGrenade::UpdateOnRemove()
{
	CMBaseMonster::UpdateOnRemove();
	if (m_pSporeGlow)
	{
		UTIL_Remove(m_pSporeGlow->edict());
		m_pSporeGlow = NULL;
	}
}
