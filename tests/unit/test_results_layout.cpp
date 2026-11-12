// The results screens' mode from their Done buttons (code/resultslayout.h)
#include "doctest.h"
#include "resultslayout.h"

TEST_CASE("results screens: 800x600 when the Done buttons fit it (stock data)")
{
	long w = 0, h = 0;
	// stock: salvage's text at 637,536 105x29, pilot review's at 652,529 104x29
	results_layout::screenSize(757, 565, 800, 600, w, h);
	CHECK(w == 800);
	CHECK(h == 600);
	results_layout::screenSize(0, 0, 1280, 1024, w, h);	// none read: as before
	CHECK(w == 800);
	CHECK(h == 600);
}

TEST_CASE("results screens: 1024x768 for Omnitech's moved Done buttons")
{
	long w = 0, h = 0;
	// Omnitech: salvage's text at 837,536 105x29, pilot review's at 752,729 104x29
	results_layout::screenSize(942, 758, 1280, 1024, w, h);
	CHECK(w == 1024);
	CHECK(h == 768);
}

TEST_CASE("results screens: the front end's size when 1024x768 is not enough")
{
	long w = 0, h = 0;
	results_layout::screenSize(1100, 700, 1280, 1024, w, h);
	CHECK(w == 1280);
	CHECK(h == 1024);
	results_layout::screenSize(900, 800, 1280, 1024, w, h);
	CHECK(w == 1280);
	CHECK(h == 1024);
}
