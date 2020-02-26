
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifndef __linux__
#include <io.h>
#else
#include <unistd.h>
#endif

#include	"extdll.h"
#include "dllapi.h"
#include "meta_api.h"
#include	"skill.h"

extern cvar_t *dllapi_log;

skilldata_t	gSkillData;

struct skill_cfg_t
{
   char *name;
   float *value;
};

skill_cfg_t skill_cfg[] = {
   {"sk_agrunt_health", &gSkillData.agruntHealth},
   {"sk_agrunt_dmg_punch", &gSkillData.agruntDmgPunch},
   {"sk_apache_health", &gSkillData.apacheHealth},
   {"sk_barney_health", &gSkillData.barneyHealth},
   {"sk_bigmomma_health_factor", &gSkillData.bigmommaHealthFactor},
   {"sk_bigmomma_dmg_slash", &gSkillData.bigmommaDmgSlash},
   {"sk_bigmomma_dmg_blast", &gSkillData.bigmommaDmgBlast},
   {"sk_bigmomma_radius_blast", &gSkillData.bigmommaRadiusBlast},
   {"sk_bullsquid_health", &gSkillData.bullsquidHealth},
   {"sk_bullsquid_dmg_bite", &gSkillData.bullsquidDmgBite},
   {"sk_bullsquid_dmg_whip", &gSkillData.bullsquidDmgWhip},
   {"sk_bullsquid_dmg_spit", &gSkillData.bullsquidDmgSpit},
   {"sk_gargantua_health", &gSkillData.gargantuaHealth},
   {"sk_gargantua_dmg_slash", &gSkillData.gargantuaDmgSlash},
   {"sk_gargantua_dmg_fire", &gSkillData.gargantuaDmgFire},
   {"sk_gargantua_dmg_stomp", &gSkillData.gargantuaDmgStomp},
   {"sk_hassassin_health", &gSkillData.hassassinHealth},
   {"sk_headcrab_health", &gSkillData.headcrabHealth},
   {"sk_headcrab_dmg_bite", &gSkillData.headcrabDmgBite},
   {"sk_hgrunt_health", &gSkillData.hgruntHealth},
   {"sk_hgrunt_kick", &gSkillData.hgruntDmgKick},
   {"sk_hgrunt_pellets", &gSkillData.hgruntShotgunPellets},
   {"sk_hgrunt_gspeed", &gSkillData.hgruntGrenadeSpeed},
   {"sk_houndeye_health", &gSkillData.houndeyeHealth},
   {"sk_houndeye_dmg_blast", &gSkillData.houndeyeDmgBlast},
   {"sk_islave_health", &gSkillData.slaveHealth},
   {"sk_islave_dmg_claw", &gSkillData.slaveDmgClaw},
   {"sk_islave_dmg_clawrake", &gSkillData.slaveDmgClawrake},
   {"sk_islave_dmg_zap", &gSkillData.slaveDmgZap},
   {"sk_ichthyosaur_health", &gSkillData.ichthyosaurHealth},
   {"sk_ichthyosaur_shake", &gSkillData.ichthyosaurDmgShake},
   {"sk_leech_health", &gSkillData.leechHealth},
   {"sk_leech_dmg_bite", &gSkillData.leechDmgBite},
   {"sk_controller_health", &gSkillData.controllerHealth},
   {"sk_controller_dmgzap", &gSkillData.controllerDmgZap},
   {"sk_controller_speedball", &gSkillData.controllerSpeedBall},
   {"sk_controller_dmgball", &gSkillData.controllerDmgBall},
   {"sk_nihilanth_health", &gSkillData.nihilanthHealth},
   {"sk_nihilanth_zap", &gSkillData.nihilanthZap},
   {"sk_scientist_health", &gSkillData.scientistHealth},
   {"sk_scientist_heal", &gSkillData.scientistHeal},
   {"sk_snark_health", &gSkillData.snarkHealth},
   {"sk_snark_dmg_bite", &gSkillData.snarkDmgBite},
   {"sk_snark_dmg_pop", &gSkillData.snarkDmgPop},
   {"sk_zombie_health", &gSkillData.zombieHealth},
   {"sk_zombie_dmg_one_slash", &gSkillData.zombieDmgOneSlash},
   {"sk_zombie_dmg_both_slash", &gSkillData.zombieDmgBothSlash},
   {"sk_12mm_bullet", &gSkillData.monDmg9MM},
   {"sk_9mmAR_bullet", &gSkillData.monDmgMP5},
   {"sk_9mm_bullet", &gSkillData.monDmg12MM},
   {"sk_9mmAR_grenade", &gSkillData.monDmgM203Grenade},
   {"sk_hornet_dmg", &gSkillData.monDmgHornet},
   {"", NULL}
};

bool get_input(FILE *fp, char *input);


void scan_monster_skill(FILE *fp)
{
   char input[1024];
   int index, len, pos;
   bool found;

   while (get_input(fp, input))
   {
      index = 0;
      found = FALSE;

      while (skill_cfg[index].name[0])
      {
         len = strlen(skill_cfg[index].name);
         if (strncmp(input, skill_cfg[index].name, len) == 0)
         {
            found = TRUE;
            pos = len;
            sscanf(&input[pos], "%f", skill_cfg[index].value);

            if (dllapi_log->value)
               LOG_MESSAGE(PLID, "skill setting %s set to %f",
                           skill_cfg[index].name, *skill_cfg[index].value);

            break;
         }
         index++;
      }

      if (!found)
      {
         //META_CONS("[MONSTER] ERROR: unknown monster_skill.cfg item: %s", input);
         LOG_MESSAGE(PLID, "ERROR: unknown monster_skill.cfg item: %s", input);
      }
   }
}


void monster_skill_init(void)
{
   char game_dir[256];
   char filename[256];
   FILE *fp = NULL;

	// Alien Grunt (agrunt)
	gSkillData.agruntHealth = 90.0f;
	gSkillData.agruntDmgPunch = 20.0f;

	// Apache (apache)
	gSkillData.apacheHealth = 250.0f;

	// Barney (barney)
	gSkillData.barneyHealth = 35.0f;

	// Big momma
	gSkillData.bigmommaHealthFactor = 1.5f;
	gSkillData.bigmommaDmgSlash = 60.0f;
	gSkillData.bigmommaDmgBlast = 120.0f;
	gSkillData.bigmommaRadiusBlast = 250.0f;

	// Bullsquid (bullsquid)
	gSkillData.bullsquidHealth = 40.0f;
	gSkillData.bullsquidDmgBite = 25.0f;
	gSkillData.bullsquidDmgWhip = 35.0f;
	gSkillData.bullsquidDmgSpit = 10.0f;

	// Gargantua
	gSkillData.gargantuaHealth = 800.0f;
	gSkillData.gargantuaDmgSlash = 30.0f;
	gSkillData.gargantuaDmgFire = 5.0f;
	gSkillData.gargantuaDmgStomp = 100.0f;

	// Hassassin (hassassin)
	gSkillData.hassassinHealth = 50.0f;

	// Headcrab (headcrab)
	gSkillData.headcrabHealth = 10.0f;
	gSkillData.headcrabDmgBite = 10.0f;

	// Hgrunt (hgrunt)
	gSkillData.hgruntHealth = 50.0f;
	gSkillData.hgruntDmgKick = 10.0f;
	gSkillData.hgruntShotgunPellets = 5.0f;
	gSkillData.hgruntGrenadeSpeed = 600.0f;

	// Houndeye (houndeye)
	gSkillData.houndeyeHealth = 20.0f;
	gSkillData.houndeyeDmgBlast = 15.0f;

	// ISlave (islave)
	gSkillData.slaveHealth = 30.0f;
	gSkillData.slaveDmgClaw = 10.0f;
	gSkillData.slaveDmgClawrake = 25.0f;
	gSkillData.slaveDmgZap = 10.0f;

	// Icthyosaur
	gSkillData.ichthyosaurHealth = 200.0f;
	gSkillData.ichthyosaurDmgShake = 35.0f;

	// Leech
	gSkillData.leechHealth = 2.0f;
	gSkillData.leechDmgBite = 2.0f;

	// Controller
	gSkillData.controllerHealth = 60.0f;
	gSkillData.controllerDmgZap = 25.0f;
	gSkillData.controllerSpeedBall = 800.0f;
	gSkillData.controllerDmgBall = 4.0f;

	// Nihilanth
	gSkillData.nihilanthHealth = 800.0f;
	gSkillData.nihilanthZap = 30.0f;

	// Scientist (scientist)
	gSkillData.scientistHealth = 20.0f;
	gSkillData.scientistHeal = 25.0f;

	// Snark (snark)
	gSkillData.snarkHealth = 2.0f;
	gSkillData.snarkDmgBite = 10.0f;
	gSkillData.snarkDmgPop = 5.0f;

	// Zombie (zombie)
	gSkillData.zombieHealth = 50.0f;
	gSkillData.zombieDmgOneSlash = 20.0f;
	gSkillData.zombieDmgBothSlash = 40.0f;

	// MONSTER WEAPONS
	gSkillData.monDmg9MM = 5.0f;
	gSkillData.monDmgMP5 = 4.0f;
	gSkillData.monDmg12MM = 10.0f;
	gSkillData.monDmgM203Grenade = 100.0f;

	// HORNET
	gSkillData.monDmgHornet = 5.0f;


   // find the directory name of the currently running MOD...
   (*g_engfuncs.pfnGetGameDir)(game_dir);

   strcpy(filename, game_dir);
   strcat(filename, "/monster_skill.cfg");

   // check if the map specific filename exists...
   if (access(filename, 0) == 0)
   {
      if (dllapi_log->value)
      {
         //META_CONS("[MONSTER] Processing monster skill file=%s", filename);
         LOG_MESSAGE(PLID, "Processing monster skill file=%s", filename);
      }

      if ((fp = fopen(filename, "r")) == NULL)
      {
         //META_CONS("[MONSTER] ERROR: Could not open \"%s\"!", filename);
         LOG_MESSAGE(PLID, "ERROR: Could not open \"%s\"!", filename);
      }

      scan_monster_skill(fp);

      fclose(fp);
   }
   else
   {
      //META_CONS("[MONSTER] ERROR: Could not find \"%s\" (default skill used)", filename);
      LOG_MESSAGE(PLID, "ERROR: Could not find \"%s\" (default skill used)", filename);
   }
}

