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
/*

===== util.cpp ========================================================

  Utility code.  Really not optional after all.

*/

#include <stdio.h>
#include <stdarg.h>		// va_start, etc
#include <time.h>

#include "extdll.h"
#include "util.h"
#include "cmbase.h"
#include "cmbasemonster.h"
#include "shake.h"
#include "decals.h"
#include "weapons.h"
#include "monsters.h"

typedef struct {
   DLL_FUNCTIONS *dllapi_table;
   NEW_DLL_FUNCTIONS *newapi_table;
} gamedll_funcs_t;

extern gamedll_funcs_t *gpGamedllFuncs;


// Print to console.
void META_CONS(char *fmt, ...) {
	va_list ap;
	char buf[1024];
	unsigned int len;

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	len=strlen(buf);
	if(len < sizeof(buf)-2)		// -1 null, -1 for newline
		strcat(buf, "\n");
	else
		buf[len-1] = '\n';

	(*g_engfuncs.pfnServerPrint)(buf);
}

float UTIL_WeaponTimeBase( void )
{
#if defined( CLIENT_WEAPONS )
	return 0.0;
#else
	return gpGlobals->time;
#endif
}

static unsigned int glSeed = 0; 

unsigned int seed_table[ 256 ] =
{
	28985, 27138, 26457, 9451, 17764, 10909, 28790, 8716, 6361, 4853, 17798, 21977, 19643, 20662, 10834, 20103,
	27067, 28634, 18623, 25849, 8576, 26234, 23887, 18228, 32587, 4836, 3306, 1811, 3035, 24559, 18399, 315,
	26766, 907, 24102, 12370, 9674, 2972, 10472, 16492, 22683, 11529, 27968, 30406, 13213, 2319, 23620, 16823,
	10013, 23772, 21567, 1251, 19579, 20313, 18241, 30130, 8402, 20807, 27354, 7169, 21211, 17293, 5410, 19223,
	10255, 22480, 27388, 9946, 15628, 24389, 17308, 2370, 9530, 31683, 25927, 23567, 11694, 26397, 32602, 15031,
	18255, 17582, 1422, 28835, 23607, 12597, 20602, 10138, 5212, 1252, 10074, 23166, 19823, 31667, 5902, 24630,
	18948, 14330, 14950, 8939, 23540, 21311, 22428, 22391, 3583, 29004, 30498, 18714, 4278, 2437, 22430, 3439,
	28313, 23161, 25396, 13471, 19324, 15287, 2563, 18901, 13103, 16867, 9714, 14322, 15197, 26889, 19372, 26241,
	31925, 14640, 11497, 8941, 10056, 6451, 28656, 10737, 13874, 17356, 8281, 25937, 1661, 4850, 7448, 12744,
	21826, 5477, 10167, 16705, 26897, 8839, 30947, 27978, 27283, 24685, 32298, 3525, 12398, 28726, 9475, 10208,
	617, 13467, 22287, 2376, 6097, 26312, 2974, 9114, 21787, 28010, 4725, 15387, 3274, 10762, 31695, 17320,
	18324, 12441, 16801, 27376, 22464, 7500, 5666, 18144, 15314, 31914, 31627, 6495, 5226, 31203, 2331, 4668,
	12650, 18275, 351, 7268, 31319, 30119, 7600, 2905, 13826, 11343, 13053, 15583, 30055, 31093, 5067, 761,
	9685, 11070, 21369, 27155, 3663, 26542, 20169, 12161, 15411, 30401, 7580, 31784, 8985, 29367, 20989, 14203,
	29694, 21167, 10337, 1706, 28578, 887, 3373, 19477, 14382, 675, 7033, 15111, 26138, 12252, 30996, 21409,
	25678, 18555, 13256, 23316, 22407, 16727, 991, 9236, 5373, 29402, 6117, 15241, 27715, 19291, 19888, 19847
};

unsigned int U_Random( void ) 
{ 
	glSeed *= 69069; 
	glSeed += seed_table[ glSeed & 0xff ];
 
	return ( ++glSeed & 0x0fffffff ); 
} 

void U_Srand( unsigned int seed )
{
	glSeed = seed_table[ seed & 0xff ];
}

/*
=====================
UTIL_SharedRandomLong
=====================
*/
int UTIL_SharedRandomLong( unsigned int seed, int low, int high )
{
	unsigned int range;

	U_Srand( (int)seed + low + high );

	range = high - low + 1;
	if ( !(range - 1) )
	{
		return low;
	}
	else
	{
		int offset;
		int rnum;

		rnum = U_Random();

		offset = rnum % range;

		return (low + offset);
	}
}

/*
=====================
UTIL_SharedRandomFloat
=====================
*/
float UTIL_SharedRandomFloat( unsigned int seed, float low, float high )
{
	//
	unsigned int range;

	U_Srand( (int)seed + *(int *)&low + *(int *)&high );

	U_Random();
	U_Random();

	range = high - low;
	if ( !range )
	{
		return low;
	}
	else
	{
		int tensixrand;
		float offset;

		tensixrand = U_Random() & 65535;

		offset = (float)tensixrand / 65536.0;

		return (low + offset * range );
	}
}

void UTIL_ParametricRocket( entvars_t *pev, Vector vecOrigin, Vector vecAngles, edict_t *owner )
{	
	pev->startpos = vecOrigin;
	// Trace out line to end pos
	TraceResult tr;
	UTIL_MakeVectors( vecAngles );
	UTIL_TraceLine( pev->startpos, pev->startpos + gpGlobals->v_forward * 8192, ignore_monsters, owner, &tr);
	pev->endpos = tr.vecEndPos;

	// Now compute how long it will take based on current velocity
	Vector vecTravel = pev->endpos - pev->startpos;
	float travelTime = 0.0;
	if ( pev->velocity.Length() > 0 )
	{
		travelTime = vecTravel.Length() / pev->velocity.Length();
	}
	pev->starttime = gpGlobals->time;
	pev->impacttime = gpGlobals->time + travelTime;
}

int g_groupmask = 0;
int g_groupop = 0;

// Normal overrides
void UTIL_SetGroupTrace( int groupmask, int op )
{
	g_groupmask		= groupmask;
	g_groupop		= op;

	ENGINE_SETGROUPMASK( g_groupmask, g_groupop );
}

void UTIL_UnsetGroupTrace( void )
{
	g_groupmask		= 0;
	g_groupop		= 0;

	ENGINE_SETGROUPMASK( 0, 0 );
}

// Smart version, it'll clean itself up when it pops off stack
UTIL_GroupTrace::UTIL_GroupTrace( int groupmask, int op )
{
	m_oldgroupmask	= g_groupmask;
	m_oldgroupop	= g_groupop;

	g_groupmask		= groupmask;
	g_groupop		= op;

	ENGINE_SETGROUPMASK( g_groupmask, g_groupop );
}

UTIL_GroupTrace::~UTIL_GroupTrace( void )
{
	g_groupmask		=	m_oldgroupmask;
	g_groupop		=	m_oldgroupop;

	ENGINE_SETGROUPMASK( g_groupmask, g_groupop );
}


#ifdef	DEBUG
edict_t *DBG_EntOfVars( const entvars_t *pev )
{
	if (pev->pContainingEntity != NULL)
		return pev->pContainingEntity;
	ALERT(at_console, "entvars_t pContainingEntity is NULL, calling into engine");
	edict_t* pent = (*g_engfuncs.pfnFindEntityByVars)((entvars_t*)pev);
	if (pent == NULL)
		ALERT(at_console, "DAMN!  Even the engine couldn't FindEntityByVars!");
	((entvars_t *)pev)->pContainingEntity = pent;
	return pent;
}
#endif //DEBUG


#ifdef	DEBUG
	void
DBG_AssertFunction(
	BOOL		fExpr,
	const char*	szExpr,
	const char*	szFile,
	int			szLine,
	const char*	szMessage)
	{
	if (fExpr)
		return;
	char szOut[512];
	if (szMessage != NULL)
		sprintf(szOut, "ASSERT FAILED:\n %s \n(%s@%d)\n%s", szExpr, szFile, szLine, szMessage);
	else
		sprintf(szOut, "ASSERT FAILED:\n %s \n(%s@%d)", szExpr, szFile, szLine);
	ALERT(at_console, szOut);
	}
#endif	// DEBUG

// ripped this out of the engine
float	UTIL_AngleMod(float a)
{
	if (a < 0)
	{
		a = a + 360 * ((int)(a / 360) + 1);
	}
	else if (a >= 360)
	{
		a = a - 360 * ((int)(a / 360));
	}
	// a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}

float UTIL_AngleDiff( float destAngle, float srcAngle )
{
	float delta;

	delta = destAngle - srcAngle;
	if ( destAngle > srcAngle )
	{
		if ( delta >= 180 )
			delta -= 360;
	}
	else
	{
		if ( delta <= -180 )
			delta += 360;
	}
	return delta;
}

Vector UTIL_VecToAngles( const Vector &vec )
{
	float rgflVecOut[3];
	VEC_TO_ANGLES(vec, rgflVecOut);
	return Vector(rgflVecOut);
}
	
//	float UTIL_MoveToOrigin( edict_t *pent, const Vector vecGoal, float flDist, int iMoveType )
void UTIL_MoveToOrigin( edict_t *pent, const Vector &vecGoal, float flDist, int iMoveType )
{
	float rgfl[3];
	vecGoal.CopyToArray(rgfl);
//		return MOVE_TO_ORIGIN ( pent, rgfl, flDist, iMoveType ); 
	MOVE_TO_ORIGIN ( pent, rgfl, flDist, iMoveType ); 
}


int UTIL_EntitiesInBox( edict_t **pList, int listMax, const Vector &mins, const Vector &maxs, int flagMask )
{
	edict_t		*pEdict = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	int			count;

	count = 0;

	if ( !pEdict )
		return count;

	for ( int i = 1; i < gpGlobals->maxEntities; i++, pEdict++ )
	{
		if ( pEdict->free )	// Not in use
			continue;
		
		if ( flagMask && !(pEdict->v.flags & flagMask) )	// Does it meet the criteria?
			continue;

		if ( mins.x > pEdict->v.absmax.x ||
			 mins.y > pEdict->v.absmax.y ||
			 mins.z > pEdict->v.absmax.z ||
			 maxs.x < pEdict->v.absmin.x ||
			 maxs.y < pEdict->v.absmin.y ||
			 maxs.z < pEdict->v.absmin.z )
			 continue;

		pList[ count ] = pEdict;
		count++;

		if ( count >= listMax )
			return count;
	}

	return count;
}


int UTIL_MonstersInSphere( edict_t **pList, int listMax, const Vector &center, float radius )
{
	edict_t		*pEdict = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	int			count;
	float		distance, delta;

	count = 0;
	float radiusSquared = radius * radius;

	if ( !pEdict )
		return count;

	for ( int i = 1; i < gpGlobals->maxEntities; i++, pEdict++ )
	{
		if ( pEdict->free )	// Not in use
			continue;
		
		if ( !(pEdict->v.flags & (FL_CLIENT|FL_MONSTER)) )	// Not a client/monster ?
			continue;

		// Use origin for X & Y since they are centered for all monsters
		// Now X
		delta = center.x - pEdict->v.origin.x;//(pEdict->v.absmin.x + pEdict->v.absmax.x)*0.5;
		delta *= delta;

		if ( delta > radiusSquared )
			continue;
		distance = delta;
		
		// Now Y
		delta = center.y - pEdict->v.origin.y;//(pEdict->v.absmin.y + pEdict->v.absmax.y)*0.5;
		delta *= delta;

		distance += delta;
		if ( distance > radiusSquared )
			continue;

		// Now Z
		delta = center.z - (pEdict->v.absmin.z + pEdict->v.absmax.z)*0.5;
		delta *= delta;

		distance += delta;
		if ( distance > radiusSquared )
			continue;

		pList[ count ] = pEdict;
		count++;

		if ( count >= listMax )
			return count;
	}


	return count;
}


edict_t *UTIL_FindEntityInSphere( edict_t *pStartEntity, const Vector &vecCenter, float flRadius )
{
	edict_t	*pentEntity;

	if (pStartEntity)
		pentEntity = pStartEntity;
	else
		pentEntity = NULL;

	pentEntity = FIND_ENTITY_IN_SPHERE( pentEntity, vecCenter, flRadius);

   if (!FNullEnt(pentEntity))
      return pentEntity;

	return NULL;
}


edict_t *UTIL_FindEntityByString( edict_t *pStartEntity, const char *szKeyword, const char *szValue )
{
	edict_t	*pentEntity;

	if (pStartEntity)
		pentEntity = pStartEntity;
	else
		pentEntity = NULL;

	pentEntity = FIND_ENTITY_BY_STRING( pentEntity, szKeyword, szValue );

   if (!FNullEnt(pentEntity))
      return pentEntity;

	return NULL;
}

edict_t *UTIL_FindEntityByClassname( edict_t *pStartEntity, const char *szName )
{
	return UTIL_FindEntityByString( pStartEntity, "classname", szName );
}

edict_t *UTIL_FindEntityByTargetname( edict_t *pStartEntity, const char *szName )
{
	return UTIL_FindEntityByString( pStartEntity, "targetname", szName );
}


edict_t *UTIL_FindEntityGeneric( const char *szWhatever, Vector &vecSrc, float flRadius )
{
	edict_t *pEntity = NULL;

	pEntity = UTIL_FindEntityByTargetname( NULL, szWhatever );
	if (pEntity)
		return pEntity;

	edict_t *pSearch = NULL;
	float flMaxDist2 = flRadius * flRadius;
	while ((pSearch = UTIL_FindEntityByClassname( pSearch, szWhatever )) != NULL)
	{
		float flDist2 = (pSearch->v.origin - vecSrc).Length();
		flDist2 = flDist2 * flDist2;
		if (flMaxDist2 > flDist2)
		{
			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}
	return pEntity;
}


// returns a edict_t pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns NULL
// Index is 1 based
edict_t	*UTIL_PlayerByIndex( int playerIndex )
{
	edict_t *pPlayer = NULL;

	if ( playerIndex > 0 && playerIndex <= gpGlobals->maxClients )
	{
		edict_t *pPlayerEdict = INDEXENT( playerIndex );
		if ( pPlayerEdict && !pPlayerEdict->free )
		{
			pPlayer = pPlayerEdict;
		}
	}
	
	return pPlayer;
}


void UTIL_MakeVectors( const Vector &vecAngles )
{
	MAKE_VECTORS( vecAngles );
}


void UTIL_MakeAimVectors( const Vector &vecAngles )
{
	float rgflVec[3];
	vecAngles.CopyToArray(rgflVec);
	rgflVec[0] = -rgflVec[0];
	MAKE_VECTORS(rgflVec);
}


#define SWAP(a,b,temp)	((temp)=(a),(a)=(b),(b)=(temp))

void UTIL_MakeInvVectors( const Vector &vec, globalvars_t *pgv )
{
	MAKE_VECTORS(vec);

	float tmp;
	pgv->v_right = pgv->v_right * -1;

	SWAP(pgv->v_forward.y, pgv->v_right.x, tmp);
	SWAP(pgv->v_forward.z, pgv->v_up.x, tmp);
	SWAP(pgv->v_right.z, pgv->v_up.y, tmp);
}


void UTIL_EmitAmbientSound( edict_t *entity, const Vector &vecOrigin, const char *samp, float vol, float attenuation, int fFlags, int pitch )
{
	float rgfl[3];
	vecOrigin.CopyToArray(rgfl);

/*jlb
	if (samp && *samp == '!')
	{
		char name[32];
		if (SENTENCEG_Lookup(samp, name) >= 0)
			EMIT_AMBIENT_SOUND(entity, rgfl, name, vol, attenuation, fFlags, pitch);
	}
	else
jlb*/
		EMIT_AMBIENT_SOUND(entity, rgfl, samp, vol, attenuation, fFlags, pitch);
}

static unsigned short FixedUnsigned16( float value, float scale )
{
	int output;

	output = value * scale;
	if ( output < 0 )
		output = 0;
	if ( output > 0xFFFF )
		output = 0xFFFF;

	return (unsigned short)output;
}

static short FixedSigned16( float value, float scale )
{
	int output;

	output = value * scale;

	if ( output > 32767 )
		output = 32767;

	if ( output < -32768 )
		output = -32768;

	return (short)output;
}

// Shake the screen of all clients within radius
// radius == 0, shake all clients
// UNDONE: Allow caller to shake clients not ONGROUND?
// UNDONE: Fix falloff model (disabled)?
// UNDONE: Affect user controls?

int gmsgShake = 0;

void UTIL_ScreenShake( const Vector &center, float amplitude, float frequency, float duration, float radius )
{
	int			i;
	float		localAmplitude;
	ScreenShake	shake;

	shake.duration = FixedUnsigned16( duration, 1<<12 );		// 4.12 fixed
	shake.frequency = FixedUnsigned16( frequency, 1<<8 );	// 8.8 fixed

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		edict_t *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer || !(pPlayer->v.flags & FL_ONGROUND) )	// Don't shake if not onground
			continue;

		localAmplitude = 0;

		if ( radius <= 0 )
			localAmplitude = amplitude;
		else
		{
			Vector delta = center - pPlayer->v.origin;
			float distance = delta.Length();
	
			// Had to get rid of this falloff - it didn't work well
			if ( distance < radius )
				localAmplitude = amplitude;//radius - distance;
		}
		if ( localAmplitude )
		{
			shake.amplitude = FixedUnsigned16( localAmplitude, 1<<12 );		// 4.12 fixed
			
			if (gmsgShake == 0)
				gmsgShake = REG_USER_MSG("ScreenShake", sizeof(ScreenShake));

			MESSAGE_BEGIN( MSG_ONE, gmsgShake, NULL, pPlayer );		// use the magic #1 for "one client"
				
				WRITE_SHORT( shake.amplitude );				// shake amount
				WRITE_SHORT( shake.duration );				// shake lasts this long
				WRITE_SHORT( shake.frequency );				// shake noise frequency

			MESSAGE_END();
		}
	}
}



void UTIL_ScreenShakeAll( const Vector &center, float amplitude, float frequency, float duration )
{
	UTIL_ScreenShake( center, amplitude, frequency, duration, 0 );
}


void UTIL_ScreenFadeBuild( ScreenFade &fade, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags )
{
	fade.duration = FixedUnsigned16( fadeTime, 1<<12 );		// 4.12 fixed
	fade.holdTime = FixedUnsigned16( fadeHold, 1<<12 );		// 4.12 fixed
	fade.r = (int)color.x;
	fade.g = (int)color.y;
	fade.b = (int)color.z;
	fade.a = alpha;
	fade.fadeFlags = flags;
}


int gmsgFade = 0;

void UTIL_ScreenFadeWrite( const ScreenFade &fade, edict_t *pEntity )
{
	if ( !pEntity || !(pEntity->v.flags & FL_CLIENT) )
		return;

	if (gmsgFade == 0)
		gmsgFade = REG_USER_MSG("ScreenFade", sizeof(ScreenFade));

	MESSAGE_BEGIN( MSG_ONE, gmsgFade, NULL, pEntity );		// use the magic #1 for "one client"
		
		WRITE_SHORT( fade.duration );		// fade lasts this long
		WRITE_SHORT( fade.holdTime );		// fade lasts this long
		WRITE_SHORT( fade.fadeFlags );		// fade type (in / out)
		WRITE_BYTE( fade.r );				// fade red
		WRITE_BYTE( fade.g );				// fade green
		WRITE_BYTE( fade.b );				// fade blue
		WRITE_BYTE( fade.a );				// fade blue

	MESSAGE_END();
}


void UTIL_ScreenFadeAll( const Vector &color, float fadeTime, float fadeHold, int alpha, int flags )
{
	int			i;
	ScreenFade	fade;


	UTIL_ScreenFadeBuild( fade, color, fadeTime, fadeHold, alpha, flags );

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		edict_t *pPlayer = UTIL_PlayerByIndex( i );
	
		UTIL_ScreenFadeWrite( fade, pPlayer );
	}
}


void UTIL_ScreenFade( edict_t *pEntity, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags )
{
	ScreenFade	fade;

	UTIL_ScreenFadeBuild( fade, color, fadeTime, fadeHold, alpha, flags );
	UTIL_ScreenFadeWrite( fade, pEntity );
}


void UTIL_HudMessage( edict_t *pEntity, const hudtextparms_t &textparms, const char *pMessage )
{
	if ( !pEntity || !(pEntity->v.flags & FL_CLIENT) )
		return;

	MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pEntity );
		WRITE_BYTE( TE_TEXTMESSAGE );
		WRITE_BYTE( textparms.channel & 0xFF );

		WRITE_SHORT( FixedSigned16( textparms.x, 1<<13 ) );
		WRITE_SHORT( FixedSigned16( textparms.y, 1<<13 ) );
		WRITE_BYTE( textparms.effect );

		WRITE_BYTE( textparms.r1 );
		WRITE_BYTE( textparms.g1 );
		WRITE_BYTE( textparms.b1 );
		WRITE_BYTE( textparms.a1 );

		WRITE_BYTE( textparms.r2 );
		WRITE_BYTE( textparms.g2 );
		WRITE_BYTE( textparms.b2 );
		WRITE_BYTE( textparms.a2 );

		WRITE_SHORT( FixedUnsigned16( textparms.fadeinTime, 1<<8 ) );
		WRITE_SHORT( FixedUnsigned16( textparms.fadeoutTime, 1<<8 ) );
		WRITE_SHORT( FixedUnsigned16( textparms.holdTime, 1<<8 ) );

		if ( textparms.effect == 2 )
			WRITE_SHORT( FixedUnsigned16( textparms.fxTime, 1<<8 ) );
		
		if ( strlen( pMessage ) < 512 )
		{
			WRITE_STRING( pMessage );
		}
		else
		{
			char tmp[512];
			strncpy( tmp, pMessage, 511 );
			tmp[511] = 0;
			WRITE_STRING( tmp );
		}
	MESSAGE_END();
}

void UTIL_HudMessageAll( const hudtextparms_t &textparms, const char *pMessage )
{
	int			i;

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		edict_t *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
			UTIL_HudMessage( pPlayer, textparms, pMessage );
	}
}


int gmsgTextMsg = 0;
int gmsgSayText = 0;

void UTIL_ClientPrintAll( int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	if (gmsgTextMsg == 0)
		gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );

	MESSAGE_BEGIN( MSG_ALL, gmsgTextMsg );
		WRITE_BYTE( msg_dest );
		WRITE_STRING( msg_name );

		if ( param1 )
			WRITE_STRING( param1 );
		if ( param2 )
			WRITE_STRING( param2 );
		if ( param3 )
			WRITE_STRING( param3 );
		if ( param4 )
			WRITE_STRING( param4 );

	MESSAGE_END();
}

void ClientPrint( entvars_t *client, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4 )
{
	if (gmsgTextMsg == 0)
		gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );

	MESSAGE_BEGIN( MSG_ONE, gmsgTextMsg, NULL, client );
		WRITE_BYTE( msg_dest );
		WRITE_STRING( msg_name );

		if ( param1 )
			WRITE_STRING( param1 );
		if ( param2 )
			WRITE_STRING( param2 );
		if ( param3 )
			WRITE_STRING( param3 );
		if ( param4 )
			WRITE_STRING( param4 );

	MESSAGE_END();
}

void UTIL_SayText( const char *pText, edict_t *pEntity )
{
	if ( !(pEntity->v.flags & FL_CLIENT) )
		return;

	if (gmsgSayText == 0)
		gmsgSayText = REG_USER_MSG( "SayText", -1 );

	MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pEntity );
		WRITE_BYTE( ENTINDEX(pEntity) );
		WRITE_STRING( pText );
	MESSAGE_END();
}

void UTIL_SayTextAll( const char *pText, edict_t *pEntity )
{
	if (gmsgSayText == 0)
		gmsgSayText = REG_USER_MSG( "SayText", -1 );

	MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
		WRITE_BYTE( ENTINDEX(pEntity) );
		WRITE_STRING( pText );
	MESSAGE_END();
}


char *UTIL_dtos1( int d )
{
	static char buf[8];
	sprintf( buf, "%d", d );
	return buf;
}

char *UTIL_dtos2( int d )
{
	static char buf[8];
	sprintf( buf, "%d", d );
	return buf;
}

char *UTIL_dtos3( int d )
{
	static char buf[8];
	sprintf( buf, "%d", d );
	return buf;
}

char *UTIL_dtos4( int d )
{
	static char buf[8];
	sprintf( buf, "%d", d );
	return buf;
}

int gmsgHudText = 0;

void UTIL_ShowMessage( const char *pString, edict_t *pEntity )
{
	if ( !pEntity || !(pEntity->v.flags & FL_CLIENT) )
		return;

	if (gmsgHudText == 0)
		gmsgHudText = REG_USER_MSG( "HudText", -1 );

	MESSAGE_BEGIN( MSG_ONE, gmsgHudText, NULL, pEntity );
	WRITE_STRING( pString );
	MESSAGE_END();
}


void UTIL_ShowMessageAll( const char *pString )
{
	int		i;

	// loop through all players

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		edict_t *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
			UTIL_ShowMessage( pString, pPlayer );
	}
}

// Overloaded to add IGNORE_GLASS
void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr )
{
	TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass?0x100:0), pentIgnore, ptr );
}


void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
	TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr );
}


void UTIL_TraceHull( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr )
{
	TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), hullNumber, pentIgnore, ptr );
}

void UTIL_TraceModel( const Vector &vecStart, const Vector &vecEnd, int hullNumber, edict_t *pentModel, TraceResult *ptr )
{
	g_engfuncs.pfnTraceModel( vecStart, vecEnd, hullNumber, pentModel, ptr );
}


TraceResult UTIL_GetGlobalTrace( )
{
	TraceResult tr;

	tr.fAllSolid		= gpGlobals->trace_allsolid;
	tr.fStartSolid		= gpGlobals->trace_startsolid;
	tr.fInOpen			= gpGlobals->trace_inopen;
	tr.fInWater			= gpGlobals->trace_inwater;
	tr.flFraction		= gpGlobals->trace_fraction;
	tr.flPlaneDist		= gpGlobals->trace_plane_dist;
	tr.pHit			= gpGlobals->trace_ent;
	tr.vecEndPos		= gpGlobals->trace_endpos;
	tr.vecPlaneNormal	= gpGlobals->trace_plane_normal;
	tr.iHitgroup		= gpGlobals->trace_hitgroup;
	return tr;
}

	
void UTIL_SetSize( entvars_t *pev, const Vector &vecMin, const Vector &vecMax )
{
	SET_SIZE( ENT(pev), vecMin, vecMax );
}
	
	
float UTIL_VecToYaw( const Vector &vec )
{
	return VEC_TO_YAW(vec);
}


void UTIL_SetOrigin( entvars_t *pev, const Vector &vecOrigin )
{
	SET_ORIGIN(ENT(pev), vecOrigin );
}

void UTIL_ParticleEffect( const Vector &vecOrigin, const Vector &vecDirection, ULONG ulColor, ULONG ulCount )
{
	PARTICLE_EFFECT( vecOrigin, vecDirection, (float)ulColor, (float)ulCount );
}


float UTIL_Approach( float target, float value, float speed )
{
	float delta = target - value;

	if ( delta > speed )
		value += speed;
	else if ( delta < -speed )
		value -= speed;
	else 
		value = target;

	return value;
}


float UTIL_ApproachAngle( float target, float value, float speed )
{
	target = UTIL_AngleMod( target );
	value = UTIL_AngleMod( target );
	
	float delta = target - value;

	// Speed is assumed to be positive
	if ( speed < 0 )
		speed = -speed;

	if ( delta < -180 )
		delta += 360;
	else if ( delta > 180 )
		delta -= 360;

	if ( delta > speed )
		value += speed;
	else if ( delta < -speed )
		value -= speed;
	else 
		value = target;

	return value;
}


float UTIL_AngleDistance( float next, float cur )
{
	float delta = next - cur;

	if ( delta < -180 )
		delta += 360;
	else if ( delta > 180 )
		delta -= 360;

	return delta;
}


float UTIL_SplineFraction( float value, float scale )
{
	value = scale * value;
	float valueSquared = value * value;

	// Nice little ease-in, ease-out spline-like curve
	return 3 * valueSquared - 2 * valueSquared * value;
}


char* UTIL_VarArgs( char *format, ... )
{
	va_list		argptr;
	static char		string[1024];
	
	va_start (argptr, format);
	vsprintf (string, format,argptr);
	va_end (argptr);

	return string;	
}
	
Vector UTIL_GetAimVector( edict_t *pent, float flSpeed )
{
	Vector tmp;
	GET_AIM_VECTOR(pent, flSpeed, tmp);
	return tmp;
}

BOOL UTIL_ShouldShowBlood( int color )
{
	if ( color != DONT_BLEED )
	{
		if ( color == BLOOD_COLOR_RED )
		{
			if ( CVAR_GET_FLOAT("violence_hblood") != 0 )
				return TRUE;
		}
		else
		{
			if ( CVAR_GET_FLOAT("violence_ablood") != 0 )
				return TRUE;
		}
	}
	return FALSE;
}

int UTIL_PointContents(	const Vector &vec )
{
	return POINT_CONTENTS(vec);
}

void UTIL_BloodStream( const Vector &origin, const Vector &direction, int color, int amount )
{
	if ( !UTIL_ShouldShowBlood( color ) )
		return;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, origin );
		WRITE_BYTE( TE_BLOODSTREAM );
		WRITE_COORD( origin.x );
		WRITE_COORD( origin.y );
		WRITE_COORD( origin.z );
		WRITE_COORD( direction.x );
		WRITE_COORD( direction.y );
		WRITE_COORD( direction.z );
		WRITE_BYTE( color );
		WRITE_BYTE( min( amount, 255 ) );
	MESSAGE_END();
}				


void UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount )
{
	if ( !UTIL_ShouldShowBlood( color ) )
		return;

	if ( color == DONT_BLEED || amount == 0 )
		return;

	// scale up blood effect in multiplayer for better visibility
	amount *= 2;

	if ( amount > 255 )
		amount = 255;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, origin );
		WRITE_BYTE( TE_BLOODSPRITE );
		WRITE_COORD( origin.x);								// pos
		WRITE_COORD( origin.y);
		WRITE_COORD( origin.z);
		WRITE_SHORT( g_sModelIndexBloodSpray );				// initial sprite model
		WRITE_SHORT( g_sModelIndexBloodDrop );				// droplet sprite models
		WRITE_BYTE( color );								// color index into host_basepal
		WRITE_BYTE( min( max( 3, amount / 10 ), 16 ) );		// size
	MESSAGE_END();
}				

Vector UTIL_RandomBloodVector( void )
{
	Vector direction;

	direction.x = RANDOM_FLOAT ( -1, 1 );
	direction.y = RANDOM_FLOAT ( -1, 1 );
	direction.z = RANDOM_FLOAT ( 0, 1 );

	return direction;
}


void UTIL_BloodDecalTrace( TraceResult *pTrace, int bloodColor )
{
	if ( UTIL_ShouldShowBlood( bloodColor ) )
	{
		if ( bloodColor == BLOOD_COLOR_RED )
			UTIL_DecalTrace( pTrace, DECAL_BLOOD1 + RANDOM_LONG(0,5) );
		else
			UTIL_DecalTrace( pTrace, DECAL_YBLOOD1 + RANDOM_LONG(0,5) );
	}
}


bool IsBSPModel(edict_t *pEdict)
{
   return ((pEdict->v.solid == SOLID_BSP) || (pEdict->v.movetype == MOVETYPE_PUSHSTEP));
}


void UTIL_DecalTrace( TraceResult *pTrace, int decalNumber )
{
	short entityIndex;
	int index;
	int message;

	if ( decalNumber < 0 )
		return;

	index = gDecals[ decalNumber ].index;

	if ( index < 0 )
		return;

	if (pTrace->flFraction == 1.0)
		return;

	// Only decal BSP models
	if ( pTrace->pHit )
	{
		edict_t *pEntity = pTrace->pHit;
		if ( pEntity && !IsBSPModel(pEntity) )
			return;
		entityIndex = ENTINDEX( pTrace->pHit );
	}
	else 
		entityIndex = 0;

	message = TE_DECAL;
	if ( entityIndex != 0 )
	{
		if ( index > 255 )
		{
			message = TE_DECALHIGH;
			index -= 256;
		}
	}
	else
	{
		message = TE_WORLDDECAL;
		if ( index > 255 )
		{
			message = TE_WORLDDECALHIGH;
			index -= 256;
		}
	}
	
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( message );
		WRITE_COORD( pTrace->vecEndPos.x );
		WRITE_COORD( pTrace->vecEndPos.y );
		WRITE_COORD( pTrace->vecEndPos.z );
		WRITE_BYTE( index );
		if ( entityIndex )
			WRITE_SHORT( entityIndex );
	MESSAGE_END();
}

/*
==============
UTIL_PlayerDecalTrace

A player is trying to apply his custom decal for the spray can.
Tell connected clients to display it, or use the default spray can decal
if the custom can't be loaded.
==============
*/
void UTIL_PlayerDecalTrace( TraceResult *pTrace, int playernum, int decalNumber, BOOL bIsCustom )
{
	int index;
	
	if (!bIsCustom)
	{
		if ( decalNumber < 0 )
			return;

		index = gDecals[ decalNumber ].index;
		if ( index < 0 )
			return;
	}
	else
		index = decalNumber;

	if (pTrace->flFraction == 1.0)
		return;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_PLAYERDECAL );
		WRITE_BYTE ( playernum );
		WRITE_COORD( pTrace->vecEndPos.x );
		WRITE_COORD( pTrace->vecEndPos.y );
		WRITE_COORD( pTrace->vecEndPos.z );
		WRITE_SHORT( (short)ENTINDEX(pTrace->pHit) );
		WRITE_BYTE( index );
	MESSAGE_END();
}

void UTIL_GunshotDecalTrace( TraceResult *pTrace, int decalNumber )
{
	if ( decalNumber < 0 )
		return;

	int index = gDecals[ decalNumber ].index;
	if ( index < 0 )
		return;

	if (pTrace->flFraction == 1.0)
		return;

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pTrace->vecEndPos );
		WRITE_BYTE( TE_GUNSHOTDECAL );
		WRITE_COORD( pTrace->vecEndPos.x );
		WRITE_COORD( pTrace->vecEndPos.y );
		WRITE_COORD( pTrace->vecEndPos.z );
		WRITE_SHORT( (short)ENTINDEX(pTrace->pHit) );
		WRITE_BYTE( index );
	MESSAGE_END();
}


void UTIL_Sparks( const Vector &position )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, position );
		WRITE_BYTE( TE_SPARKS );
		WRITE_COORD( position.x );
		WRITE_COORD( position.y );
		WRITE_COORD( position.z );
	MESSAGE_END();
}


void UTIL_Ricochet( const Vector &position, float scale )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, position );
		WRITE_BYTE( TE_ARMOR_RICOCHET );
		WRITE_COORD( position.x );
		WRITE_COORD( position.y );
		WRITE_COORD( position.z );
		WRITE_BYTE( (int)(scale*10) );
	MESSAGE_END();
}


void UTIL_StringToVector( float *pVector, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;

	strcpy( tempString, pString );
	pstr = pfront = tempString;

	for ( j = 0; j < 3; j++ )			// lifted from pr_edict.c
	{
		pVector[j] = atof( pfront );

		while ( *pstr && *pstr != ' ' )
			pstr++;
		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}
	if (j < 2)
	{
		/*
		ALERT( at_error, "Bad field in entity!! %s:%s == \"%s\"\n",
			pkvd->szClassName, pkvd->szKeyName, pkvd->szValue );
		*/
		for (j = j+1;j < 3; j++)
			pVector[j] = 0;
	}
}


void UTIL_StringToIntArray( int *pVector, int count, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;

	strcpy( tempString, pString );
	pstr = pfront = tempString;

	for ( j = 0; j < count; j++ )			// lifted from pr_edict.c
	{
		pVector[j] = atoi( pfront );

		while ( *pstr && *pstr != ' ' )
			pstr++;
		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}

	for ( j++; j < count; j++ )
	{
		pVector[j] = 0;
	}
}

Vector UTIL_ClampVectorToBox( const Vector &input, const Vector &clampSize )
{
	Vector sourceVector = input;

	if ( sourceVector.x > clampSize.x )
		sourceVector.x -= clampSize.x;
	else if ( sourceVector.x < -clampSize.x )
		sourceVector.x += clampSize.x;
	else
		sourceVector.x = 0;

	if ( sourceVector.y > clampSize.y )
		sourceVector.y -= clampSize.y;
	else if ( sourceVector.y < -clampSize.y )
		sourceVector.y += clampSize.y;
	else
		sourceVector.y = 0;
	
	if ( sourceVector.z > clampSize.z )
		sourceVector.z -= clampSize.z;
	else if ( sourceVector.z < -clampSize.z )
		sourceVector.z += clampSize.z;
	else
		sourceVector.z = 0;

	return sourceVector.Normalize();
}


float UTIL_WaterLevel( const Vector &position, float minz, float maxz )
{
	Vector midUp = position;
	midUp.z = minz;

	if (UTIL_PointContents(midUp) != CONTENTS_WATER)
		return minz;

	midUp.z = maxz;
	if (UTIL_PointContents(midUp) == CONTENTS_WATER)
		return maxz;

	float diff = maxz - minz;
	while (diff > 1.0)
	{
		midUp.z = minz + diff/2.0;
		if (UTIL_PointContents(midUp) == CONTENTS_WATER)
		{
			minz = midUp.z;
		}
		else
		{
			maxz = midUp.z;
		}
		diff = maxz - minz;
	}

	return midUp.z;
}


void UTIL_Bubbles( Vector mins, Vector maxs, int count )
{
	Vector mid =  (mins + maxs) * 0.5;

	float flHeight = UTIL_WaterLevel( mid,  mid.z, mid.z + 1024 );
	flHeight = flHeight - mins.z;

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, mid );
		WRITE_BYTE( TE_BUBBLES );
		WRITE_COORD( mins.x );	// mins
		WRITE_COORD( mins.y );
		WRITE_COORD( mins.z );
		WRITE_COORD( maxs.x );	// maxz
		WRITE_COORD( maxs.y );
		WRITE_COORD( maxs.z );
		WRITE_COORD( flHeight );			// height
		WRITE_SHORT( g_sModelIndexBubbles );
		WRITE_BYTE( count ); // count
		WRITE_COORD( 8 ); // speed
	MESSAGE_END();
}

void UTIL_BubbleTrail( Vector from, Vector to, int count )
{
	float flHeight = UTIL_WaterLevel( from,  from.z, from.z + 256 );
	flHeight = flHeight - from.z;

	if (flHeight < 8)
	{
		flHeight = UTIL_WaterLevel( to,  to.z, to.z + 256 );
		flHeight = flHeight - to.z;
		if (flHeight < 8)
			return;

		// UNDONE: do a ploink sound
		flHeight = flHeight + to.z - from.z;
	}

	if (count > 255) 
		count = 255;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BUBBLETRAIL );
		WRITE_COORD( from.x );	// mins
		WRITE_COORD( from.y );
		WRITE_COORD( from.z );
		WRITE_COORD( to.x );	// maxz
		WRITE_COORD( to.y );
		WRITE_COORD( to.z );
		WRITE_COORD( flHeight );			// height
		WRITE_SHORT( g_sModelIndexBubbles );
		WRITE_BYTE( count ); // count
		WRITE_COORD( 8 ); // speed
	MESSAGE_END();
}


void UTIL_Remove( edict_t *pEntity )
{
	if ( !pEntity )
		return;

//jlb	pEntity->UpdateOnRemove();
	pEntity->v.flags |= FL_KILLME;
	pEntity->v.targetname = 0;
}


BOOL UTIL_IsValidEntity( edict_t *pent )
{
	if ( !pent || pent->free || (pent->v.flags & FL_KILLME) )
		return FALSE;
	return TRUE;
}


//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void UTIL_LogPrintf( char *fmt, ... )
{
	va_list			argptr;
	static char		string[1024];
	
	va_start ( argptr, fmt );
	vsprintf ( string, fmt, argptr );
	va_end   ( argptr );

	// Print to server console
	ALERT( at_logged, "%s", string );
}

//=========================================================
// UTIL_DotPoints - returns the dot product of a line from
// src to check and vecdir.
//=========================================================
float UTIL_DotPoints ( const Vector &vecSrc, const Vector &vecCheck, const Vector &vecDir )
{
	Vector2D	vec2LOS;

	vec2LOS = ( vecCheck - vecSrc ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	return DotProduct (vec2LOS , ( vecDir.Make2D() ) );
}


//=========================================================
// UTIL_StripToken - for redundant keynames
//=========================================================
void UTIL_StripToken( const char *pKey, char *pDest )
{
	int i = 0;

	while ( pKey[i] && pKey[i] != '#' )
	{
		pDest[i] = pKey[i];
		i++;
	}
	pDest[i] = 0;
}


Vector VecBModelOrigin( entvars_t* pevBModel )
{
	return pevBModel->absmin + ( pevBModel->size * 0.5 );
}


bool UTIL_IsAlive(entvars_t *pev)
{
   return ((pev->deadflag == DEAD_NO) && (pev->health > 0) &&
           ((pev->flags & FL_NOTARGET) == 0) && (pev->takedamage != 0));
}


bool UTIL_IsAlive(edict_t *pEdict)
{
   return ((pEdict->v.deadflag == DEAD_NO) && (pEdict->v.health > 0) &&
            ((pEdict->v.flags & FL_NOTARGET) == 0) && (pEdict->v.takedamage != 0));
}


bool UTIL_IsPlayer(edict_t *pEdict)
{
   return ((pEdict->v.flags & FL_CLIENT) == FL_CLIENT);
}


Vector UTIL_BodyTarget(edict_t *pEdict, Vector posSrc)
{
	if (pEdict->v.flags & FL_CLIENT)
		return pEdict->v.origin + (pEdict->v.view_ofs * RANDOM_FLOAT(0.5, 1.1));
	else
		return (pEdict->v.origin + ((pEdict->v.mins + pEdict->v.maxs) * 0.5));
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target
//=========================================================
bool UTIL_FVisible ( edict_t *pEntity, edict_t *pLooker )
{
	TraceResult tr;
	Vector		vecLookerOrigin;
	Vector		vecTargetOrigin;
	
	if (FBitSet( pEntity->v.flags, FL_NOTARGET ))
		return FALSE;

	// don't look through water
	if ((pLooker->v.waterlevel != 3 && pEntity->v.waterlevel == 3) 
		|| (pLooker->v.waterlevel == 3 && pEntity->v.waterlevel == 0))
		return FALSE;

	vecLookerOrigin = pLooker->v.origin + pLooker->v.view_ofs;//look through the caller's 'eyes'
	vecTargetOrigin = pEntity->v.origin + pEntity->v.view_ofs;

	UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, pLooker, &tr);
	
	if (tr.flFraction != 1.0)
	{
		return FALSE;// Line of sight is not established
	}
	else
	{
		return TRUE;// line of sight is valid.
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target vector
//=========================================================
bool UTIL_FVisible ( const Vector &vecOrigin, edict_t *pLooker )
{
	TraceResult tr;
	Vector		vecLookerOrigin;
	
	vecLookerOrigin = pLooker->v.origin + pLooker->v.view_ofs;

	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, pLooker, &tr);
	
	if (tr.flFraction != 1.0)
	{
		return FALSE;// Line of sight is not established
	}
	else
	{
		return TRUE;// line of sight is valid.
	}
}


//=========================================================
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
bool UTIL_FInViewCone ( edict_t *pEntity, edict_t *pLooker, float fov )
{
	Vector2D	vec2LOS;
	float	flDot;

	UTIL_MakeVectors ( pLooker->v.angles );
	
	vec2LOS = ( pEntity->v.origin - pLooker->v.origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

	if ( flDot > fov )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// FInViewCone - returns true is the passed vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
bool UTIL_FInViewCone ( Vector *pOrigin, edict_t *pLooker, float fov )
{
	Vector2D	vec2LOS;
	float		flDot;

	UTIL_MakeVectors ( pLooker->v.angles );
	
	vec2LOS = ( *pOrigin - pLooker->v.origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

	if ( flDot > fov )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


#define ARMOR_RATIO	 0.2	// Armor Takes 80% of the damage
#define ARMOR_BONUS  0.5	// Each Point of Armor is work 1/x points of health

int gmsgDamage = 0;

int UTIL_TakeDamage( edict_t *pEdict, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
   // note: This function should ONLY be used for players!!!

	int bitsDamage = bitsDamageType;
	int ffound = TRUE;
	int fTookDamage;
	float flRatio;
	float flBonus;
	float flHealthPrev = pEdict->v.health;

	flBonus = ARMOR_BONUS;
	flRatio = ARMOR_RATIO;

	if (!pEdict->v.takedamage)
		return 0;

	if ( ( bitsDamageType & DMG_BLAST ) )
	{
		// blasts damage armor more.
		flBonus *= 2;
	}

	// Already dead
	if ( !UTIL_IsAlive(pEdict) )
		return 0;

	// Armor. 
	if (pEdict->v.armorvalue && !(bitsDamageType & (DMG_FALL | DMG_DROWN)) )// armor doesn't protect against fall or drown damage!
	{
		float flNew = flDamage * flRatio;

		float flArmor;

		flArmor = (flDamage - flNew) * flBonus;

		// Does this use more armor than we have?
		if (flArmor > pEdict->v.armorvalue)
		{
			flArmor = pEdict->v.armorvalue;
			flArmor *= (1/flBonus);
			flNew = flDamage - flArmor;
			pEdict->v.armorvalue = 0;
		}
		else
			pEdict->v.armorvalue -= flArmor;
		
		flDamage = flNew;
	}

	float	flTake;
	Vector	vecDir;

	if ( pEdict->v.deadflag == DEAD_NO )
	{
		// no pain sound during death animation.
//jlb		PainSound();// "Ouch!"
	}

	// WARNING the cast to INT is critical!!!  The player could wind up with 0.5 health
	flTake = (int)flDamage;

	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	vecDir = Vector( 0, 0, 0 );

	if (!FNullEnt( pevInflictor ))
	{
		edict_t *pInflictor = ENT(pevInflictor);

		if (pInflictor)
		{
			vecDir = ( UTIL_Center(pInflictor) - Vector ( 0, 0, 10 ) - UTIL_Center(pEdict) ).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

	fTookDamage = flTake;

	// if this is a player, move him around!
	if ( ( !FNullEnt( pevInflictor ) ) && (pEdict->v.movetype == MOVETYPE_WALK) && (!pevAttacker || pevAttacker->solid != SOLID_TRIGGER) )
	{
		float force = flDamage * ((32 * 32 * 72.0) / (pEdict->v.size.x * pEdict->v.size.y * pEdict->v.size.z)) * 5;
	
		if ( force > 1000.0) 
			force = 1000.0;

		pEdict->v.velocity = pEdict->v.velocity + vecDir * -force;
	}

	// do the damage
	pEdict->v.health -= flTake;
	
	// store entity that hurt this player
	pEdict->v.dmg_inflictor = ENT(pevAttacker);
	
	if ( pEdict->v.health <= 0 )
	{
		pEdict->v.health = 1;  // can't suicide if already dead!
		gpGamedllFuncs->dllapi_table->pfnClientKill(pEdict);
		
		// Add 1 score to the monster that killed this player
		if ( pevAttacker->flags & FL_MONSTER )
			pevAttacker->frags += 1.0;
	}

	// tell director about it
	MESSAGE_BEGIN( MSG_SPEC, SVC_HLTV );
		WRITE_BYTE ( DRC_EVENT );	// take damage event
		WRITE_SHORT( ENTINDEX(pEdict) );	// index number of primary entity
		WRITE_SHORT( ENTINDEX(ENT(pevInflictor)) );	// index number of secondary entity
		WRITE_LONG( 5 );   // eventflags (priority and flags)
	MESSAGE_END();

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

		// DMG_BURN	
		// DMG_FREEZE
		// DMG_BLAST
		// DMG_SHOCK

   if (pEdict->v.health > 0)
   {
		pEdict->v.punchangle.x = -2;

		// only send down damage type that have hud art
		int visibleDamageBits = bitsDamage & DMG_SHOWNHUD;

		if (gmsgDamage == 0)
			gmsgDamage = REG_USER_MSG( "Damage", -1 );

		MESSAGE_BEGIN( MSG_ONE, gmsgDamage, NULL, VARS(pEdict) );
			WRITE_BYTE( 0 );
			WRITE_BYTE( fTookDamage );
			WRITE_LONG( visibleDamageBits );
			WRITE_COORD( pevInflictor->origin.x );
			WRITE_COORD( pevInflictor->origin.y );
			WRITE_COORD( pevInflictor->origin.z );
		MESSAGE_END();
	}

	return fTookDamage;
}


int UTIL_TakeHealth (edict_t *pEdict, float flHealth, int bitsDamageType)
{
	if (!pEdict->v.takedamage)
		return 0;

// heal
	if ( pEdict->v.health >= pEdict->v.max_health )
		return 0;

	pEdict->v.health += flHealth;

	if (pEdict->v.health > pEdict->v.max_health)
		pEdict->v.health = pEdict->v.max_health;

	return 1;
}


void UTIL_TraceBleed( edict_t *pEdict, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
   // note: This function should ONLY be used for players!!!

	if (flDamage == 0)
		return;

	if (! (bitsDamageType & (DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB | DMG_MORTAR)))
		return;
	
	// make blood decal on the wall! 
	TraceResult Bloodtr;
	Vector vecTraceDir; 
	float flNoise;
	int cCount;
	int i;

	if (flDamage < 10)
	{
		flNoise = 0.1;
		cCount = 1;
	}
	else if (flDamage < 25)
	{
		flNoise = 0.2;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}

	for ( i = 0 ; i < cCount ; i++ )
	{
		vecTraceDir = vecDir * -1;// trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172, ignore_monsters, pEdict, &Bloodtr);

		if ( Bloodtr.flFraction != 1.0 )
		{
			int bloodColor = pEdict->v.iuser3;
			if ( !bloodColor )
				bloodColor = BLOOD_COLOR_RED;

			UTIL_BloodDecalTrace( &Bloodtr, bloodColor );
		}
	}
}

void UTIL_TraceAttack( edict_t *pEdict, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
   // note: This function should ONLY be used for players!!!

	if ( pEdict->v.takedamage )
	{
		int bloodColor = pEdict->v.iuser3;
		if ( !bloodColor )
			bloodColor = BLOOD_COLOR_RED;

		AddMultiDamage( pevAttacker, pEdict, flDamage, bitsDamageType );

		SpawnBlood(ptr->vecEndPos, bloodColor, flDamage);// a little surface blood.

		UTIL_TraceBleed( pEdict, flDamage, vecDir, ptr, bitsDamageType );
	}
}


int UTIL_IsMoving(edict_t *pEdict)
{
   return (pEdict->v.velocity != g_vecZero);
}


Vector UTIL_EyePosition(edict_t *pEdict)
{
   return (pEdict->v.origin + pEdict->v.view_ofs);
}


Vector UTIL_Center(edict_t *pEdict)
{
   if (pEdict->v.flags & FL_CLIENT)
	   return pEdict->v.origin;
   else
      return (pEdict->v.origin + ((pEdict->v.mins + pEdict->v.maxs) * 0.5));
}


edict_t *UTIL_GetNextTarget( edict_t *pEntity )
{
	if ( FStringNull( pEntity->v.target ) )
		return NULL;
	edict_t *pTarget = FIND_ENTITY_BY_TARGETNAME ( NULL, STRING(pEntity->v.target) );
	if ( FNullEnt(pTarget) )
		return NULL;

	return pTarget;
}

edict_t *UTIL_FindNearestPlayer(edict_t *pEdict, float m_flFieldOfView)
{
   int playerIndex = 0;
   float distance, nearest_distance;
   edict_t *pNearestPlayer = NULL;

   nearest_distance = 9999.9f;

	while ( playerIndex <= gpGlobals->maxClients )
	{
		edict_t *pPlayerEdict = INDEXENT( playerIndex );

		if ( pPlayerEdict && !pPlayerEdict->free )
		{
         if (UTIL_IsPlayer(pPlayerEdict) && UTIL_IsAlive(pPlayerEdict))
         {
            if (UTIL_FInViewCone( pPlayerEdict, pEdict, m_flFieldOfView ) &&
                    !FBitSet( pPlayerEdict->v.flags, FL_NOTARGET ) && UTIL_FVisible( pPlayerEdict, pEdict ) )
            {
               distance = (pPlayerEdict->v.origin - pEdict->v.origin).Length();
               if (distance < nearest_distance)
               {
                  nearest_distance = distance;
                  pNearestPlayer = pPlayerEdict;
               }
            }
         }

		}

      playerIndex++;
	}

   return pNearestPlayer;
}


bool UTIL_IsBSPModel( edict_t *pent )
{
	return (pent->v.solid == SOLID_BSP || pent->v.movetype == MOVETYPE_PUSHSTEP);
}

void UTIL_TakeDamageExternal( edict_t *pEdict, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Tell AMXX to call TakeDamage for us.
	char extCmd[64];
	sprintf( extCmd, "monster_hurt_entity %i %i %i %f %i\n", ENTINDEX( pEdict ), ENTINDEX( ENT( pevInflictor ) ), ENTINDEX( ENT( pevAttacker ) ), flDamage, bitsDamageType );
	SERVER_COMMAND( extCmd );
}
