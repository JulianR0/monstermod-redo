CPP = g++
BASEFLAGS = -Dstricmp=strcasecmp -Dstrcmpi=strcasecmp -m32 -fPIC
OPTFLAGS = -O2
CPPFLAGS = ${BASEFLAGS} ${OPTFLAGS} -w -I. -I../engine -I../common -I../pm_shared -I../metamod

OBJ = \
	agrunt.o \
	AI_BaseNPC_Schedule.o \
	animating.o \
	animation.o \
	apache.o \
	barney.o \
	bigmomma.o \
	bullsquid.o \
	cmbase.o \
	combat.o \
	controller.o \
	defaultai.o \
	dllapi.o \
	effects.o \
	explode.o \
	flyingmonster.o \
	gargantua.o \
	ggrenade.o \
	globalreplace.o \
	gonome.o \
	h_ai.o \
	h_export.o \
	hassassin.o \
	headcrab.o \
	hgrunt.o \
	hornet.o \
	houndeye.o \
	hwgrunt.o \
	islave.o \
	massn.o \
	monster_api.o \
	monster_config.o \
	monstermaker.o \
	monsters.o \
	monsterstate.o \
	music.o \
	nodes.o \
	otis.o \
	pitdrone.o \
	rgrunt.o \
	ripent.o \
	scientist.o \
	shock.o \
	shockroach.o \
	skill.o \
	sound.o \
	sporegrenade.o \
	squeakgrenade.o \
	strooper.o \
	stukabat.o \
	subs.o \
	talkmonster.o \
	turret.o \
	util.o \
	voltigore.o \
	weapons.o \
	xenmaker.o \
	zombie.o

monster_mm_i386.so: ${OBJ}
	${CPP} -m32 -fPIC -shared -o $@ ${OBJ}

clean:
	-rm -f *.o
	-rm -f *.so	

%.o:	%.cpp
	${CPP} ${CPPFLAGS} -c $< -o $@

%.o:	%.c
	${CPP} ${CPPFLAGS} -c $< -o $@

