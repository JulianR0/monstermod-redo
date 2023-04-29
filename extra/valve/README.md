## Half-Life (valve)

Auxiliary Tools to use MonsterMod in the original Half-Life.

### AMX Mod X Plugins

#### -A note about AMXX plugins-

All plugins in this section require AMXX 1.9.0 or greater, they will not work on 1.8.2 or older.

Compiled plugins are provided in the `bin` folder for your convenience. However, if you prefer to build the plugins yourself, the source code of all AMXX plugins are located in the `src` folder for compilation.

#### Bridge Plugin

MonsterMod monsters are hacked "func_wall"'s with simulated AI. Because of this, MonsterMod cannot interact with the Half-Life monsters. This issue also happens on the other side: The Half-Life monsters cannot interact with the MonsterMod monsters.

In simple terms, this plugin acts as a "bridge" to help Half-Life and MonsterMod communicate with each other. With this plugin, HL and MM monsters can "see" and interact with each other, and will react accordingly.

Without this plugin, HL and MM monsters will be "invisible" to each other.

#### Soundlist

This plugin allows you to use the "soundlist" keyvalue in HL monsters to individually replace monster sounds. Path starts from `sound/MAPNAME`. Use `../` to go to the previous directory if needed.

The plugin can work standalone, meaning it can be used without MonsterMod.

#### Extra Keyvalues

**"Bridge Plugin" is REQUIRED for this add-on to work**

This plugin allows you use the following keyvalues on HL monsters:

- classify
- is_player_ally *(non-monstermaker entities)*
- bloodcolor
- respawn_as_playerally *(monstermaker entity)*

The plugin also adds a 5th keyvalue, **"use_monstermod"**. When reading entities from the BSP file, if MonsterMod finds an entity that already exists in the game, it will stick to the game entity and ignore it. By setting this keyvalue to "1", it will explicity use MonsterMod for this entity.

This is useful when, for example, you are trying to spawn a Pit Drone from a monstermaker. HL does not recognize monster_pitdrone, but MonsterMod does. By redirecting the entity to MonsterMod, Pit Drones will spawn from this monstermaker.
