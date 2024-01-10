//=========================================================
// Set CVar - trigger_setcvar from Sven Co-op.
// Change a map CVar when triggered.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cmbase.h"
#include	"cmbasemonster.h"
#include	"cmbaseextra.h"

#define MAX_CVARS 10

// list of CVars that can be modified by this entity
char *cvar_list[MAX_CVARS]
{
	"mp_falldamage",
	"mp_flashlight",
	"mp_fraglimit",
	"mp_timelimit",
	"mp_weaponstay",
	"sv_accelerate",
	"sv_airaccelerate"
	"sv_friction",
	"sv_gravity",
	"sv_maxspeed"
}; // quite lame compared to SC, but many CVars don't exist in vanilla HL

void CMSetCVar::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszCVarToChange"))
	{
		for (int index = 0; index < MAX_CVARS; index++)
		{
			if (strcmp(pkvd->szValue, cvar_list[index]) == 0)
			{
				m_iszCVarToChange = ALLOC_STRING(pkvd->szValue);
				break;
			}
		}
		if (FStringNull(m_iszCVarToChange))
		{
			ALERT(at_console, "trigger_setcvar - can't change CVar \"%s\". not supported!\n", pkvd->szValue);
		}
		pkvd->fHandled = TRUE;
	}
	else
		CMBaseMonster::KeyValue(pkvd);
}
void CMSetCVar::Spawn()
{
	pev->solid = SOLID_NOT;
	SetUse(&CMSetCVar::ActUse);
	pev->classname = MAKE_STRING("trigger_setcvar");
}

void CMSetCVar::ActUse(edict_t *pActivator, edict_t *pCaller, USE_TYPE useType, float value)
{
	// nothing to change
	if (FStringNull(m_iszCVarToChange))
		return;

	CVAR_SET_FLOAT(STRING(m_iszCVarToChange), atof(STRING(pev->message)));

	if (!FStringNull(pev->netname))
	{
		FireTargets(STRING(pev->netname), pActivator, this->edict(), USE_TOGGLE, 0.0f);
	}
}
