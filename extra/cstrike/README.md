## Counter-Strike (cstrike)

Auxiliary Tools to use MonsterMod in Counter-Strike.

### AMX Mod X Plugins

#### -A note about AMXX plugins-

All plugins in this section require AMXX 1.9.0 or greater, they will not work on 1.8.2 or older.

Compiled plugins are provided in the `bin` folder for your convenience. However, if you prefer to build the plugins yourself, the source code of all AMXX plugins are located in the `src` folder for compilation.

#### Unprecacher

Counter-Strike precaches the sounds of all weapons. This means that sounds such as "clip-ins", "clip-outs" are added to the list, taking quite a bit of space in the precache count. Let it be a reminder that our good old GoldSrc can only store a maximum of 512 precached resources. Most of these sounds are handled client-side by the models themselves, so there is no need for them to be kept precached on the server. Only the weapons fire sounds are needed.

This plugin removes 85 sounds from the precache list, adding extra space for additional monsters to fit in the map.
