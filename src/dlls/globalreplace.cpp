//=========================================================
// Global Replacement:
// Tool to replace all default models/sounds with a
// customized list.
//=========================================================
#include "extdll.h"
#include "dllapi.h"
#include "meta_api.h"

#include "cmbase.h"
#include "cmbasemonster.h"

namespace REPLACER
{
REPLACER *GMR;
REPLACER *GSR;
int numModels;
int numSounds;

void Init(void)
{
	if ( GMR != NULL )
	{
		free( GMR );
		GMR = NULL;
	}
	if ( GSR != NULL )
	{
		free( GSR );
		GSR = NULL;
	}
	numModels = 0;
	numSounds = 0;
}

bool AddGlobalModel(const char *from, const char *to)
{
	if (numModels < MAX_REPLACEMENTS)
	{
		// allocate for the first time
		if (!numModels)
			GMR = (REPLACER*)calloc(MAX_REPLACEMENTS, sizeof(*GMR));
		
		strcpy(GMR[numModels].source, from);
		strcpy(GMR[numModels].destination, to);

		numModels++;
		return true;
	}

	LOG_MESSAGE(PLID, "Can't replace model '%s', too many models in GMR.", from);
	return false;
}

bool AddGlobalSound(const char *from, const char *to)
{
	if (numSounds < MAX_REPLACEMENTS)
	{
		// allocate for the first time
		if (!numSounds)
			GSR = (REPLACER*)calloc(MAX_REPLACEMENTS, sizeof(*GSR));
		
		strcpy(GSR[numSounds].source, from);
		strcpy(GSR[numSounds].destination, to);

		numSounds++;
		return true;
	}

	LOG_MESSAGE(PLID, "Can't replace sound '%s', too many sounds in GSR.", from);
	return false;
}

bool AddIndividualSound(edict_t *pMonster, const char *from, const char *to)
{
	CMBaseMonster *castMonster = GetClassPtr((CMBaseMonster *)VARS(pMonster));

	if ( castMonster == NULL )
		return false;

	int m_iSounds = castMonster->m_isrSounds;

	if (m_iSounds < MAX_REPLACEMENTS)
	{
		// allocate for the first time
		if (!m_iSounds)
			castMonster->m_srSoundList = (REPLACER*)calloc(MAX_REPLACEMENTS, sizeof(*castMonster->m_srSoundList));
		
		strcpy(castMonster->m_srSoundList[m_iSounds].source, from);
		strcpy(castMonster->m_srSoundList[m_iSounds].destination, to);

		castMonster->m_isrSounds++;
		return true;
	}

	LOG_MESSAGE(PLID, "Can't replace sound '%s', too many sounds in list.", from);
	return false;
}

const char* FindModelReplacement( edict_t *pMonster, const char *from )
{
	// Individually set models takes priority!
	if (UTIL_IsValidEntity(pMonster) && !FStringNull(pMonster->v.model))
		return STRING(pMonster->v.model);

	// Find the model
	for (int model = 0; model < numModels; model++)
	{
		if (strcmp(GMR[model].source, from) == 0)
		{
			// If found, use that model instead
			return GMR[model].destination;
		}
	}

	// Nothing found, stick with default
	return from;
}

const char* FindSoundReplacement( edict_t *pMonster, const char *from )
{
	// Individually set sounds takes priority!
	if (UTIL_IsValidEntity(pMonster))
	{
		CMBaseMonster *castMonster = NULL;
		
		// Check if this is really a monster or not
		if (pMonster->v.flags & FL_MONSTER)
			castMonster = GetClassPtr((CMBaseMonster *)VARS(pMonster));
		else
		{
			// This is probably a monster-owned projectile of sorts
			if (!FNullEnt(pMonster->v.owner))
				castMonster = GetClassPtr((CMBaseMonster *)VARS(pMonster->v.owner));
		}
		
		// If still no valid BaseMonster pointer, full stop, use GSR.
		if (castMonster && castMonster->m_isrSounds)
		{
			for (int sound = 0; sound < castMonster->m_isrSounds; sound++)
			{
				if (strcmp(castMonster->m_srSoundList[sound].source, from) == 0)
				{
					// If found, use it
					return castMonster->m_srSoundList[sound].destination;
				}
			}

			// If nothing is found stick to GSR if available
		}
	}

	for (int sound = 0; sound < numSounds; sound++)
	{
		if (strcmp(GSR[sound].source, from) == 0)
		{
			// If found, use that sound instead
			return GSR[sound].destination;
		}
	}
	
	// Nothing found, stick with default
	return from;
}
}