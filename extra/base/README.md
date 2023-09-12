## Base Plugins (All Mods)

Auxiliary Tools to use MonsterMod in any GoldSrc game.

These are base plugins that should be installed first before adding any mod-specific plugin.

### AMX Mod X Plugins

#### -A note about AMXX plugins-

All plugins in this section require AMXX 1.9.0 or greater, they will not work on 1.8.2 or older.

Compiled plugins are provided in the `bin` folder for your convenience. However, if you prefer to build the plugins yourself, the source code of all AMXX plugins are located in the `src` folder for compilation.

#### Use Dispatcher

Because MonsterMod entities are, -quite literally-, just a func_wall with its own logic, trying to trigger any MonsterMod entity from the outside will fail, as it will attempt to use the logic of game's "func_wall" instead of our own.

In normal circunstances, you do not need this plugin. But let's say you have a turret monster, disabled by default, and you gave it a targetname. Even with a name, triggering this turret regardless of the method used will not activate it.

The same problem occurs from the other side, a MonsterMod entity trying to trigger something will expect said entity to be part of MonsterMod as well.

This plugin redirects the game's func_wall Use() function to MonsterMod, as well as bridging Use() calls from MonsterMod back to the game, connecting the missing pieces and allowing you to trigger MonsterMod entities from the game, and game entities from MonsterMod.

#### External TakeDamage

When a MonsterMod entity tries to inflict damage to something, it expects said entity to be either a player or a MonsterMod monster. If something else is to be found, it will deal no damage to it, as it doesn't know how to manage it.

This plugin allows MonsterMod entities to inflict damage to normal game entities, such as breakables.
