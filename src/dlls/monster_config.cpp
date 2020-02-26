#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifndef __linux__
#include <io.h>
#else
#include <unistd.h>
#endif

#include "extdll.h"
#include "dllapi.h"
#include "meta_api.h"

#include "monster_plugin.h"

extern cvar_t *dllapi_log;

extern monster_type_t monster_types[];
extern int monster_spawn_count;


bool get_input(FILE *fp, char *input)
{
   char line[1024];
   int len, pos;

   while (!feof(fp))
   {
      if (fgets(line, 1023, fp) != NULL)
      {
         len = strlen(line);

         if (len == 0)
            continue;  // skip any null lines

         // remove any trailing newline, carriage return or whitespace...
         while ((line[len-1] == '\n') || (line[len-1] == '\r') ||
                isspace(line[len-1]))
         {
            line[len-1] = 0;
            len--;
            if (len == 0)
               break;
         }

         pos = 0;

         while (isspace(line[pos]))
            pos++;  // skip leading blanks

         if ((line[pos] == '/') && (line[pos+1] == '/'))
            continue;  // skip comment lines

         if (line[pos] == 0)
            continue;  // skip empty lines

         strcpy(input, &line[pos]);
         return TRUE;
      }
   }

   return FALSE;  // no input found
}


bool scan_monster_cfg(FILE *fp)
{
   char input[1024];
   bool origin, angle, delay, angle_min, angle_max, monster;
   float x, y, z;

   while (get_input(fp, input))
   {
      if (monster_spawn_count == MAX_MONSTERS)
         continue;  // skip if max monster spawn count reached

      if (input[0] == '{')
      {
         origin = angle = delay = angle_min = angle_max = monster = FALSE;

         monster_spawnpoint[monster_spawn_count].monster_count = 0;

         while (get_input(fp, input))
         {
            if (input[0] == '}')
               break;

            if (strncmp(input, "origin/", 7) == 0)
            {
               if (origin)
               {
                  //META_CONS("[MONSTER] ERROR: origin found twice: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: origin found twice: %s", input);
                  return TRUE;  // error
               }
               if (sscanf(&input[7], "%f %f %f", &x, &y, &z) != 3)
               {
                  //META_CONS("[MONSTER] ERROR: invalid origin: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: invalid origin: %s", input);
                  return TRUE;  // error
               }
               origin = TRUE;
               monster_spawnpoint[monster_spawn_count].origin[0] = x;
               monster_spawnpoint[monster_spawn_count].origin[1] = y;
               monster_spawnpoint[monster_spawn_count].origin[2] = z;
            }
            else if (strncmp(input, "delay/", 6) == 0)
            {
               if (delay)
               {
                  //META_CONS("[MONSTER] ERROR: delay found twice: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: delay found twice: %s", input);
                  return TRUE;  // error
               }
               if (sscanf(&input[6], "%f", &x) != 1)
               {
                  //META_CONS("[MONSTER] ERROR: invalid delay: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: invalid delay: %s", input);
                  return TRUE;  // error
               }
               delay = TRUE;
               monster_spawnpoint[monster_spawn_count].delay = x;
            }
            else if (strncmp(input, "angle/", 6) == 0)
            {
               if (angle)
               {
                  //META_CONS("[MONSTER] ERROR: angle found twice: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: angle found twice: %s", input);
                  return TRUE;  // error
               }
               if (angle_min || angle_max)
               {
                  //META_CONS("[MONSTER] ERROR: you can't specify angle AND angle_min or angle_max: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: you can't specify angle AND angle_min or angle_max: %s", input);
                  return TRUE;  // error
               }
               if (sscanf(&input[6], "%f", &x) != 1)
               {
                  //META_CONS("[MONSTER] ERROR: invalid angle: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: invalid angle: %s", input);
                  return TRUE;  // error
               }
               angle = TRUE;
               monster_spawnpoint[monster_spawn_count].angle_min = x;
               monster_spawnpoint[monster_spawn_count].angle_max = x;
            }
            else if (strncmp(input, "angle_min/", 10) == 0)
            {
               if (angle_min)
               {
                  //META_CONS("[MONSTER] ERROR: angle_min found twice: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: angle_min found twice: %s", input);
                  return TRUE;  // error
               }
               if (angle)
               {
                  //META_CONS("[MONSTER] ERROR: you can't specify angle AND angle_min or angle_max: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: you can't specify angle AND angle_min or angle_max: %s", input);
                  return TRUE;  // error
               }
               if (sscanf(&input[10], "%f", &x) != 1)
               {
                  //META_CONS("[MONSTER] ERROR: invalid angle_min: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: invalid angle_min: %s", input);
                  return TRUE;  // error
               }
               angle_min = TRUE;
               monster_spawnpoint[monster_spawn_count].angle_min = x;
            }
            else if (strncmp(input, "angle_max/", 10) == 0)
            {
               if (angle_max)
               {
                  //META_CONS("[MONSTER] ERROR: angle_max found twice: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: angle_max found twice: %s", input);
                  return TRUE;  // error
               }
               if (angle)
               {
                  //META_CONS("[MONSTER] ERROR: you can't specify angle AND angle_min or angle_max: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: you can't specify angle AND angle_min or angle_max: %s", input);
                  return TRUE;  // error
               }
               if (sscanf(&input[10], "%f", &x) != 1)
               {
                  //META_CONS("[MONSTER] ERROR: invalid angle_max: %s", input);
                  LOG_MESSAGE(PLID, "ERROR: invalid angle_max: %s", input);
                  return TRUE;  // error
               }
               angle_max = TRUE;
               monster_spawnpoint[monster_spawn_count].angle_max = x;
            }
            else if (strncmp(input, "monster/", 8) == 0)
            {
               int index;
               int count = monster_spawnpoint[monster_spawn_count].monster_count;
               if (count < MAX_MONSTER_COUNT)
               {
                  for (index=0; monster_types[index].name[0]; index++)
                  {
                     if (strcmp(&input[8], monster_types[index].name) == 0)
                     {
                        monster_spawnpoint[monster_spawn_count].monster[count] = index;
                        monster_spawnpoint[monster_spawn_count].monster_count++;

                        monster_types[index].need_to_precache = TRUE;
                        break;
                     }
                  }
                  if (monster_types[index].name[0] == 0)
                  {
                     //META_CONS("[MONSTER] ERROR: invalid monster name: %s", input);
                     LOG_MESSAGE(PLID, "ERROR: invalid monster name: %s", input);
                     return TRUE;
                  }
               }
               monster = TRUE;
            }
            else
            {
               //META_CONS("[MONSTER] ERROR: unknown command: %s", input);
               LOG_MESSAGE(PLID, "ERROR: unknown command: %s", input);
               return TRUE;  // error occurred
            }
         }

         // check for all necessary fields here...
         if (!origin)
         {
            //META_CONS("[MONSTER] ERROR: you didn't specify an origin!");
            LOG_MESSAGE(PLID, "ERROR: you didn't specify an origin!");
            return TRUE;
         }
         if (angle_min && !angle_max)
         {
            //META_CONS("[MONSTER] ERROR: you specified angle_min but didn't specify angle_max!");
            LOG_MESSAGE(PLID, "ERROR: you specified angle_min but didn't specify angle_max!");
            return TRUE;
         }
         if (angle_max && !angle_min)
         {
            //META_CONS("[MONSTER] ERROR: you specified angle_max but didn't specify angle_min!");
            LOG_MESSAGE(PLID, "ERROR: you specified angle_max but didn't specify angle_min!");
            return TRUE;
         }
         if (!monster)
         {
            //META_CONS("[MONSTER] ERROR: No monster key found!");
            LOG_MESSAGE(PLID, "ERROR: No monster key found!");
            return TRUE;
         }

         if (!delay)
            monster_spawnpoint[monster_spawn_count].delay = 30;  // 30 second default delay

         if (monster_spawnpoint[monster_spawn_count].delay < 1)
            monster_spawnpoint[monster_spawn_count].delay = 1;  // no negative or zero delay

         if (!angle && !angle_min && !angle_max)  // no angle specified, use 0-359 as default
         {
            monster_spawnpoint[monster_spawn_count].angle_min = 0;
            monster_spawnpoint[monster_spawn_count].angle_max = 359;
         }

         if (monster_spawnpoint[monster_spawn_count].angle_min < 0)
            monster_spawnpoint[monster_spawn_count].angle_min = 0;  // no negative angles

         if (monster_spawnpoint[monster_spawn_count].angle_max > 359)
            monster_spawnpoint[monster_spawn_count].angle_max = 359;  // no angle > 359 degrees

         monster_spawnpoint[monster_spawn_count].respawn_time = gpGlobals->time + RANDOM_FLOAT(10.0, 20.0);
         monster_spawnpoint[monster_spawn_count].need_to_respawn = TRUE;

         if (dllapi_log->value)
         {
            char name_monsters[256];

            LOG_MESSAGE(PLID, "Added monster spawn at: %7.2f %7.2f %7.2f",
               monster_spawnpoint[monster_spawn_count].origin.x,
               monster_spawnpoint[monster_spawn_count].origin.y,
               monster_spawnpoint[monster_spawn_count].origin.z);
            LOG_MESSAGE(PLID, "      with delay = %7.2f, angle_min = %7.2f, angle_max = %7.2f",
               monster_spawnpoint[monster_spawn_count].delay,
               monster_spawnpoint[monster_spawn_count].angle_min,
               monster_spawnpoint[monster_spawn_count].angle_max);
            name_monsters[0] = 0;
            for (int i = 0; i < monster_spawnpoint[monster_spawn_count].monster_count; i++)
            {
               strcat(name_monsters, monster_types[monster_spawnpoint[monster_spawn_count].monster[i]].name);
               strcat(name_monsters, " ");
            }
            LOG_MESSAGE(PLID, "      monsters = %s", name_monsters);
         }

         monster_spawn_count++;
      }
   }

   return FALSE;
}


bool process_monster_cfg(void)
{
   char game_dir[256];
   char filename[256];
   FILE *fp = NULL;
   bool status = FALSE;  // no error

   monster_spawn_count = 0;

   // find the directory name of the currently running MOD...
   (*g_engfuncs.pfnGetGameDir)(game_dir);

   strcpy(filename, game_dir);
#ifdef __linux__
   strcat(filename, "/maps/");
#else
   strcat(filename, "\\maps\\");
#endif
   strcat(filename, STRING(gpGlobals->mapname));
   strcat(filename, "_monster.cfg");

   // check if the map specific filename exists...
   if (access(filename, 0) == 0)
   {
      if (dllapi_log->value)
      {
         //META_CONS("[MONSTER] Processing config file=%s", filename);
         LOG_MESSAGE(PLID, "Processing config file=%s", filename);
      }

      if ((fp = fopen(filename, "r")) == NULL)
      {
         //META_CONS("[MONSTER] ERROR: Could not open \"%s\"!", filename);
         LOG_MESSAGE(PLID, "ERROR: Could not open \"%s\" file!", filename);

         return TRUE;  // return bad status
      }

      status = scan_monster_cfg(fp);

      fclose(fp);
   }

   return status;
}


bool scan_monster_precache_cfg(FILE *fp)
{
   char input[1024];
   bool found;

   while (get_input(fp, input))
   {
      found = FALSE;

      for (int index=0; monster_types[index].name[0]; index++)
      {
         if (strcmp(input, monster_types[index].name) == 0)
         {
            monster_types[index].need_to_precache = TRUE;
            found = TRUE;
            break;
         }
      }

      if (found == FALSE)
      {
         //META_CONS("[MONSTER] ERROR: invalid precache monster name: %s", input);
         LOG_MESSAGE(PLID, "ERROR: invalid precache monster name: %s", input);
      }
   }

   return FALSE;
}


bool process_monster_precache_cfg(void)
{
   char game_dir[256];
   char filename[256];
   FILE *fp = NULL;
   bool status = FALSE;  // no error

   // find the directory name of the currently running MOD...
   (*g_engfuncs.pfnGetGameDir)(game_dir);

   strcpy(filename, game_dir);
   strcat(filename, "/monster_precache.cfg");

   // check if the map specific filename exists...
   if (access(filename, 0) == 0)
   {
      if (dllapi_log->value)
      {
         //META_CONS("[MONSTER] Processing config file=%s", filename);
         LOG_MESSAGE(PLID, "Processing config file=%s", filename);
      }

      if ((fp = fopen(filename, "r")) == NULL)
      {
         //META_CONS("[MONSTER] ERROR: Could not open \"%s\"!", filename);
         LOG_MESSAGE(PLID, "ERROR: Could not open \"%s\" file!", filename);

         return TRUE;  // return bad status
      }

      status = scan_monster_precache_cfg(fp);

      fclose(fp);
   }

   return status;
}
