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
typedef struct
{
	char source[128];
	char destination[128];
} REPLACER;

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

const char* FindModelReplacement( edict_t *pMonster, const char *from )
{
	// Individually set models takes priority!
	if ( pMonster && !FStringNull(pMonster->v.model))
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
	if ( pMonster )
	{
		CMBaseMonster *castMonster = GetClassPtr((CMBaseMonster *)VARS(pMonster));
		//placeholder for soundlist keyvalue;
	}

	for (int sound = 0; sound < numSounds; sound++)
	{
		if (strcmp(GSR[sound].source, from) == 0)
		{
			// If found, use that model instead
			return GSR[sound].destination;
		}
	}
	
	// Nothing found, stick with default
	return from;
}
}