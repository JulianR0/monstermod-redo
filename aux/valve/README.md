## Half-Life (valve)

Auxiliary Tools to use MonsterMod in the original Half-Life.

### AMX Mod X Plugins

#### -A note about AMXX plugins-

All plugins in this section require AMXX 1.9.0 or greater, they will not work on 1.8.2 or older.

Compiled plugins are provided in the `bin` folder for your convenience. However, if you prefer to build the plugins yourself, the source code of all AMXX plugins are located in the `src` folder for compilation.

#### Half-Life <--> MonsterMod Bridge Plugin

MonsterMod monsters are hacked "func_wall"'s with simulated AI. Because of this, MonsterMod cannot interact with the Half-Life monsters. This issue also happens on the other side: The Half-Life monsters cannot interact with the MonsterMod monsters.

In simple terms, this plugin acts as a "bridge" to help Half-Life and MonsterMod communicate with each other. With this plugin, HL and MM monsters can "see" and interact with each other, and will react accordingly.

Without this plugin, HL and MM monsters will be "invisible" to each other.
