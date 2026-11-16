// tests/unit/test_mc2xart.cpp
//
// wolfman-MC2X renames the interface layouts the game opens by name
// (mcl_dialog.fit is mc2x_dialog.fit, buttonlayout1024.fit is
// mc2x_buttonlayout1024.fit...). mc2xArtName gives MC2X's name for a stock
// one; the game tries it only when the stock file is not there.

#include "doctest.h"

#include "mc2xart.h"
#include "hud_layout.h"
#include "inifile.h"
#include "paths.h"

#include <cstdio>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <direct.h>
#define mc2x_mkdir(p) _mkdir(p)
#define mc2x_rmdir(p) _rmdir(p)
#else
#include <sys/stat.h>
#include <unistd.h>
#define mc2x_mkdir(p) mkdir(p, 0755)
#define mc2x_rmdir(p) rmdir(p)
#endif

namespace
{

std::string mapped(const char* stock)
{
	char out[64];
	if (!mc2xArtName(stock, out, sizeof(out)))
		return "-";
	return out;
}

} // namespace

TEST_SUITE("Mc2xArt")
{

TEST_CASE("mc2xArtName: the renames MC2X's art uses")
{
	CHECK(mapped("mcl_dialog") == "mc2x_dialog");
	CHECK(mapped("mcl_dialog_onebutton") == "mc2x_dialog_onebutton");
	CHECK(mapped("mcl_sm") == "mc2x_sm");
	CHECK(mapped("mcl_loadingscreen") == "mc2x_loadingscreen");
	CHECK(mapped("buttonlayout1024") == "mc2x_buttonlayout1024");
	CHECK(mapped("mcui_keyref_1280") == "mc2x_keyref_1280");
	CHECK(mapped("mcui_keyref_entry") == "mc2x_keyref_entry");
}

TEST_CASE("mc2xArtName: names MC2X shortened as well")
{
	CHECK(mapped("mcl_gn_deploymentteams") == "mc2x_deploymentteams");
	CHECK(mapped("mcl_gn_availablemechentry") == "mc2x_availablemechentry");
	CHECK(mapped("mcl_pr_availablepilotentry") == "mc2x_availablepilotentry");
	CHECK(mapped("mcl_cm_opplayer") == "mc2x_opplayer");
	CHECK(mapped("mcl_mp_loadmap") == "mc2x_loadmap");
	CHECK(mapped("mcl_mp_loadmap_list0") == "mc2x_loadmap_list0");
	CHECK(mapped("mcl_mp_mapinfo") == "mc2x_mapinfo");
	CHECK(mapped("cameras") == "mc2xcams");
	CHECK(mapped("colors") == "mc2xcols");
	CHECK(mapped("gamesys") == "mc2xgsys");
	// the rest of mcl_gn_ and mcl_mp_ only change the prefix
	CHECK(mapped("mcl_mp_param") == "mc2x_mp_param");
}

// FitIniFile::open tries MC2X's name for a missing layout in the art
// folder, and only there, and says when it did.
TEST_CASE("FitIniFile::open falls back to MC2X's name in the art folder")
{
	const std::string savedArtPath = artPath;	// artPath is char[80] (paths.cpp)
	strcpy(artPath, "mc2xart_scratch/");
	mc2x_mkdir("mc2xart_scratch");
	auto write = [](const char* path) {
		FILE* fp = fopen(path, "wb");
		REQUIRE(fp != nullptr);
		fputs("FITini\n[Probe]\nl Value = 7\nFITend\n", fp);
		fclose(fp);
	};
	write("mc2xart_scratch/mc2x_probe.fit");
	write("mc2xart_scratch/mcl_both.fit");
	write("mc2xart_scratch/mc2x_both.fit");
	write("mc2x_elsewhere.fit");	// outside the art folder

	FitIniFile file;
	REQUIRE(file.open("mc2xart_scratch/mcl_probe.fit") == NO_ERR);
	CHECK(file.openedMC2XName());
	long value = 0;
	REQUIRE(file.seekBlock("Probe") == NO_ERR);
	REQUIRE(file.readIdLong("Value", value) == NO_ERR);
	CHECK(value == 7);
	file.close();

	// the stock file wins when it is there
	REQUIRE(file.open("mc2xart_scratch/mcl_both.fit") == NO_ERR);
	CHECK_FALSE(file.openedMC2XName());
	file.close();

	// a missing name with no MC2X form, or outside the art folder, stays missing
	CHECK(file.open("mc2xart_scratch/cursors.fit") != NO_ERR);
	CHECK(file.open("mcl_elsewhere.fit") != NO_ERR);

	remove("mc2xart_scratch/mc2x_probe.fit");
	remove("mc2xart_scratch/mcl_both.fit");
	remove("mc2xart_scratch/mc2x_both.fit");
	remove("mc2x_elsewhere.fit");
	mc2x_rmdir("mc2xart_scratch");
	strcpy(artPath, savedArtPath.c_str());
}

TEST_CASE("FitIniFile: MC2X's mc2xgsys.fit answers for gamesys.fit's [General]")
{
	const std::string savedMissionPath = missionPath;
	strcpy(missionPath, "mc2xgsys_scratch/");
	mc2x_mkdir("mc2xgsys_scratch");
	FILE* fp = fopen("mc2xgsys_scratch/mc2xgsys.fit", "wb");
	REQUIRE(fp != nullptr);
	fputs("FITini\n[mc2xGeneral]\nf MaxVisualRange = 900.0\nFITend\n", fp);
	fclose(fp);
	fp = fopen("mc2xgsys_scratch/stock.fit", "wb");
	REQUIRE(fp != nullptr);
	fputs("FITini\n[mc2xGeneral]\nf MaxVisualRange = 900.0\nFITend\n", fp);
	fclose(fp);

	FitIniFile file;
	REQUIRE(file.open("mc2xgsys_scratch/gamesys.fit") == NO_ERR);
	CHECK(file.openedMC2XName());
	REQUIRE(file.seekBlock("General") == NO_ERR);
	float range = 0.f;
	REQUIRE(file.readIdFloat("MaxVisualRange", range) == NO_ERR);
	CHECK(range == 900.f);
	CHECK(file.seekBlock("Missing") != NO_ERR);
	file.close();

	// a file opened under its own name keeps the block names it has
	REQUIRE(file.open("mc2xgsys_scratch/stock.fit") == NO_ERR);
	CHECK(file.seekBlock("General") != NO_ERR);
	file.close();

	remove("mc2xgsys_scratch/mc2xgsys.fit");
	remove("mc2xgsys_scratch/stock.fit");
	mc2x_rmdir("mc2xgsys_scratch");
	strcpy(missionPath, savedMissionPath.c_str());
}

TEST_CASE("mc2xDataFileName: the data files MC2X renamed")
{
	char out[64];
	auto data = [&](const char* stock) -> std::string {
		return mc2xDataFileName(stock, out, sizeof(out)) ? std::string(out) : std::string("-");
	};
	CHECK(data("compbas.csv") == "mc2xcomp.csv");
	CHECK(data("buildings.csv") == "mc2x_inv.csv");
	CHECK(data("pilots.csv") == "mc2xpilots.csv");
	CHECK(data("gamesys.fit") == "mc2xgsys.fit");
	CHECK(data("mc2.fx") == "mc2x.fx");
	CHECK(data("corebrain.abx") == "mc2xcore.abx");
	CHECK(data("orders.abx") == "-");
	CHECK(data("") == "-");
	CHECK_FALSE(mc2xDataFileName("compbas.csv", out, 4));
}

// stockOrMC2XFileName: stock's file when it is there, MC2X's only when it
// alone is, and the stock name when neither is (the caller reports it).
TEST_CASE("stockOrMC2XFileName prefers the stock file")
{
	mc2x_mkdir("mc2xdata_scratch");
	auto write = [](const char* path) {
		FILE* fp = fopen(path, "wb");
		REQUIRE(fp != nullptr);
		fputs("x", fp);
		fclose(fp);
	};
	char name[64];

	write("mc2xdata_scratch/mc2xpilots.csv");
	stockOrMC2XFileName("mc2xdata_scratch/", "pilots.csv", name, sizeof(name));
	CHECK(std::string(name) == "mc2xpilots.csv");

	write("mc2xdata_scratch/pilots.csv");
	stockOrMC2XFileName("mc2xdata_scratch/", "pilots.csv", name, sizeof(name));
	CHECK(std::string(name) == "pilots.csv");

	stockOrMC2XFileName("mc2xdata_scratch/", "compbas.csv", name, sizeof(name));
	CHECK(std::string(name) == "compbas.csv");

	remove("mc2xdata_scratch/mc2xpilots.csv");
	remove("mc2xdata_scratch/pilots.csv");
	mc2x_rmdir("mc2xdata_scratch");
}

TEST_CASE("mc2xArtName: other names have no MC2X form")
{
	CHECK(mapped("mcui_mr_layout") == "-");
	CHECK(mapped("cursors") == "-");
	CHECK(mapped("") == "-");
	CHECK(mapped("mcl_") == "-");
}

TEST_CASE("mc2xArtName: a name too long for the buffer is refused")
{
	char out[8];
	CHECK_FALSE(mc2xArtName("mcl_dialog", out, sizeof(out)));
	CHECK_FALSE(mc2xArtName("mcl_dialog", nullptr, 64));
}

TEST_CASE("mc2xVehicleButton: stock's support units in MC2X's 22-button layout")
{
	// stock: airstrike, guard tower (artillery), sensor probe, repair
	// vehicle, scout, minelayer, recovery, cancel
	const int expected[8] = { 0, 6, 1, 2, 3, 4, 5, 21 };
	for (int i = 0; i < 8; i++)
		CHECK(mc2xVehicleButton(i) == expected[i]);
	CHECK(mc2xVehicleButton(-1) == -1);
	CHECK(mc2xVehicleButton(8) == -1);
}

TEST_CASE("mc2xBlockName: mc2xgsys.fit's General is mc2xGeneral, nothing else is renamed")
{
	CHECK(strcmp(mc2xBlockName("data/missions/mc2xgsys.fit", "General"), "mc2xGeneral") == 0);
	CHECK(strcmp(mc2xBlockName("mc2xgsys.fit", "General"), "mc2xGeneral") == 0);
	CHECK(mc2xBlockName("data/art/mc2x_buttonlayout1024.fit", "General") == nullptr);
	CHECK(mc2xBlockName("data/missions/mc2xgsys.fit", "general") == nullptr);
	CHECK(mc2xBlockName("data/missions/mc2xgsys.fit", "mc2xGeneral") == nullptr);
	CHECK(mc2xBlockName(nullptr, "General") == nullptr);
	CHECK(mc2xBlockName("mc2xgsys.fit", nullptr) == nullptr);
}

}
