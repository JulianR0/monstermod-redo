## Base Plugins (All Mods)

Auxiliary Tools to use MonsterMod in any GoldSrc game.

These are base plugins that should be installed first before adding any mod-specific plugin.

### AMX Mod X Plugins

#### -A note about AMXX plugins-

All plugins in this section require AMXX 1.9.0 or greater, they will not work on 1.8.2 or older.

Compiled plugins are provided in the `bin` folder for your convenience. However, if you prefer to build the plugins yourself, the source code of all AMXX plugins are located in the `src` folder for compilation.

#### GoldSrc --> MonsterMod Use Dispatcher

Because MonsterMod entities are, -quite literally-, just a func_wall with its own logic, trying to trigger any MonsterMod entity from the outside will fail, as it will attempt to use the logic of game's "func_wall" instead of our own.

To add salt to injury, Metamod is incapable of hooking use functions, as the pfnUse/DispatchUse methods provided by the API has been deprecated, and no longer work. Leaving AMXX's HamSandwich as the only module that can hook an entity's Use() function.

In normal circunstances, you do not need this plugin. But let's say you have a turret monster, disabled by default, and you gave it a targetname. Even with a name, triggering this turret regardless of the method used will not activate it.

This plugin redirects the game's func_wall Use() function to MonsterMod, connecting the missing piece and allowing you to trigger MonsterMod entities with your shiny func_button.

I can't believe I've done this. *incomprehensible screams*
