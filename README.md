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

The source code is completely free for everyone to use: In the event that the development of this project falls and becomes stagnant again, the plugin will live on, as the project's second goal is its preservation. The original botman's page where you can download the 2002 plugin will not stay up forever.

Under no circumstances shall we allow this project to fade away and become lost amidst the gears of time.

## Installation

*NOTE: Outdated. Ideally, users should be able to use the plugin "out-of-the-box" without the need to do the complicated mess explained below.*
*TODO: Add build instructions, just using 'make' on G++ 4.8 is really vague.*

If you are trying to use the compiled binary, you must know that it has been compiled with a mayor GCC version and it will be highly unlikely that it will run on a vanilla HLDS server. If the plugin fails to load due to libstdc++ not having CXXABI_1.X.X or similar, read on. HLDS uses it's own libstdc++ library. Any plugins compiled with GCC versions 5.x and greater will not work with the outdated library.

To remedy this issue you have two options:

You can recompile the source code under g++ 4.8 and use the newly generated binary. Make sure to edit the Makefile so it points to that version of g++. Compilation is done by simply running `make` on the `src/dlls` folder.

Alternatively, you can "remove" the outdated library to force HLDS to use the libstdc++ provided by the linux distro, which is generally more up to date. You might need to install GCC/G++ on the operating system if it doesn't work.

## MonsterMod and ReHLDS

Usage of ReHLDS is highly recommended, as you can use the command `rescount` which will reveal the current number of precached models and sounds. You can also use `reslist model` and `reslist sound` to see the entire list of precached content.

Keeping track of the number of precached content will allow you to maximize the number of monsters you can use without risking going over the limits.

## Using MonsterMod on Counter-Strike

Counter-Strike precaches non trivial sounds of all weapons. This means that sounds such as "clip-ins", "clip-outs" are added to the list, taking quite a bit of space in the precache count. Let it be a reminder that our good old Half-Life can only store a maximum of 512 precached resources. Since these sounds are handled client-side by the models themselves, theres is no need to be kept precached on the server. Only the weapons fire sounds are needed.

MonsterMod does not have an integrated "Unprecacher" to remove those sounds, but you can remove them manually with AMX Mod X, using Fakemeta. Register forward **FM_PrecacheSound** and return **FMRES_SUPERCEDE** on the following sounds:

```
"weapons/ak47_boltpull.wav"
"weapons/ak47_clipin.wav"
"weapons/ak47_clipout.wav"
"weapons/aug_boltpull.wav"
"weapons/aug_boltslap.wav"
"weapons/aug_clipin.wav"
"weapons/aug_clipout.wav"
"weapons/aug_forearm.wav"
"weapons/awp_clipin.wav"
"weapons/awp_clipout.wav"
"weapons/awp_deploy.wav"
"weapons/boltdown.wav"
"weapons/boltpull1.wav"
"weapons/boltup.wav"
"weapons/clipin1.wav"
"weapons/clipout1.wav"
"weapons/de_clipin.wav"
"weapons/de_clipout.wav"
"weapons/de_deploy.wav"
"weapons/elite_clipout.wav"
"weapons/elite_deploy.wav"
"weapons/elite_leftclipin.wav"
"weapons/elite_reloadstart.wav"
"weapons/elite_rightclipin.wav"
"weapons/elite_sliderelease.wav"
"weapons/elite_twirl.wav"
"weapons/famas_boltpull.wav"
"weapons/famas_boltslap.wav"
"weapons/famas_clipin.wav"
"weapons/famas_clipout.wav"
"weapons/famas_forearm.wav"
"weapons/fiveseven_clipin.wav"
"weapons/fiveseven_clipout.wav"
"weapons/fiveseven_slidepull.wav"
"weapons/fiveseven_sliderelease.wav"
"weapons/g3sg1_clipin.wav"
"weapons/g3sg1_clipout.wav"
"weapons/g3sg1_slide.wav"
"weapons/galil_boltpull.wav"
"weapons/galil_clipin.wav"
"weapons/galil_clipout.wav"
"weapons/m4a1_boltpull.wav"
"weapons/m4a1_clipin.wav"
"weapons/m4a1_clipout.wav"
"weapons/m4a1_deploy.wav"
"weapons/m4a1_silencer_off.wav"
"weapons/m4a1_silencer_on.wav"
"weapons/m249_boxin.wav"
"weapons/m249_boxout.wav"
"weapons/m249_chain.wav"
"weapons/m249_coverdown.wav"
"weapons/m249_coverup.wav"
"weapons/mac10_boltpull.wav"
"weapons/mac10_clipin.wav"
"weapons/mac10_clipout.wav"
"weapons/mp5_clipin.wav"
"weapons/mp5_clipout.wav"
"weapons/mp5_slideback.wav"
"weapons/p90_boltpull.wav"
"weapons/p90_clipin.wav"
"weapons/p90_clipout.wav"
"weapons/p90_cliprelease.wav"
"weapons/p228_clipin.wav"
"weapons/p228_clipout.wav"
"weapons/p228_slidepull.wav"
"weapons/p228_sliderelease.wav"
"weapons/scout_bolt.wav"
"weapons/scout_clipin.wav"
"weapons/scout_clipout.wav"
"weapons/sg550_boltpull.wav"
"weapons/sg550_clipin.wav"
"weapons/sg550_clipout.wav"
"weapons/sg552_boltpull.wav"
"weapons/sg552_clipin.wav"
"weapons/sg552_clipout.wav"
"weapons/slideback1.wav"
"weapons/sliderelease1.wav"
"weapons/ump45_boltslap.wav"
"weapons/ump45_clipin.wav"
"weapons/ump45_clipout.wav"
"weapons/usp_clipin.wav"
"weapons/usp_clipout.wav"
"weapons/usp_silencer_off.wav"
"weapons/usp_silencer_on.wav"
"weapons/usp_slideback.wav"
"weapons/usp_sliderelease.wav"
```

Doing this will free **85** sounds from the precache list that you can now use for additional monsters.

## Known Bugs

I'm aware that the plugin is far from perfect, and there are a few things that need fixing/will be fixing as the project evolves:

- Human Grunts are unable to reload their weapons. As a workaround, infinite ammo has been given to them.

- Male Assassins share the same AI as HGrunts, so their ammo problem is still a thing, and the same workaround is used.

- Shock Troopers seems to be broken despite sharing HGrunts AI code. Despite having infinite ammo, they eventually stop firing. Worse, taking cover is absolutely broken, and they remain completely frozen in place when it happens.

- If a Heavy Weapons Grunt is to lose their target while his minigun is still spinning, the next time it targets an enemy it will instantly fire instead of spinning up the minigun again.

## Milestones

Attempting to recreate everything in one go is a daunting task.
Let it be known that the original 2002 source code will NOT compile on today's compilers, and does NOT contain all the necessary files for compilation. The preliminary was to rewrite and provide as many files or lines of code to ensure it can compile again, and be usable on an actual HLDS installation.

The original Visual C++ 6.0 DSP file exists in the repository but neither the file nor the code has been updated to newer formats. Don't expect it to compile on modern Visual Studio versions.

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
- Implement *-almost-* all default Sven Co-op monsters.
- Custom model support.

### Tier 4

- Model scaling.
- Custom sound support.
- Add configurations to change AI behaviour.
- Fix all pending bugs.

### Tier 5

- Optimize code and enhance the AI.
- Create "tool" entities for easier map customization.


What will the future hold after all Tiers has been completed?
