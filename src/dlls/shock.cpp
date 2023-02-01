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
// shock - projectile shot from shockrifles.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cmbase.h"
#include	"cmbasemonster.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"effects.h"
#include	"decals.h"
#include	"weapons.h"
#include	"customentity.h"
#include	"shock.h"


void CMShock::Spawn()
{
	Precache();
	
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("shock_beam");
	SET_MODEL(ENT(pev), "models/shock_effect.mdl");
	UTIL_SetOrigin(pev, pev->origin);
	pev->dmg = gSkillData.monDmgShockroach;
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));
	
	CreateEffects();
	SetThink( &CMShock::FlyThink );
	pev->nextthink = gpGlobals->time;
}

void CMShock::Precache()
{
	PRECACHE_MODEL("sprites/flare3.spr");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("models/shock_effect.mdl");
	PRECACHE_SOUND("weapons/shock_impact.wav");	
}

void CMShock::FlyThink()
{
	if (pev->waterlevel == 3)
	{
		entvars_t *pevOwner = VARS(pev->owner);
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/shock_impact.wav", VOL_NORM, ATTN_NORM);
		RadiusDamage(pev->origin, pev, pevOwner ? pevOwner : pev, pev->dmg * 3, 144, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
		ClearEffects();
		SetThink( &CMBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.05;
	}
}

edict_t *CMShock::Shoot(entvars_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity)
{
	CMShock *pShock = CreateClassPtr((CMShock *)NULL);
	
	if (pShock == NULL)
		return NULL;
	
	UTIL_SetOrigin(pShock->pev, vecStart);
	pShock->Spawn();

	pShock->pev->velocity = vecVelocity;
	pShock->pev->owner = ENT(pevOwner);
	pShock->pev->angles = angles;

	pShock->pev->nextthink = gpGlobals->time;
	
	return pShock->edict();
}

void CMShock::Touch(edict_t *pOther)
{
	// Do not collide with the owner.
	if (pOther == pev->owner)
		return;

	TraceResult tr = UTIL_GetGlobalTrace( );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 8 );		// radius * 0.1
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 255 );		// b
		WRITE_BYTE( 10 );		// time * 10
		WRITE_BYTE( 10 );		// decay * 0.1
	MESSAGE_END( );

	ClearEffects();
	if (!pOther->v.takedamage)
	{
		// make a splat on the wall
		const int baseDecal = DECAL_SCORCH1;
		UTIL_DecalTrace(&tr, baseDecal + RANDOM_LONG(0, 1));

		int iContents = UTIL_PointContents(pev->origin);

		// Create sparks
		if (iContents != CONTENTS_WATER)
		{
			UTIL_Sparks(tr.vecEndPos);
		}
	}
	else
	{
		int damageType = DMG_SHOCK;
		ClearMultiDamage();
		entvars_t *pevOwner = VARS(pev->owner);
		entvars_t *pevAttacker = pevOwner ? pevOwner : pev;
		
		if ( UTIL_IsPlayer( pOther ) )
			UTIL_TraceAttack( pOther, pevAttacker, pev->dmg, pev->velocity.Normalize(), &tr, damageType );
		else if ( pOther->v.euser4 != NULL )
		{
			CMBaseMonster *pMonster = GetClassPtr((CMBaseMonster *)VARS(pOther));
			pMonster->TraceAttack( pevAttacker, pev->dmg, pev->velocity.Normalize(), &tr, damageType );
		}
		
		ApplyMultiDamage(pev, pevAttacker);
	}

	// splat sound
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/shock_impact.wav", VOL_NORM, ATTN_NORM);

	pev->modelindex = 0;
	pev->solid = SOLID_NOT;
	SetThink( &CMBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.01; // let the sound play
}

void CMShock::CreateEffects()
{
	m_pSprite = CMSprite::SpriteCreate( "sprites/flare3.spr", pev->origin, FALSE );
	m_pSprite->SetAttachment( edict(), 0 );
	m_pSprite->pev->scale = 0.35;
	m_pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 170, kRenderFxNoDissipation );
	//m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	//m_pSprite->pev->flags |= FL_SKIPLOCALHOST;

	m_pBeam = CMBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	if (m_pBeam)
	{
		m_pBeam->EntsInit( entindex(), entindex() );
		m_pBeam->SetStartAttachment( 1 );
		m_pBeam->SetEndAttachment( 2 );
		m_pBeam->SetBrightness( 180 );
		m_pBeam->SetScrollRate( 10 );
		m_pBeam->SetNoise( 0 );
		m_pBeam->SetFlags( BEAM_FSHADEOUT );
		m_pBeam->SetColor( 0, 255, 255 );
		//m_pBeam->pev->spawnflags = SF_BEAM_TEMPORARY;
		m_pBeam->RelinkBeam();
	}
	else
	{
		ALERT(at_console, "Could not create shockbeam beam!\n");
	}

	m_pNoise = CMBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	if (m_pNoise)
	{
		m_pNoise->EntsInit( entindex(), entindex() );
		m_pNoise->SetStartAttachment( 1 );
		m_pNoise->SetEndAttachment( 2 );
		m_pNoise->SetBrightness( 180 );
		m_pNoise->SetScrollRate( 30 );
		m_pNoise->SetNoise( 30 );
		m_pNoise->SetFlags( BEAM_FSHADEOUT );
		m_pNoise->SetColor( 255, 255, 173 );
		//m_pNoise->pev->spawnflags = SF_BEAM_TEMPORARY;
		m_pNoise->RelinkBeam();
	}
	else
	{
		ALERT(at_console, "Could not create shockbeam noise!\n");
	}
}

void CMShock::ClearEffects()
{
	if (m_pBeam)
	{
		UTIL_Remove( m_pBeam->edict() );
		m_pBeam = NULL;
	}

	if (m_pNoise)
	{
		UTIL_Remove( m_pNoise->edict() );
		m_pNoise = NULL;
	}

	if (m_pSprite)
	{
		UTIL_Remove( m_pSprite->edict() );
		m_pSprite = NULL;
	}
}

void CMShock::UpdateOnRemove()
{
	CMBaseAnimating::UpdateOnRemove();
	ClearEffects();
}
