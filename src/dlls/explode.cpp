/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
/*

===== explode.cpp ========================================================

  Explosion-related code

*/
#include "extdll.h"
#include "util.h"
#include "cmbase.h"
#include "cmbasemonster.h"
#include "decals.h"
#include "explode.h"

// Spark Shower
class CMShower : public CMBaseEntity
{
	void Spawn( void );
	void Think( void );
	void Touch( CMBaseEntity *pOther );
	int ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

void CMShower::Spawn( void )
{
	pev->velocity = RANDOM_FLOAT( 200, 300 ) * pev->angles;
	pev->velocity.x += RANDOM_FLOAT(-100.f,100.f);
	pev->velocity.y += RANDOM_FLOAT(-100.f,100.f);
	if ( pev->velocity.z >= 0 )
		pev->velocity.z += 200;
	else
		pev->velocity.z -= 200;
	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;
	SET_MODEL( edict(), "models/grenade.mdl");	// Need a model, just use the grenade, we don't draw it anyway
	UTIL_SetSize(pev, g_vecZero, g_vecZero );
	pev->effects |= EF_NODRAW;
	pev->speed = RANDOM_FLOAT( 0.5, 1.5 );

	pev->angles = g_vecZero;
    pev->classname = MAKE_STRING( "_spark_shower" );
}


void CMShower::Think( void )
{
	UTIL_Sparks( pev->origin );

	pev->speed -= 0.1;
	if ( pev->speed > 0 )
		pev->nextthink = gpGlobals->time + 0.1;
	else
		UTIL_Remove( this->edict() );
	pev->flags &= ~FL_ONGROUND;
}

void CMShower::Touch( CMBaseEntity *pOther )
{
	if ( pev->flags & FL_ONGROUND )
		pev->velocity = pev->velocity * 0.1;
	else
		pev->velocity = pev->velocity * 0.6;

	if ( (pev->velocity.x*pev->velocity.x+pev->velocity.y*pev->velocity.y) < 10.0 )
		pev->speed = 0;
}

class CMEnvExplosion : public CMBaseMonster
{
public:
	void Spawn( );
	void EXPORT Smoke ( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CMBaseEntity *pActivator, CMBaseEntity *pCaller, USE_TYPE useType, float value );

	int m_iMagnitude;// how large is the fireball? how much damage?
	int m_spriteScale; // what's the exact fireball sprite scale? 
};

void CMEnvExplosion::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "iMagnitude"))
	{
		m_iMagnitude = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CMBaseEntity::KeyValue( pkvd );
}

void CMEnvExplosion::Spawn( void )
{ 
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	pev->movetype = MOVETYPE_NONE;
	/*
	if ( m_iMagnitude > 250 )
	{
		m_iMagnitude = 250;
	}
	*/

	float flSpriteScale;
	flSpriteScale = ( m_iMagnitude - 50) * 0.6;
	
	/*
	if ( flSpriteScale > 50 )
	{
		flSpriteScale = 50;
	}
	*/
	if ( flSpriteScale < 10 )
	{
		flSpriteScale = 10;
	}

	m_spriteScale = (int)flSpriteScale;
    pev->classname = MAKE_STRING( "_env_explosion" );
}

void CMEnvExplosion::Use( CMBaseEntity *pActivator, CMBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	TraceResult tr;

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector ( 0 , 0 , 8 );
	
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, ENT(pev), & tr);
	
	// Pull out of the wall a bit
	if ( tr.flFraction != 1.0 )
	{
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * (m_iMagnitude - 24) * 0.6);
	}
	else
	{
		pev->origin = pev->origin;
	}

	// draw decal
	if (! ( pev->spawnflags & SF_ENVEXPLOSION_NODECAL))
	{
		if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
		{
			UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
		}
		else
		{
			UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
		}
	}

	// draw fireball
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NOFIREBALL ) )
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( (BYTE)m_spriteScale ); // scale * 10
			WRITE_BYTE( 15  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 0 ); // no sprite
			WRITE_BYTE( 15  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();
	}

	// do damage
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NODAMAGE ) )
	{
		RadiusDamage ( pev, pev->owner == NULL ? pev : VARS( pev->owner ), m_iMagnitude, CLASS_NONE, DMG_BLAST );
	}

	SetThink( &CMEnvExplosion::Smoke );
	pev->nextthink = gpGlobals->time + 0.3;

	// draw sparks
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NOSPARKS ) )
	{
		int sparkCount = RANDOM_LONG(0,3);

		for ( int i = 0; i < sparkCount; i++ )
		{
			CMBaseEntity *pSpark = CreateClassPtr((CMShower *)NULL);
			if ( pSpark == NULL )
			{
				ALERT( at_console, "Failed to spawn spark_shower!" );
			}
			else
			{
				UTIL_SetOrigin( pSpark->pev, pev->origin );
				pSpark->pev->angles = tr.vecPlaneNormal;
				pSpark->Spawn();
			}
			// Create( "spark_shower", pev->origin, tr.vecPlaneNormal, NULL );
		}
	}
}

void CMEnvExplosion::Smoke( void )
{
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NOSMOKE ) )
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( (BYTE)m_spriteScale ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
		MESSAGE_END();
	}
	
	if ( !(pev->spawnflags & SF_ENVEXPLOSION_REPEATABLE) )
	{
		UTIL_Remove( this->edict() );
	}
}


// Stock to quickly create a one-time explosion
void ExplosionCreate( const Vector &center, const Vector &angles, edict_t *pOwner, int magnitude, int flags, float delay )
{
	KeyValueData	kvd;
	char			buf[128];
	
	//CMBaseEntity *pExplosion = CMBaseEntity::Create( "env_explosion", center, angles, pOwner );
	CMBaseEntity *pExplosion = CreateClassPtr((CMEnvExplosion *)NULL);
	if ( pExplosion == NULL )
	{
		ALERT( at_console, "Failed to create env_explosion!" );
	}
	else
	{
		sprintf( buf, "%3d", magnitude );
		kvd.szKeyName = "iMagnitude";
		kvd.szValue = buf;
		pExplosion->KeyValue( &kvd );
		pExplosion->pev->owner = pOwner;
		pExplosion->pev->spawnflags |= flags;
		
		// This is a temporary entity, filter out the flag
		pExplosion->pev->spawnflags &= ~SF_ENVEXPLOSION_REPEATABLE;
		
		if ( delay > 0.0f )
		{
			pExplosion->SetThink( &CMBaseEntity::SUB_CallUseToggle );
			pExplosion->pev->nextthink = gpGlobals->time + delay;
		}
		else
		{
			pExplosion->Use( NULL, NULL, USE_TOGGLE, 0 );
		}
		pExplosion->Spawn();
	}
}
