// Compile-time pins of the sizes of structs read from and written to disk.
//
// Nothing here runs: the file only has to compile. A failure means a struct's
// layout changed, and what breaks depends on the group below.
//
// - Save games write these structs whole, so changing one breaks existing saves
//   on that platform. If the change is intended, update the number and accept
//   that older saves stop loading. `long` is 8 bytes on LP64 (Linux, arm64) and
//   4 on LLP64 (Windows), so each has two numbers, and saves do not move between
//   the two.
// - Game data files are built once and copied to every platform, so those
//   structs must have one size everywhere.
//
// The compiler error names the actual and the expected size, as the arguments
// of SizeOf_<struct><actual, expected>.
//
// These pins catch a change of size, not of layout: reordering a struct's fields,
// or swapping two types of the same size, still passes and still breaks files.

#include <cstddef>

#include <gameobj.h>
#include <mover.h>
#include <terrobj.h>
#include <bldng.h>
#include <turret.h>
#include <gate.h>
#include <artlry.h>
#include <carnage.h>
#include <mech.h>
#include <gvehicl.h>
#include <weaponbolt.h>
#include <objmgr.h>
#include <team.h>
#include <comndr.h>
#include <warrior.h>
#include <tgl.h>
#include <tgainfo.h>
#include <move.h>
#include <vertex.h>
#include <ffile.h>
#include <ablexec.h>

namespace {

// One template per struct: MSVC reports a static_assert once per source location,
// and this way every failure names its struct and sizes.
#define MC2_PIN_SIZE(Type, Expected) \
	template <std::size_t Actual, std::size_t Want> struct SizeOf_##Type \
	{ \
		static_assert(Actual == Want, #Type " changed size: see code/ondisksizes.cpp"); \
	}; \
	static_assert(sizeof(SizeOf_##Type<sizeof(Type), (Expected)>) != 0, #Type)

#ifdef PLATFORM_WINDOWS
#define MC2_PIN_SAVE_SIZE(Type, Lp64, Llp64) MC2_PIN_SIZE(Type, Llp64)
#else
#define MC2_PIN_SAVE_SIZE(Type, Lp64, Llp64) MC2_PIN_SIZE(Type, Lp64)
#endif

// Save games: objects (GameObjectManager::load in objmgr.cpp). GameObjectData and
// MoverData are never saved on their own: load refuses a packet of their size.
MC2_PIN_SAVE_SIZE(GameObjectData, 372, 372);
MC2_PIN_SAVE_SIZE(MoverData, 3128, 3000);
MC2_PIN_SAVE_SIZE(TerrainObjectData, 720, 720);
MC2_PIN_SAVE_SIZE(BuildingData, 756, 756);
MC2_PIN_SAVE_SIZE(TurretData, 820, 820);
MC2_PIN_SAVE_SIZE(GateData, 768, 760);
MC2_PIN_SAVE_SIZE(ArtilleryData, 432, 432);
MC2_PIN_SAVE_SIZE(CarnageData, 396, 396);
MC2_PIN_SAVE_SIZE(MechData, 3928, 3704);
MC2_PIN_SAVE_SIZE(GroundVehicleData, 3264, 3136);
MC2_PIN_SAVE_SIZE(WeaponBoltData, 600, 580);

// Save games: everything else written raw
MC2_PIN_SAVE_SIZE(ObjectManagerData, 60, 60);
MC2_PIN_SAVE_SIZE(TeamData, 496, 488);
MC2_PIN_SAVE_SIZE(TeamStaticData, 88, 80);
MC2_PIN_SAVE_SIZE(CommanderData, 1424, 1288);
MC2_PIN_SAVE_SIZE(StaticCommanderData, 16, 8);
MC2_PIN_SAVE_SIZE(MechWarriorData, 20936, 19864);
MC2_PIN_SAVE_SIZE(StaticMechWarriorData, 40, 40);
MC2_PIN_SAVE_SIZE(StackItem, 8, 8);	// ABL static data (mclib/ablenv.cpp)

// Game data files: one size on every platform. (GlobalMapArea, DoorLink and the
// other pathing records are pinned in mclib/move.cpp.)
MC2_PIN_SIZE(TG_TypeVertex, 28);
MC2_PIN_SIZE(TG_TypeTriangle, 56);
MC2_PIN_SIZE(TG_Texture, 268);
MC2_PIN_SIZE(TG_TinyTexture, 12);
MC2_PIN_SIZE(TGAFileHeader, 18);
MC2_PIN_SIZE(MapCell, 4);
MC2_PIN_SIZE(PostcompVertex, 32);	// terrain (MapData::newInit, MapData::save)
MC2_PIN_SIZE(FILEENTRY, 266);		// FST archive table (mclib/ffile.cpp)
// animation frames (mclib/msl.cpp); aliased because the macro pastes the name
typedef Stuff::Point3D AnimPosition;
typedef Stuff::UnitQuaternion AnimRotation;
MC2_PIN_SIZE(AnimPosition, 12);
MC2_PIN_SIZE(AnimRotation, 16);

// ObjectManager::load tells an object's type by its packet size alone, so two
// of these with one size would load as the wrong type.
constexpr std::size_t objectPacketSizes[] = {
	sizeof(GameObjectData), sizeof(MoverData), sizeof(TerrainObjectData),
	sizeof(BuildingData), sizeof(TurretData), sizeof(GateData),
	sizeof(ArtilleryData), sizeof(CarnageData), sizeof(MechData),
	sizeof(GroundVehicleData), sizeof(WeaponBoltData),
};

// (C++11 constexpr: one return statement each)
constexpr bool noneEqual(std::size_t value, const std::size_t* sizes, std::size_t count)
{
	return count == 0 || (value != sizes[0] && noneEqual(value, sizes + 1, count - 1));
}

constexpr bool allDistinct(const std::size_t* sizes, std::size_t count)
{
	return count == 0 || (noneEqual(sizes[0], sizes + 1, count - 1) && allDistinct(sizes + 1, count - 1));
}

static_assert(allDistinct(objectPacketSizes, sizeof(objectPacketSizes) / sizeof(objectPacketSizes[0])),
		"two object save structs have one size: ObjectManager::load would mix them up");

} // namespace
