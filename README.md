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

MonsterMod is a MetaMod plugin. It's purpose was to allow support for monsters to spawn in multiplayer games where they are not. The project updates became incredibly obscure, getting up-to-date versions and the new additions was very difficult. And the only one who had even futher progress kept the plugin private.

Nearly 18 years after botman's original plugin was released, the future of the project became nothing but a forgotten, ancient relic of the past.

Not anymore...

This project aims towards the recreation of the new features of the "obscured and updated" Monster Mod plugin. Taking botman's original 2002 plugin and working up from the ground, and to rebuild it with the new features and monsters that only few were able to see.

The source code is completely free to use for everyone: In the event that the development of this new project falls and becomes stagnant again, the plugin will live on, as the project's second goal is it's preservation. The original botman's page where you can download the 2002 plugin will not stay up forever.

Under no circumstances we shall allow this project to fade away and become lost amidst the gears of time.

## Milestones

Attempting to recreate everything in one go is a daunting task.
Let it be known that the original 2002 source code will NOT compile on today's compilers, and does NOT contain all the necessary files for compilation. The preliminary was to rewrite and provide as many files or lines of code to ensure it can compile again, and be usable on an actual HLDS installation.

Currently, I aim working this for linux only. While the original 2002 Visual C++ DSP file exists in the repository, it has not been updated to newer formats. Who knows if it can even open on newer Visual Studio versions, let alone, to compile?

Even so, in linux, I had to resort to compile under GCC 4.8 for the plugin to work. While I could use a newer compiler (GCC 9 at the time of this writing), whenever or not the plugin will run is dependant on the HLDS installation and the system libraries is it hosted with. If you compile with newer GCC compilers, be warned that it may refuse to load, as it will complain about libstdc++ not having CXXABI_1.X.X or similar.

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
- Implement HUD info about the monsters, along with the "displayname" keyvalue.
- Implement custom monster classification, the "classify" keyvalue.

### Tier 3

- Implement *-almost-* all Opposing Force monsters.
- Implement *-almost-* all default Sven Co-op monsters.
- Add configurations to change AI behaviour.

### Tier 4

- Custom model support, along with scaling.
- Custom sound support.
- Implement extra entities to enhance map gameplay.

### Tier 5

- Update source code so it can compile AND run **ON WINDOWS**.


What will the future hold after all Tiers has been completed?
