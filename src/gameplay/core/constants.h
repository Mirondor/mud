/**************************************************************************
*  File: constants.h                                     Part of Bylins   *
*  Usage: header file for mud contstants.                                 *
*                                                                         *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
**************************************************************************/

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include "gameplay/classes/classes_constants.h"
#include "engine/entities/entities_constants.h"
#include "engine/structs/structs.h"
#include "engine/core/sysdep.h"
#include "engine/core/conf.h"

#include <vector>
#include <array>
#include <map>

struct IntApplies {
	int spell_aknowlege;    // drop_chance to know spell               //
	int to_skilluse;        // ADD CHANSE FOR USING SKILL         //
	int mana_per_tic;
	int spell_success;        //  max count of spell on 1s level    //
	int improve;        // drop_chance to improve skill           //
	int observation;        // drop_chance to use kAwake/CRITICAL //
};

struct ChaApplies {
	int leadership;
	int charms;
	int morale;
	int illusive;
	int dam_to_hit_rate;
};

struct SizeApplies {
	int ac;
	int interpolate;        // ADD VALUE FOR SOME SKILLS  //
	int initiative;
	int shocking;
};

struct WeaponApplies {
	int shocking;
	int bashing;
	int parrying;
};

struct PrayAffect {
	int metter;
	EApply location;
	int modifier;
	Bitvector bitvector;
	int battleflag;
};

struct ApplyNegative {
	std::string name;
	EApply location;
	ESaving savetype;
};

extern const char *circlemud_version;
extern const char *dirs_rus[];
extern const char *dirs[];
extern const char *DirsFrom[];
extern const char *DirsTo[];
extern const char *room_bits[];
extern const char *exit_bits[];
extern const char *sector_types[];
extern const char *genders[];
extern const char *position_types[];
extern const char *resistance_types[];
extern const char *player_bits[];
extern const char *action_bits[];
extern const char *preference_bits[];
extern const char *connected_types[];
extern const char *where[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const std::vector<ApplyNegative> apply_negative;
extern const std::map<ESaving, std::string> saving_name;
extern const char *weapon_affects[];
extern const char *anti_bits[];
extern const char *no_bits[];
extern const char *material_type[];
extern const char *container_bits[];
extern const char *fullness[];
extern const std::vector<const char *> npc_role_types;
extern const char *npc_race_types[];
extern const char *weekdays[];
extern const char *month_name[];
extern const char *weekdays_poly[];
extern const char *month_name_poly[];
extern const char *ingradient_bits[];
extern const char *magic_container_bits[];
extern const char *function_bits[];
extern const char *pray_metter[];
extern const char *pray_whom[];
extern const char *room_aff_visib_bits[];
extern const char *room_aff_invis_bits[];
extern const char *room_self_aff_invis_bits[];
extern const char *equipment_types[];
extern struct IntApplies int_app[];
extern const size_t INT_APP_SIZE;
extern struct ChaApplies cha_app[];
extern struct SizeApplies size_app[];
extern struct WeaponApplies weapon_app[];
extern std::vector<PrayAffect> pray_affect;
extern int rev_dir[];
extern int movement_loss[];
extern int mana[];
extern int mana_gain_cs[];
extern const char *material_name[];
extern struct AttackHitType attack_hit_text[];
extern const char *godslike_bits[];
extern const char *weapon_class[];

//The number of changing coefficients (the others are unchanged)
const int kMaxExpCoefficientsUsed = 15;
// unless you change this, Puff casts all your dg spells
const int kDgCasterProxy = 113;

const int kFirstRoom = 1;
const int kStrangeRoom = 3;
const int kFireMoves = 20;
const int kLookingMoves = 5;
const int kHearingMoves = 2;
const int kLookhideMoves = 3;
const int kSneakMoves = 1;
const int kCamouflageMoves = 1;
const int kPicklockMoves = 10;
const int kTrackMoves = 3;
const int kSenseMoves = 4;
const int kHidetrackMoves = 10;
const int kMobArmourMult = 5;
const int kMobAcMult = 5;
const int kMobDamageMult = 3;
const int kMaxGroupedFollowers = 7;

extern const MobVnum kHorseVnum;
extern const ObjVnum kStartBread;
extern const ObjVnum kCreateLight;
extern const int kHorseCost;

extern const char *SIELENCE;
extern const char *SOUNDPROOF;

#endif // CONSTANTS_H_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
