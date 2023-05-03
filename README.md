# MonsterMod

Redo. AKA Recreation.

## History

*-Do note that due to the obscurity of the project, the information displayed here may be incorrect or inaccurate.-*

In July 2th of 2002, a MetaMod plugin named "Monster" was created. **botman** was the one behind the project. The plugin allowed server operators to mix normal gameplay with monsters on various mods, such as Team Fortress Classic, Counter-Strike, among others.

Unfortunately, this plugin earned only 3 updates, it's last version being released on July 30th of the same year.

Another person, **Rick90** on ModDB ( *RollerPig?* ), continued development of the plugin on *an unknown date*, and give it a new name: "Monster Mod". This modification of the original project gave it extended features. New monsters being it's key additions.

Few were lucky to adquire a compiled binary of the plugin, this rare binary has met it's final public update on September 29th of 2007. The development of the project continued behind closed doors, becoming a private property for a specific server.

The source code of the rare binary was never known, and the private development met it's death on July 2th of 2014. The final announcement published on the old ModDB page was made on that very same day, and the binary or source code distribution of such update never saw the light of the day.

February 25th, 2020.
A small light is seen at the distance...

## What is this?

MonsterMod is a MetaMod plugin. Its purpose was to allow multiplayer games to add monsters, where it wasn't possible to do so by normal means. The updates of the project became incredibly obscure: Getting the "up-to-date" versions containing the new additions (opposing force monsters, for example) were very difficult. And the only one who managed to bring the plugin even futher kept the progress of the plugin private.

After 20 years (and a half) since botman's original plugin was released, the future of the project became nothing but a forgotten, ancient relic of the past.

Not anymore...

The first goal of this project aims towards the recreation of the new features of the "obscured and updated" Monster Mod plugin. Taking botman's original 2002 plugin and working from the ground up, the mission is to rebuild it with the new features and monsters that only few were able to see.

The source code is completely free for everyone to use: In the event that the development of this project falls and becomes stagnant again, the plugin will live on, as the project's second goal is its preservation. ~~The original botman's page where you can download the 2002 plugin will not stay up forever.~~ **April 2023 update**: The page is gone, the original 2002 plugin can no longer be found.

Under no circumstances shall we allow this project to fade away and become lost amidst the gears of time.

## Installation

The plugin -should- be able to be used out-of-the-box by simply downloading the binary and adding the appropiate entry in metamod's plugin list.

**Windows:**
`win32 addons\monstermod\monster_mm.dll`

**Linux:**
`linux addons/monstermod/monster_mm_i386.so`

Additional configuration files are included in the release files, each explaining it's usage and installation instructions.

Extra MonsterMod features can be unlocked with additional AMX Mod X plugins which are located in the `extra` folder. All these plugins are optional, and only required based on your use-case.

## Build Instructions

The [wiki](https://github.com/JulianR0/monstermod-redo/wiki) contains instructions on how to compile MonsterMod by yourself.

## MonsterMod and ReHLDS

Usage of ReHLDS is highly recommended, as you can use the command `rescount` which will reveal the current number of precached models and sounds. You can also use `reslist model` and `reslist sound` to see the entire list of precached content.

Keeping track of the number of precached content will allow you to maximize the number of monsters you can use without risking going over the limits.

## Known Bugs

I'm aware that the plugin is far from perfect, and there are a few things that need polishing *-especially the AI-*. I'll try to fix/will be fixing as the project evolves:

- Rarely, Stukabats will become unable to fly towards their target, standing in air doing seemingly nothing. Cause of bug unknown.

There are probably more issues that aren't listed here, I'll attempt to fix them as I find them. Of course, any bug report is welcome. If reporting a bug, try to explain step by step how the bug ocurred. The easier it is to replicate a bug, the easier it is to locate it and fix it.

## Milestones

Attempting to recreate everything in one go is a daunting task.
Let it be known that the original 2002 source code will NOT compile on today's compilers, and does NOT contain all the necessary files for compilation. The preliminary was to rewrite and provide as many files or lines of code to ensure it can compile again, and be usable on an actual HLDS installation.

Current milestones are separated by "Tiers", which are as follows:

### Tier 0

- Update source code so it can compile AND run **ON LINUX**. **[DONE]**

### Tier 1

- Rework config processing to use a "key --> value" format. **[DONE]**
- Add info_node and info_node_air support for AI navigation. **[DONE]**
- Add node_viewer to debug node graph generation. **[DONE]**
- Implement Gargantua and all Turrets from Half-Life. **[DONE]**

### Tier 2

- Add *-at least minimal-* death messages. *-Example: "\<player\> was killed by a \<monster\>".-* **[DONE]**
- Implement HUD info about the monsters, along with the "displayname" keyvalue. **[DONE]**
- Implement custom monster classification, the "classify" keyvalue. **[DONE]**

### Tier 3

- Update source code so it can compile AND run **ON WINDOWS**. **[DONE]**
- Implement *-almost-* all Opposing Force monsters. **[DONE]**
- Implement *-almost-* all default Sven Co-op monsters. **[DONE]**
- Make MonsterMod aware of normal game entities. *-For those wanting to use this in vanilla HL.-* **[DONE]**
- Custom model support. **[DONE]**

### Tier 4

- Implement reading entities within the BSP itself. **[DONE]**
- Add build instructions. **[DONE]**
- Global Model Replacement. **[DONE]**
- Global Sound Replacement. **[DONE]**
- Miscellaneous customization options, such as blood color. **[DONE]**
- Individual sound replacement: "soundlist" keyvalue for monsters. **[DONE]**
- Sentences support for speakable monsters. **[DONE]**
- Attempt to fix bugs as they appear.

### Tier 5

- Enhance the AI.
- Add configurations to change AI behaviour.
- Create "tool" entities for easier map customization.
- Optimize and clean the code.
- Add a wiki with full documentation.
- Fix **ALL** bugs, specially those not covered in Tier 4.


What will the future hold after all Tiers has been completed?
