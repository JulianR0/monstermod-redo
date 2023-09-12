#pragma semicolon 1

#include <amxmodx>
#include <fakemeta>

// List of sounds that must NOT be precached
new g_SoundList[][64] =
{
	"weapons/ak47_boltpull.wav",
	"weapons/ak47_clipin.wav",
	"weapons/ak47_clipout.wav",
	"weapons/aug_boltpull.wav",
	"weapons/aug_boltslap.wav",
	"weapons/aug_clipin.wav",
	"weapons/aug_clipout.wav",
	"weapons/aug_forearm.wav",
	"weapons/awp_clipin.wav",
	"weapons/awp_clipout.wav",
	"weapons/awp_deploy.wav",
	"weapons/boltdown.wav",
	"weapons/boltpull1.wav",
	"weapons/boltup.wav",
	"weapons/clipin1.wav",
	"weapons/clipout1.wav",
	"weapons/de_clipin.wav",
	"weapons/de_clipout.wav",
	"weapons/de_deploy.wav",
	"weapons/elite_clipout.wav",
	"weapons/elite_deploy.wav",
	"weapons/elite_leftclipin.wav",
	"weapons/elite_reloadstart.wav",
	"weapons/elite_rightclipin.wav",
	"weapons/elite_sliderelease.wav",
	"weapons/elite_twirl.wav",
	"weapons/famas_boltpull.wav",
	"weapons/famas_boltslap.wav",
	"weapons/famas_clipin.wav",
	"weapons/famas_clipout.wav",
	"weapons/famas_forearm.wav",
	"weapons/fiveseven_clipin.wav",
	"weapons/fiveseven_clipout.wav",
	"weapons/fiveseven_slidepull.wav",
	"weapons/fiveseven_sliderelease.wav",
	"weapons/g3sg1_clipin.wav",
	"weapons/g3sg1_clipout.wav",
	"weapons/g3sg1_slide.wav",
	"weapons/galil_boltpull.wav",
	"weapons/galil_clipin.wav",
	"weapons/galil_clipout.wav",
	"weapons/m4a1_boltpull.wav",
	"weapons/m4a1_clipin.wav",
	"weapons/m4a1_clipout.wav",
	"weapons/m4a1_deploy.wav",
	"weapons/m4a1_silencer_off.wav",
	"weapons/m4a1_silencer_on.wav",
	"weapons/m249_boxin.wav",
	"weapons/m249_boxout.wav",
	"weapons/m249_chain.wav",
	"weapons/m249_coverdown.wav",
	"weapons/m249_coverup.wav",
	"weapons/mac10_boltpull.wav",
	"weapons/mac10_clipin.wav",
	"weapons/mac10_clipout.wav",
	"weapons/mp5_clipin.wav",
	"weapons/mp5_clipout.wav",
	"weapons/mp5_slideback.wav",
	"weapons/p90_boltpull.wav",
	"weapons/p90_clipin.wav",
	"weapons/p90_clipout.wav",
	"weapons/p90_cliprelease.wav",
	"weapons/p228_clipin.wav",
	"weapons/p228_clipout.wav",
	"weapons/p228_slidepull.wav",
	"weapons/p228_sliderelease.wav",
	"weapons/scout_bolt.wav",
	"weapons/scout_clipin.wav",
	"weapons/scout_clipout.wav",
	"weapons/sg550_boltpull.wav",
	"weapons/sg550_clipin.wav",
	"weapons/sg550_clipout.wav",
	"weapons/sg552_boltpull.wav",
	"weapons/sg552_clipin.wav",
	"weapons/sg552_clipout.wav",
	"weapons/slideback1.wav",
	"weapons/sliderelease1.wav",
	"weapons/ump45_boltslap.wav",
	"weapons/ump45_clipin.wav",
	"weapons/ump45_clipout.wav",
	"weapons/usp_clipin.wav",
	"weapons/usp_clipout.wav",
	"weapons/usp_silencer_off.wav",
	"weapons/usp_silencer_on.wav",
	"weapons/usp_slideback.wav",
	"weapons/usp_sliderelease.wav"
};

public plugin_init() 
{
	register_plugin( "CS-MONSTER: Unprecacher", "1.0", "Giegue" );
}

public plugin_precache()
{
	register_forward( FM_PrecacheSound, "fw_PrecacheSound" );
}

public fw_PrecacheSound( const sound[] )
{
	static bool:bBlock, i;
	bBlock = false;
	i = 0;
	
	for ( i = 0; i < sizeof( g_SoundList ); i++ )
	{
		if (contain( sound, g_SoundList[ i ] ) != -1 )
		{
			bBlock = true;
			break;
		}
	}
	
	if ( bBlock )
		return FMRES_SUPERCEDE;
	
	return FMRES_IGNORED;
}
