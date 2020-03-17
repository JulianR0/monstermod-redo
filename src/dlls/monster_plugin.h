//
// monster_plugin.h
//

#ifndef MONSTER_PLUGIN_H
#define MONSTER_PLUGIN_H

typedef struct pKVD
{
	char key[33];
	char value[33];
};

typedef struct
{
   char *name;
   bool need_to_precache;
} monster_type_t;


class CMBaseMonster;

typedef struct
{
   int monster_index;
   edict_t *monster_pent;
   bool killed;
   int respawn_index;
   CMBaseMonster *pMonster;
} monster_t;

#define MAX_MONSTER_ENTS 400 // increased from 200 so it can hold non-monster entities

extern monster_t monsters[MAX_MONSTER_ENTS];

typedef struct {
   Vector origin;
   Vector angles;
   float delay;
   unsigned char monster;
   int spawnflags;
   pKVD *keyvalue;
   float respawn_time;
   bool need_to_respawn;
} monster_spawnpoint_t;

#define MAX_MONSTERS 100
extern monster_spawnpoint_t monster_spawnpoint[MAX_MONSTERS];

// this is here to store if a node we want to spawn is an ordinary one, or a flying one
typedef struct
{
	Vector origin;
	bool is_air_node;
} node_spawnpoint_t;

// nodes.cpp defines 1024 max nodes, but that amount is likely to trigger a
// no free edicts crash if the server num_edicts is low. Increase if needed.
#define MAX_NODES 256
extern node_spawnpoint_t node_spawnpoint[MAX_NODES];

extern DLL_GLOBAL short g_sModelIndexFireball;// holds the index for the fireball
extern DLL_GLOBAL short g_sModelIndexSmoke;// holds the index for the smoke cloud
extern DLL_GLOBAL short g_sModelIndexWExplosion;// holds the index for the underwater explosion
extern DLL_GLOBAL short g_sModelIndexBubbles;// holds the index for the bubbles model
extern DLL_GLOBAL short g_sModelIndexBloodDrop;// holds the sprite index for the initial blood
extern DLL_GLOBAL short g_sModelIndexBloodSpray;// holds the sprite index for splattered blood
extern DLL_GLOBAL short g_sModelIndexLaser;// holds the index for the laser beam
extern DLL_GLOBAL const char *g_pModelNameLaser;
extern DLL_GLOBAL short g_sModelIndexLaserDot;// holds the index for the laser beam dot

#endif
