#ifndef GLOBALREPLACE_H
#define GLOBALREPLACE_H

#define MAX_REPLACEMENTS 255

namespace REPLACER
{
void Init(void);
bool AddGlobalModel(const char *from, const char *to);
bool AddGlobalSound(const char *from, const char *to);

const char* FindModelReplacement( edict_t *pMonster, const char *from );
inline const char* FindModelReplacement( const char *from ) { return FindModelReplacement( NULL, from ); }

const char* FindSoundReplacement( edict_t *pMonster, const char *from );
inline const char* FindSoundReplacement( const char *from ) { return FindSoundReplacement( NULL, from ); }
}

#endif