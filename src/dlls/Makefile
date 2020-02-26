# COMPILE ONLY WITH GCC 4.8!

CPP = g++
BASEFLAGS = -Dstricmp=strcasecmp -Dstrcmpi=strcasecmp -m32
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
	ggrenade.o \
	h_ai.o \
	h_export.o \
	hassassin.o \
	headcrab.o \
	hgrunt.o \
	hornet.o \
	houndeye.o \
	islave.o \
	monster_api.o \
	monster_config.o \
	monsters.o \
	monsterstate.o \
	nodes.o \
	scientist.o \
	skill.o \
	sound.o \
	squeakgrenade.o \
	subs.o \
	talkmonster.o \
	util.o \
	weapons.o \
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

