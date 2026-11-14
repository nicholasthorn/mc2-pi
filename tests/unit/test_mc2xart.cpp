// tests/unit/test_mc2xart.cpp
//
// wolfman-MC2X renames the interface layouts the game opens by name
// (mcl_dialog.fit is mc2x_dialog.fit, buttonlayout1024.fit is
// mc2x_buttonlayout1024.fit...). mc2xArtName gives MC2X's name for a stock
// one; the game tries it only when the stock file is not there.

#include "doctest.h"

#include "mc2xart.h"
#include "hud_layout.h"

#include <cstring>
#include <string>

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

}
