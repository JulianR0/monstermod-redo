/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
****/

// this is a stripped down "bspfile.h". contains only entity data similar
// to using a ripent tool to read ents from a bsp, hence the name.
// -Giegue
#include "extdll.h"

// upper design bounds

#define	MAX_MAP_ENTITIES	1024
#define	MAX_MAP_ENTSTRING	(128*1024)

// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024

//=============================================================================

#define BSPVERSION	30

typedef struct
{
	int		fileofs, filelen;
} lump_t;

#define	LUMP_ENTITIES	0

#define	HEADER_LUMPS	15

typedef struct
{
	int			version;
	lump_t		lumps[HEADER_LUMPS];
} dheader_t;

//============================================================================

// the utilities get to be lazy and just use large static arrays

extern	int			entdatasize;
extern	char		dentdata[MAX_MAP_ENTSTRING];

void	LoadBSPFile(char *filename);

//===============

typedef struct epair_s
{
	struct epair_s	*next;
	char	*key;
	char	*value;
} epair_t;

typedef struct
{
	vec3_t		origin;
	int			firstbrush;
	int			numbrushes;
	epair_t		*epairs;
} entity_t;

extern	int			num_entities;
extern	entity_t	entities[MAX_MAP_ENTITIES];

void	ParseEntities(void);

epair_t *ParseEpair(void);

// --
/* MERGE cmdlib.h AND scriplib.h INTO ripent.h */
/* Only add needed functions. */
// --

// -- cmdlib.h --
int		LoadFile(char *filename, void **bufferptr);
int		LittleLong(int l);
char	*copystring(char *s);

FILE	*SafeOpenRead(char *filename);
void	SafeRead(FILE *f, void *buffer, int count);
char	*ExpandPath(char *path);	// from scripts

// -- scriplib.h --
#define	MAXTOKEN	512

extern	char	token[MAXTOKEN];
bool			GetToken(bool crossline);
void			ParseFromMemory(char *buffer, int size);
