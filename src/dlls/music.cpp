//=========================================================
// Ambient Music - when triggered, it will play an mp3
// music file locally to players, looped or once.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cmbase.h"
#include	"cmbasemonster.h"
#include	"cmbaseextra.h"

// spawnflags
#define	SF_MUSIC_LOOP				2 // music will loop instead of playing once.
#define	SF_MUSIC_ACTIVATOR_ONLY		4 // only play to the one that activates this entity.

void CMAmbientMusic::KeyValue(KeyValueData *pkvd)
{
	CMBaseMonster::KeyValue(pkvd);
}
void CMAmbientMusic::Spawn()
{
	pev->solid = SOLID_NOT;
	SetUse(&CMAmbientMusic::MusicUse);
	pev->classname = MAKE_STRING("ambient_music");
}

void CMAmbientMusic::MusicUse(edict_t *pActivator, edict_t *pCaller, USE_TYPE useType, float value)
{
	// no music
	if (FStringNull(pev->message))
		return;

	// not a player (if not to everyone)
	if (pev->spawnflags & SF_MUSIC_ACTIVATOR_ONLY && !UTIL_IsPlayer(pActivator))
		return;

	if (pev->spawnflags & SF_MUSIC_ACTIVATOR_ONLY)
		MESSAGE_BEGIN(MSG_ALL, SVC_STUFFTEXT);
	else
		MESSAGE_BEGIN(MSG_ONE, SVC_STUFFTEXT, NULL, pActivator);

	// triggering off
	if (useType == USE_OFF || m_fPlaying && useType != USE_ON)
	{
		WRITE_STRING("mp3 stop\n");
		m_fPlaying = FALSE;
	}
	else // USE_ON / USE_TOGGLE
	{
		char szPath[256];
		
		if (pev->spawnflags & SF_MUSIC_LOOP)
			sprintf(szPath, "mp3 loop %s\n", STRING(pev->message));
		else
			sprintf(szPath, "mp3 play %s\n", STRING(pev->message));
		
		WRITE_STRING(szPath);
		m_fPlaying = TRUE;
	}

	MESSAGE_END();
}
