// CZ:DS weapons (Counter-Strike set) built on the featureful configurable weapon framework.
// Numbers (damage, cycle times, spread, kickback) follow ReGameDLL_CS / Velaron cs16-client.
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "skill.h"
#include "weapons.h"
#include "player.h"
#if !CLIENT_DLL
#include "explode.h"
#endif

enum czusp_e
{
	CZUSP_IDLE, CZUSP_SHOOT1, CZUSP_SHOOT2, CZUSP_SHOOT3, CZUSP_SHOOT_EMPTY, CZUSP_RELOAD, CZUSP_DRAW, CZUSP_ATTACH_SILENCER,
	CZUSP_UNSIL_IDLE, CZUSP_UNSIL_SHOOT1, CZUSP_UNSIL_SHOOT2, CZUSP_UNSIL_SHOOT3, CZUSP_UNSIL_SHOOT_EMPTY, CZUSP_UNSIL_RELOAD,
	CZUSP_UNSIL_DRAW, CZUSP_DETACH_SILENCER
};

enum czm4a1_e
{
	CZM4A1_IDLE, CZM4A1_SHOOT1, CZM4A1_SHOOT2, CZM4A1_SHOOT3, CZM4A1_RELOAD, CZM4A1_DRAW, CZM4A1_ATTACH_SILENCER,
	CZM4A1_UNSIL_IDLE, CZM4A1_UNSIL_SHOOT1, CZM4A1_UNSIL_SHOOT2, CZM4A1_UNSIL_SHOOT3, CZM4A1_UNSIL_RELOAD, CZM4A1_UNSIL_DRAW,
	CZM4A1_DETACH_SILENCER
};

enum czawp_e { CZAWP_IDLE, CZAWP_SHOOT, CZAWP_SHOOT2, CZAWP_SHOOT3, CZAWP_RELOAD, CZAWP_DRAW };
enum czaug_e { CZAUG_IDLE1, CZAUG_RELOAD, CZAUG_DRAW, CZAUG_SHOOT1, CZAUG_SHOOT2, CZAUG_SHOOT3 };

static WeaponKickBackProfile::RuleList CzKick(float vb, float lb, float vm, float lm, float vmax, float lmax,
	float dvb, float dlb, float dvm, float dlm, float dvmax, float dlmax)
{
	// standing/moving share the base profile, ducking is calmer, in air is wild (CS)
	WeaponKickBack air(vb * 2.0f, lb * 1.4f, vm, lm * 8.0f, vmax * 1.5f, lmax * 1.6f, 6, 5);
	WeaponKickBack duck(dvb, dlb, dvm, dlm, dvmax, dlmax, 7, 7);
	WeaponKickBack stand(vb, lb, vm, lm, vmax, lmax, 7, 7);
	return WeaponKickBackProfile::RuleList{
		WeaponKickBackRule{PlayerMovementConditions::InAir(), air},
		WeaponKickBackRule{PlayerMovementConditions::Ducking(), duck},
		WeaponKickBackRule{PlayerMovementConditions(), stand}
	};
}

static void CzRifleSpread(WeaponParameters& params, bool alt, float stillFactor)
{
	WeaponSpread::RuleList rules = {
		WeaponSpreadRule::Dynamic(0.035f, 0.4f, PlayerMovementConditions::InAir()),
		WeaponSpreadRule::Dynamic(0.035f, 0.07f, PlayerMovementConditions::Moving(140.0f)),
		WeaponSpreadRule::Dynamic(0.0f, stillFactor, PlayerMovementConditions())
	};
	params.fire.spread.SetInaccuracyAuto(alt, 0.2f, 1.0f, 0.3f, false, 1.0f, rules);
}

static void CzPistolSpread(WeaponParameters& params, bool alt, float air, float moving, float duck, float stand)
{
	WeaponSpread::RuleList rules = {
		WeaponSpreadRule::Dynamic(0.0f, air, PlayerMovementConditions::InAir()),
		WeaponSpreadRule::Dynamic(0.0f, moving, PlayerMovementConditions::Moving(0.0f)),
		WeaponSpreadRule::Dynamic(0.0f, duck, PlayerMovementConditions::Ducking()),
		WeaponSpreadRule::Dynamic(0.0f, stand, PlayerMovementConditions())
	};
	params.fire.spread.SetInaccuracyRecovering(alt, 0.08f, 0.3f, 0.275f, 0.4f, rules);
}

// ---------------------------------------------------------------------------
// weapon_usp  (silenced mode = main, like the CZ training pistol; secondary toggles the silencer)
// ---------------------------------------------------------------------------
class CCzUsp : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_USP; }
	bool GetItemInfo(ItemInfo *p) override
	{
		p->iSlot = 1;
		p->iPosition = 0;
		return true;
	}
	WeaponParameters GetDefaultParameters() const override;
};
LINK_WEAPON_TO_CLASS(weapon_usp, CCzUsp)

WeaponParameters CCzUsp::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 12;
	params.maxClip = 12;
	params.ammoName = "45acp";

	params.worldModel = "models/w_usp.mdl";
	params.viewModel = "models/v_usp.mdl";
	params.playerModel = "models/p_usp.mdl";
	params.playerAnimExt = "onehanded";
	params.priority = 10;
	params.mirrorViewModel = true;

	params.modelSounds = {"weapons/usp_unsil-1.wav", "weapons/usp1.wav", "weapons/usp2.wav", "weapons/usp_clipout.wav",
		"weapons/usp_clipin.wav", "weapons/usp_silencer_on.wav", "weapons/usp_silencer_off.wav",
		"weapons/usp_sliderelease.wav", "weapons/usp_slideback.wav"};
	params.modelSoundsDefined = true;

	// main = silencer attached
	params.deploy.animIndex.main = CZUSP_DRAW;
	params.deploy.animIndex.alt = CZUSP_UNSIL_DRAW;
	params.deploy.duration = 1.0f;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{WeaponParameters::IdleAnim{CZUSP_IDLE, 1.0f, 60.0f}};
	params.idleAnims.alt = WeaponParameters::IdleAnimArray{WeaponParameters::IdleAnim{CZUSP_UNSIL_IDLE, 1.0f, 60.0f}};

	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damageInfo.main.damage = FloatRange(30.0f);
	params.fire.rangeModifier.main = 0.79f;
	params.fire.rangeModifier.alt = 0.79f;
	params.fire.bulletDistance = 4096;
	params.fire.anims.main = {CZUSP_SHOOT1, CZUSP_SHOOT2, CZUSP_SHOOT3};
	params.fire.anims.mainEmptied = {CZUSP_SHOOT_EMPTY};
	params.fire.anims.alt = WeaponParameters::FireAnimArray{CZUSP_UNSIL_SHOOT1, CZUSP_UNSIL_SHOOT2, CZUSP_UNSIL_SHOOT3};
	params.fire.anims.altEmptied = WeaponParameters::FireAnimArray{CZUSP_UNSIL_SHOOT_EMPTY};
	params.fire.sound = {CHAN_WEAPON, {"weapons/usp1.wav", "weapons/usp2.wav"}, 1.0f, ATTN_NORM, PITCH_NORM};
	params.fire.sound.alt = WeaponSoundScript(CHAN_WEAPON, {"weapons/usp_unsil-1.wav"}, 1.0f, ATTN_NORM, PITCH_NORM);

	CzPistolSpread(params, false, 1.3f, 0.25f, 0.125f, 0.15f);
	CzPistolSpread(params, true, 1.2f, 0.225f, 0.08f, 0.1f);
	params.fire.cycleTime = 0.15f;
	params.fire.semiAuto = true;
	params.fire.allowUnderwater = true;
	params.fire.muzzleFlash.main = false;
	params.fire.muzzleFlash.alt = true;
	params.fire.weaponVolume.main = QUIET_GUN_VOLUME;
	params.fire.weaponVolume.alt = NORMAL_GUN_VOLUME;
	params.fire.weaponFlash = DIM_GUN_FLASH;
	params.fire.idleDelay = 60.0f;
	params.fire.kickBack.SetKickBack(false, 0.0f);
	params.fire.kickBack.SetKickBack(true, 2.0f, 0.0f);
	params.fire.clientPunchPitch = FloatRange(0.0f);

	params.fire.shellModel = "models/pshell.mdl";
	params.fire.shellSound = TE_BOUNCE_SHELL;
	params.fire.shellAttachment = 2;
	params.fire.shellLeftSide = true;

	// Alt = silencer off/on
	params.secondaryFireType = SecondaryFireType::SWITCH_MODE;
	params.altMode.animIndex = CZUSP_DETACH_SILENCER;
	params.altMode.attackDelay = 3.0f;
	params.altMode.endAnimIndex = CZUSP_ATTACH_SILENCER;
	params.altMode.endAnimDuration = 3.0f;
	params.fire.switchModeDelay = 3.0f;

	params.reload.animIndex.main = {WeaponParameters::ReloadAnim(CZUSP_RELOAD)};
	params.reload.animIndex.alt = WeaponParameters::ReloadAnimArray{WeaponParameters::ReloadAnim(CZUSP_UNSIL_RELOAD)};
	params.reload.duration = 2.7f;
	params.reload.idleDelay = FloatRange(60.0f);

	params.dropAmmo.classname = "ammo_45acp";
	return std::move(params);
}

// ---------------------------------------------------------------------------
// weapon_m4a1
// ---------------------------------------------------------------------------
class CCzM4A1 : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_M4A1; }
	bool GetItemInfo(ItemInfo *p) override
	{
		p->iSlot = 2;
		p->iPosition = 0;
		return true;
	}
	WeaponParameters GetDefaultParameters() const override;
};
LINK_WEAPON_TO_CLASS(weapon_m4a1, CCzM4A1)

WeaponParameters CCzM4A1::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 30;
	params.maxClip = 30;
	params.ammoName = "556nato";

	params.worldModel = "models/w_m4a1.mdl";
	params.viewModel = "models/v_m4a1.mdl";
	params.playerModel = "models/p_m4a1.mdl";
	params.playerAnimExt = "rifle";
	params.priority = 25;
	params.mirrorViewModel = true;

	params.modelSounds = {"weapons/m4a1_unsil-1.wav", "weapons/m4a1_unsil-2.wav", "weapons/m4a1-1.wav", "weapons/m4a1_clipin.wav",
		"weapons/m4a1_clipout.wav", "weapons/m4a1_boltpull.wav", "weapons/m4a1_deploy.wav", "weapons/m4a1_silencer_on.wav",
		"weapons/m4a1_silencer_off.wav"};
	params.modelSoundsDefined = true;

	// main = silencer attached (CZ training gives a silenced M4A1)
	params.deploy.animIndex.main = CZM4A1_DRAW;
	params.deploy.animIndex.alt = CZM4A1_UNSIL_DRAW;
	params.deploy.duration = 1.0f;

	params.idleAnims.main = WeaponParameters::IdleAnimArray{WeaponParameters::IdleAnim{CZM4A1_IDLE, 1.0f, 60.0f}};
	params.idleAnims.alt = WeaponParameters::IdleAnimArray{WeaponParameters::IdleAnim{CZM4A1_UNSIL_IDLE, 1.0f, 60.0f}};

	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damageInfo.main.damage = FloatRange(33.0f);
	params.fire.rangeModifier.main = 0.95f;
	params.fire.rangeModifier.alt = 0.97f;
	params.fire.anims.main = {CZM4A1_SHOOT1, CZM4A1_SHOOT2, CZM4A1_SHOOT3};
	params.fire.anims.alt = WeaponParameters::FireAnimArray{CZM4A1_UNSIL_SHOOT1, CZM4A1_UNSIL_SHOOT2, CZM4A1_UNSIL_SHOOT3};
	params.fire.sound = {CHAN_WEAPON, {"weapons/m4a1-1.wav"}, 1.0f, ATTN_NORM, PITCH_NORM};
	params.fire.sound.alt = WeaponSoundScript(CHAN_WEAPON, {"weapons/m4a1_unsil-1.wav", "weapons/m4a1_unsil-2.wav"}, 1.0f, ATTN_NORM, PITCH_NORM);

	CzRifleSpread(params, false, 0.025f);
	CzRifleSpread(params, true, 0.02f);
	params.fire.cycleTime = 0.0875f;
	params.fire.allowUnderwater = true;
	params.fire.muzzleFlash.main = false;
	params.fire.muzzleFlash.alt = true;
	params.fire.weaponVolume.main = QUIET_GUN_VOLUME;
	params.fire.weaponVolume.alt = NORMAL_GUN_VOLUME;
	params.fire.weaponFlash = DIM_GUN_FLASH;
	params.fire.idleDelay = 60.0f;
	params.fire.clientPunchPitch = FloatRange(0.0f);

	// CS KickBack(up, lateral, upMod, latMod, upMax, latMax, dir): still, air, moving, ducking
	params.fire.kickBack.SetKickBack(false, CzKick(0.65f, 0.35f, 0.25f, 0.015f, 3.5f, 2.25f, 0.6f, 0.3f, 0.2f, 0.0125f, 3.25f, 2.0f));

	params.fire.shellModel = "models/rshell.mdl";
	params.fire.shellSound = TE_BOUNCE_SHELL;
	params.fire.shellAttachment = 2;
	params.fire.shellLeftSide = true;

	params.secondaryFireType = SecondaryFireType::SWITCH_MODE;
	params.altMode.animIndex = CZM4A1_DETACH_SILENCER;
	params.altMode.attackDelay = 2.0f;
	params.altMode.endAnimIndex = CZM4A1_ATTACH_SILENCER;
	params.altMode.endAnimDuration = 2.0f;
	params.fire.switchModeDelay = 2.0f;

	params.reload.animIndex.main = {WeaponParameters::ReloadAnim(CZM4A1_RELOAD)};
	params.reload.animIndex.alt = WeaponParameters::ReloadAnimArray{WeaponParameters::ReloadAnim(CZM4A1_UNSIL_RELOAD)};
	params.reload.duration = 3.05f;
	params.reload.idleDelay = FloatRange(60.0f);

	params.playerMaxSpeed.main.value = 230.0f / 250.0f;
	params.playerMaxSpeed.main.isFactor = true;

	params.dropAmmo.classname = "ammo_556nato";
	return std::move(params);
}

// ---------------------------------------------------------------------------
// weapon_aug
// ---------------------------------------------------------------------------
class CCzAug : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_AUG; }
	bool GetItemInfo(ItemInfo *p) override
	{
		p->iSlot = 2;
		p->iPosition = 1;
		return true;
	}
	WeaponParameters GetDefaultParameters() const override;
};
LINK_WEAPON_TO_CLASS(weapon_aug, CCzAug)

WeaponParameters CCzAug::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 30;
	params.maxClip = 30;
	params.ammoName = "556nato";

	params.worldModel = "models/w_aug.mdl";
	params.viewModel = "models/v_aug.mdl";
	params.playerModel = "models/p_aug.mdl";
	params.playerAnimExt = "carbine";
	params.priority = 25;
	params.mirrorViewModel = true;

	params.modelSounds = {"weapons/aug-1.wav", "weapons/aug_clipout.wav", "weapons/aug_clipin.wav", "weapons/aug_boltpull.wav",
		"weapons/aug_boltslap.wav", "weapons/aug_forearm.wav"};
	params.modelSoundsDefined = true;

	params.deploy.animIndex = CZAUG_DRAW;
	params.deploy.duration = 1.0f;
	params.idleAnims.main = WeaponParameters::IdleAnimArray{WeaponParameters::IdleAnim{CZAUG_IDLE1, 1.0f, 60.0f}};

	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damageInfo.main.damage = FloatRange(32.0f);
	params.fire.rangeModifier = 0.96f;
	params.fire.anims.main = {CZAUG_SHOOT1, CZAUG_SHOOT2, CZAUG_SHOOT3};
	params.fire.sound = {CHAN_WEAPON, {"weapons/aug-1.wav"}, 1.0f, ATTN_NORM, PITCH_NORM};

	CzRifleSpread(params, false, 0.02f);
	params.fire.cycleTime = 0.0825f;
	params.fire.allowUnderwater = true;
	params.fire.muzzleFlash = true;
	params.fire.weaponVolume = NORMAL_GUN_VOLUME;
	params.fire.weaponFlash = NORMAL_GUN_FLASH;
	params.fire.idleDelay = 60.0f;
	params.fire.clientPunchPitch = FloatRange(0.0f);

	params.fire.kickBack.SetKickBack(false, CzKick(0.625f, 0.375f, 0.25f, 0.0125f, 3.5f, 2.25f, 0.575f, 0.325f, 0.2f, 0.011f, 3.25f, 2.0f));

	params.fire.shellModel = "models/rshell.mdl";
	params.fire.shellSound = TE_BOUNCE_SHELL;
	params.fire.shellAttachment = 2;

	// scope: secondary toggles zoom, firing rate slows down while zoomed in CS (0.135)
	params.secondaryFireType = SecondaryFireType::SWITCH_MODE;
	params.altMode.zoomFOV = 55;
	params.altMode.attackDelay = 0.3f;
	params.altMode.zoomSound.waves = {"weapons/zoom.wav"};
	params.fire.cycleTime.alt = 0.135f;

	params.reload.animIndex = {WeaponParameters::ReloadAnim(CZAUG_RELOAD)};
	params.reload.duration = 3.3f;
	params.reload.idleDelay = FloatRange(60.0f);

	params.playerMaxSpeed.main.value = 240.0f / 250.0f;
	params.playerMaxSpeed.main.isFactor = true;

	params.dropAmmo.classname = "ammo_556nato";
	return std::move(params);
}

// ---------------------------------------------------------------------------
// weapon_awp
// ---------------------------------------------------------------------------
class CCzAwp : public CConfigurableWeapon
{
public:
	int WeaponId() const override { return WEAPON_AWP; }
	bool GetItemInfo(ItemInfo *p) override
	{
		p->iSlot = 2;
		p->iPosition = 2;
		return true;
	}
	WeaponParameters GetDefaultParameters() const override;
};
LINK_WEAPON_TO_CLASS(weapon_awp, CCzAwp)

WeaponParameters CCzAwp::GetDefaultParameters() const
{
	WeaponParameters params;

	params.initialAmmoAmount = 10;
	params.maxClip = 10;
	params.ammoName = "338magnum";

	params.worldModel = "models/w_awp.mdl";
	params.viewModel = "models/v_awp.mdl";
	params.playerModel = "models/p_awp.mdl";
	params.playerAnimExt = "rifle";
	params.priority = 30;
	params.mirrorViewModel = true;

	params.modelSounds = {"weapons/awp1.wav", "weapons/boltpull1.wav", "weapons/boltup.wav", "weapons/boltdown.wav",
		"weapons/awp_deploy.wav", "weapons/awp_clipin.wav", "weapons/awp_clipout.wav"};
	params.modelSoundsDefined = true;

	params.deploy.animIndex = CZAWP_DRAW;
	params.deploy.duration = 1.0f;
	params.idleAnims.main = WeaponParameters::IdleAnimArray{WeaponParameters::IdleAnim{CZAWP_IDLE, 1.0f, 60.0f}};

	params.fire.fireType = WeaponParameters::Fire::BULLETS;
	params.fire.damageInfo.main.damage = FloatRange(115.0f);
	params.fire.rangeModifier = 0.99f;
	params.fire.anims.main = {CZAWP_SHOOT, CZAWP_SHOOT2, CZAWP_SHOOT3};
	params.fire.sound = {CHAN_WEAPON, {"weapons/awp1.wav"}, 1.0f, ATTN_NORM, PITCH_NORM};

	// still+zoomed is pin-point, unscoped / moving is wide (CS values)
	WeaponSpread::RuleList rules;
	rules.push_back(WeaponSpreadRule::Static(0.85f, PlayerMovementConditions::InAir()));
	rules.push_back(WeaponSpreadRule::Static(0.25f, PlayerMovementConditions::Moving(140.0f)));
	rules.push_back(WeaponSpreadRule::Static(0.1f, PlayerMovementConditions::Moving(10.0f)));
	rules.push_back(WeaponSpreadRule::Static(0.0f, PlayerMovementConditions::Ducking()));
	rules.push_back(WeaponSpreadRule::Static(0.001f, PlayerMovementConditions()));
	params.fire.spread.SetStaticSpread(false, rules);
	params.fire.cycleTime = 1.45f;
	params.fire.allowUnderwater = true;
	params.fire.muzzleFlash = true;
	params.fire.weaponVolume = LOUD_GUN_VOLUME;
	params.fire.weaponFlash = NORMAL_GUN_FLASH;
	params.fire.idleDelay = 60.0f;
	params.fire.bulletDistance = 8192;
	params.fire.clientPunchPitch = FloatRange(0.0f);
	params.fire.kickBack.SetKickBack(false, 2.0f, 0.0f);

	params.fire.shellModel = "models/rshell_big.mdl";
	params.fire.shellSound = TE_BOUNCE_SHELL;
	params.fire.shellAttachment = 2;
	params.fire.shellEjectDelay = 0.55f;

	params.secondaryFireType = SecondaryFireType::SWITCH_MODE;
	params.altMode.zoomFOV = 40;
	params.altMode.zoomFOV2 = 10;
	params.altMode.resetZoomOnFire = true;
	params.altMode.resumeZoomAfterReset = true;
	params.altMode.hideViewModelOnZoom = true;
	params.altMode.attackDelay = 0.3f;
	params.altMode.zoomSound.waves = {"weapons/zoom.wav"};
	params.altMode.zoomSound2.waves = {"weapons/zoom.wav"};
	params.altMode.unzoomSound.waves = {"weapons/zoom.wav"};

	params.reload.animIndex = {WeaponParameters::ReloadAnim(CZAWP_RELOAD)};
	params.reload.duration = 2.5f;
	params.reload.idleDelay = FloatRange(60.0f);

	params.playerMaxSpeed.main.value = 210.0f / 250.0f;
	params.playerMaxSpeed.main.isFactor = true;

	params.dropAmmo.classname = "ammo_338magnum";
	return std::move(params);
}

// ---------------------------------------------------------------------------
// Grenades: he / flashbang / smoke  (CS anims: 0 idle, 1 pullpin, 2 throw, 3 deploy)
// ---------------------------------------------------------------------------
static WeaponParameters CzGrenadeParams(const char* ammo, int maxCarry, const char* w, const char* v, const char* p, const char* projectile)
{
	WeaponParameters params;
	params.initialAmmoAmount = 1;
	params.maxClip = WEAPON_NOCLIP;
	params.ammoName = ammo;
	params.worldModel = w;
	params.viewModel = v;
	params.playerModel = p;
	params.playerAnimExt = "grenade";
	params.priority = 5;
	params.mirrorViewModel = true;

	params.deploy.animIndex = 3;
	params.idleAnims.main = WeaponParameters::IdleAnimArray{WeaponParameters::IdleAnim{0, 1.0f, FloatRange(10.0f, 15.0f)}};

	params.fire.fireType = WeaponParameters::Fire::PROJECTILE;
	params.fire.anims = {2};
	params.fire.cycleTime = 0.5f;
	params.fire.idleDelay = 0.5f;
	params.fire.chargeAnims = {1};
	params.fire.chargeTime = 1.0f;
	params.fire.allowHolsterDuringCharge = false;
	params.fire.chargeEachFire = true;
	params.fire.cooldownAnims = {3};
	params.fire.cooldownTime = 0.5f;
	params.fire.projectileName = projectile;
	params.fire.projectileOffsetForward = 16.0f;
	params.fire.projectileAddCurrentVelocity = WeaponParameters::Fire::ADD_VELOCITY_ABSOLUTE;
	params.fire.projectileRespectPunchangle = true;
	params.fire.projectileDetonationTime = 1.5f;
	params.fire.projectileDetonationCooked = false;
	params.fire.projectileGrenadePhysics = WeaponParameters::Fire::GRENADEPHYS_AUTO;
	params.dropAmmo.classname = "";
	params.exhausitble = true;
	(void)maxCarry;
	return params;
}

#define CZ_GRENADE_CLASS(CLS, WID, SLOTPOS, NAME, AMMO, MAXC, MDL, PROJ) \
class CLS : public CConfigurableWeapon \
{ \
public: \
	int WeaponId() const override { return WID; } \
	bool GetItemInfo(ItemInfo *p) override { p->iSlot = 3; p->iPosition = SLOTPOS; return true; } \
	WeaponParameters GetDefaultParameters() const override \
	{ \
		return CzGrenadeParams(AMMO, MAXC, "models/w_" MDL ".mdl", "models/v_" MDL ".mdl", "models/p_" MDL ".mdl", PROJ); \
	} \
}; \
LINK_WEAPON_TO_CLASS(NAME, CLS)

CZ_GRENADE_CLASS(CCzHeGrenade, WEAPON_HEGRENADE, 0, weapon_hegrenade, "HEGrenade", 1, "hegrenade", "cz he")
CZ_GRENADE_CLASS(CCzFlashbang, WEAPON_FLASHBANG, 1, weapon_flashbang, "Flashbang", 2, "flashbang", "cz flashbang")
CZ_GRENADE_CLASS(CCzSmokeGrenade, WEAPON_SMOKEGRENADE, 2, weapon_smokegrenade, "SmokeGrenade", 1, "smokegrenade", "cz smoke")


// ---------------------------------------------------------------------------------------------
// Radio controlled stick-on bomb (train02). One model v_rcontrolbomb.mdl holds both the bomb and the detonator.
//   0 idle bomb, 1 idle detonator, 2 draw bomb, 3 draw detonator, 4 holster bomb, 5 holster detonator, 6 plant, 7 detonate
// Attack1 in the tool zone plants the bomb, then Attack1 with the detonator blows it up. Attack2 swaps bomb <-> detonator.
// ---------------------------------------------------------------------------------------------
enum czrc_e { RC_IDLE_BOMB, RC_IDLE_DET, RC_DRAW_BOMB, RC_DRAW_DET, RC_HOLSTER_BOMB, RC_HOLSTER_DET, RC_PLANT, RC_DETONATE };

#if !CLIENT_DLL
class CCzRcPlanted : public CBaseAnimating
{
public:
	void Spawn() override
	{
		PRECACHE_MODEL("models/w_rcontrolbomb_bomb.mdl");
		SET_MODEL(ENT(pev), "models/w_rcontrolbomb_bomb.mdl");
		pev->movetype = MOVETYPE_NONE;
		pev->solid = SOLID_NOT;
		UTIL_SetOrigin(pev, pev->origin);
	}
	void Precache() override { PRECACHE_MODEL("models/w_rcontrolbomb_bomb.mdl"); }
	int ObjectCaps() override { return CBaseAnimating::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};
LINK_ENTITY_TO_CLASS(cz_rc_planted, CCzRcPlanted)
#endif

class CCzRcBomb : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	static TYPEDESCRIPTION m_SaveData[];
#endif
	int WeaponId() const override { return WEAPON_RCBOMB; }
	bool GetItemInfo(ItemInfo *p) override { p->iSlot = 4; p->iPosition = 2; p->iFlags = ITEM_FLAG_SELECTONEMPTY; return true; }
	WeaponParameters GetDefaultParameters() const override
	{
		WeaponParameters params;
		params.initialAmmoAmount = 5;
		params.maxClip = WEAPON_NOCLIP;
		params.ammoName = "RCBomb";
		params.worldModel = "models/w_rcontrolbomb.mdl";
		params.viewModel = "models/v_rcontrolbomb.mdl";
		params.playerAnimExt = "trip";
		params.priority = -10;
		params.toolIcon = "icon_radiocontrolledbomb";
		return params;
	}
	void Spawn() override
	{
		Precache();
		SetMyModel();
		SetInitialAmmoAmount();
		InitMaxClip();
		FallInit();
		SetWorldModelProps();
	}
	void Precache() override
	{
		PrecacheWeaponModels();
		PrecacheModelSounds();
#if !CLIENT_DLL
		PRECACHE_MODEL("models/w_rcontrolbomb_bomb.mdl");
		PRECACHE_MODEL("sprites/zerogxplode.spr");
		UTIL_PrecacheOther("cz_rc_planted");
#endif
	}
	bool AddToPlayer(CBasePlayer *pPlayer) override
	{
		bool r = CBasePlayerWeapon::AddToPlayer(pPlayer);
		m_rcMode = 0; m_rcPhase = 0; m_rcPlanted = 0;
		return r;
	}
	bool CanDeploy() override { return m_rcPlanted || CBasePlayerWeapon::CanDeploy(); }
	bool IsUseable() override { return CanDeploy(); }
	bool CanBeDropped() override { return false; }
	bool Deploy() override
	{
		const WeaponParameters& params = MyParameters();
		m_rcPhase = 0;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
		bool r = DefaultDeploy(ViewModelToDeploy(params.ViewModel()), params.PlayerModel(), m_rcMode ? RC_DRAW_DET : RC_DRAW_BOMB, params.playerAnimExt.c_str());
		if (r)
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5f;
		return r;
	}
	void Holster() override
	{
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
		SendWeaponAnim(m_rcMode ? RC_HOLSTER_DET : RC_HOLSTER_BOMB);
		m_rcPhase = 0;
	}
	bool InZone()
	{
		const int ti = MyParameters().toolIndex;
		return ti >= 0 && FBitSet(m_pPlayer->m_ToolStateBits, 1 << ti);
	}
	void PrimaryAttack() override
	{
		if (m_rcPhase != 0)
			return;
		if (m_rcMode == 0)
		{
			if (m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] <= 0 || !InZone())
			{
				m_flNextPrimaryAttack = GetNextAttackDelay(0.5f);
				return;
			}
			SendWeaponAnim(RC_PLANT);
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
			m_rcPhase = 1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.07f;
			m_flNextPrimaryAttack = GetNextAttackDelay(3.0f);
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 3.0f;
		}
		else
		{
			SendWeaponAnim(RC_DETONATE);
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
			m_rcPhase = 3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5f;
			m_flNextPrimaryAttack = GetNextAttackDelay(3.5f);
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 3.5f;
		}
	}
	void SecondaryAttack() override
	{
		if (m_rcPhase != 0)
			return;
		if (m_rcMode == 0 && m_rcPlanted == 0 && m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] <= 0)
			return;
		SendWeaponAnim(m_rcMode ? RC_HOLSTER_DET : RC_HOLSTER_BOMB);
		m_rcPhase = 5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.7f;
		m_flNextPrimaryAttack = GetNextAttackDelay(1.7f);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.7f;
	}
	void WeaponIdle() override
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		switch (m_rcPhase)
		{
		case 1: // the bomb is stuck on the target now
			if (!InZone())
			{
				// the player walked away during the animation: nothing is planted
				m_rcPhase = 0;
				SendWeaponAnim(RC_IDLE_BOMB);
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
				return;
			}
#if !CLIENT_DLL
			PlantBomb();
#endif
			m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]--;
			m_rcPlanted = 1;
			m_rcMode = 1;
			m_rcPhase = 2;
			SendWeaponAnim(RC_DRAW_DET);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.33f;
			return;
		case 2:
			m_rcPhase = 0;
			SendWeaponAnim(RC_IDLE_DET);
			break;
		case 3: // put the detonator away, the bang follows
			m_rcPhase = 4;
			SendWeaponAnim(RC_HOLSTER_DET);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.13f;
			return;
		case 4:
#if !CLIENT_DLL
			DetonateBombs();
#endif
			m_rcPlanted = 0;
			m_rcPhase = 6;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
			return;
		case 5: // swap bomb <-> detonator
			m_rcMode ^= 1;
			m_rcPhase = 0;
			SendWeaponAnim(m_rcMode ? RC_DRAW_DET : RC_DRAW_BOMB);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
			return;
		case 6: // after the explosion take the next bomb out
			m_rcPhase = 0;
			m_rcMode = 0;
			if (m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0)
			{
				SendWeaponAnim(RC_DRAW_BOMB);
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5f;
			}
			else
			{
				SendWeaponAnim(RC_IDLE_BOMB);
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
			}
			return;
		default:
			SendWeaponAnim(m_rcMode ? RC_IDLE_DET : RC_IDLE_BOMB);
			break;
		}
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	}
	void GetWeaponData(weapon_data_t& data) override
	{
		data.iuser1 = m_rcMode;
		data.iuser2 = m_rcPhase;
		data.iuser3 = m_rcPlanted;
	}
	void SetWeaponData(const weapon_data_t& data) override
	{
		m_rcMode = data.iuser1;
		m_rcPhase = data.iuser2;
		m_rcPlanted = data.iuser3;
	}

#if !CLIENT_DLL
	void PlantBomb()
	{
		const int ti = MyParameters().toolIndex;
		CBaseEntity* zone = ti >= 0 ? CBaseEntity::OwnInstance(m_pPlayer->m_UseToolTriggers[ti]) : nullptr;
		Vector pos = m_pPlayer->pev->origin + gpGlobals->v_forward * 32.0f;
		Vector ang = g_vecZero;
		if (zone && !FStringNull(zone->pev->noise))
		{
			CBaseEntity* guide = UTIL_FindEntityByTargetname(nullptr, STRING(zone->pev->noise));
			if (guide)
			{
				pos = guide->pev->origin;
				ang = guide->pev->angles;
				guide->pev->effects |= EF_NODRAW;
			}
		}
		CBaseEntity::Create("cz_rc_planted", pos, ang, m_pPlayer->edict());
		if (zone)
			zone->Use(m_pPlayer, m_pPlayer, USE_TOGGLE, 0.0f);
	}
	void DetonateBombs()
	{
		CBaseEntity* e = nullptr;
		while ((e = UTIL_FindEntityByClassname(e, "cz_rc_planted")) != nullptr)
		{
			ExplosionCreate(e->pev->origin, e->pev->angles, m_pPlayer->edict(), 60, false);
			UTIL_ScreenShake(e->pev->origin, 8.0f, 150.0f, 1.5f, 400.0f);
			UTIL_Remove(e);
		}
		// the shields that only the bomb can break
		CBaseEntity* b = nullptr;
		while ((b = UTIL_FindEntityByClassname(b, "func_breakable")) != nullptr)
		{
			if (b->pev->iuser4 == 2)
				b->Use(m_pPlayer, m_pPlayer, USE_TOGGLE, 0.0f);
		}
		CBaseEntity* g = nullptr;
		while ((g = UTIL_FindEntityByClassname(g, "item_generic")) != nullptr)
		{
			if (g->pev->model && strstr(STRING(g->pev->model), "rcontrolbomb_indicator"))
				UTIL_Remove(g);
		}
	}
#endif

	int m_rcMode;
	int m_rcPhase;
	int m_rcPlanted;
};

#if !CLIENT_DLL
TYPEDESCRIPTION CCzRcBomb::m_SaveData[] =
{
	DEFINE_FIELD(CCzRcBomb, m_rcMode, FIELD_INTEGER),
	DEFINE_FIELD(CCzRcBomb, m_rcPhase, FIELD_INTEGER),
	DEFINE_FIELD(CCzRcBomb, m_rcPlanted, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CCzRcBomb, CBasePlayerWeapon)
#endif

LINK_WEAPON_TO_CLASS(weapon_radiocontrolledbomb, CCzRcBomb)
