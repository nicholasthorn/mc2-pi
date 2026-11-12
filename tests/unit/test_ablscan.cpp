// tests/unit/test_ablscan.cpp
//
// ABL integer literals: their digits' exact value, as its low 32 bits, the
// rule the FIT and CSV readers follow too (wholeNumberLow32). The stock
// mission scripts (mc2_01, mc2_08, mc2_17, mc2_18, multiplayer) call
// fadeToColor(4278190080, ...), past the int range; the 2001 build gave
// 0xFF000000 (opaque black) for it, and so does this. For literals a float
// cannot hold (past 2^24, none in any script here) the result is exact now,
// where the 2001 build rounded through the float: a deliberate change.

#include "doctest.h"

#include "ablscan.h"
#include "ablsymt.h"
#include "ablenv.h"
#include "fuzz/fuzz_targets.h"

#include <cstdint>
#include <cstring>

extern ModuleEntryPtr ModuleRegistry;

namespace
{

// A module-level constant of the module compiled as handle, by its
// (lower-case) name; NULL when there is none.
SymTableNodePtr findConstant(long handle, const char* name)
{
	SymTableNodePtr table = ModuleRegistry[handle].moduleIdPtr->defn.info.routine.localSymTable;
	return searchSymTable(name, table);
}

} // namespace

TEST_SUITE("AblScan")
{

TEST_CASE("ablIntegerLiteral: past the int range, the value's low 32 bits")
{
	CHECK((uint32_t)ablIntegerLiteral("4278190080") == 0xFF000000u);
	CHECK((uint32_t)ablIntegerLiteral("4294967296") == 0u);
	CHECK(ablIntegerLiteral("2147483648") == (int)0x80000000u);
}

TEST_CASE("ablIntegerLiteral: exact where the old float route rounded (a deliberate change)")
{
	// 0xFF123456: the float route gave 0xFF123400
	CHECK((uint32_t)ablIntegerLiteral("4279383126") == 0xFF123456u);
	// the float route gave 2147483648 (INT_MIN) and 16777216
	CHECK(ablIntegerLiteral("2147483647") == 2147483647);
	CHECK(ablIntegerLiteral("16777217") == 16777217);
}

TEST_CASE("ablIntegerLiteral: ordinary literals are unchanged")
{
	CHECK(ablIntegerLiteral("0") == 0);
	CHECK(ablIntegerLiteral("12") == 12);
	CHECK(ablIntegerLiteral("350") == 350);
	CHECK(ablIntegerLiteral("16777216") == 16777216);
}

TEST_CASE("literals compiled from source text")
{
	const char* source =
		"module M : integer;\n"
		"\tconst\n"
		"\t\tBIG = 4278190080;\n"
		"\t\tEXACT = 4279383126;\n"
		"\t\tLOWEST = -2147483648;\n"
		"\t\tHALF = 1.5;\n"
		"\tvar\n"
		"\t\tinteger x;\n"
		"\tcode\n"
		"\t\tx = 4278190080;\n"
		"\t\treturn(x);\n"
		"endmodule.\n";
	const long handle = ablTestCompile(source);
	REQUIRE(handle >= 0);
	SymTableNodePtr big = findConstant(handle, "big");
	SymTableNodePtr exact = findConstant(handle, "exact");
	SymTableNodePtr lowest = findConstant(handle, "lowest");
	SymTableNodePtr half = findConstant(handle, "half");
	REQUIRE(big != nullptr);
	REQUIRE(exact != nullptr);
	REQUIRE(lowest != nullptr);
	REQUIRE(half != nullptr);
	CHECK((uint32_t)big->defn.info.constant.value.integer == 0xFF000000u);
	CHECK((uint32_t)exact->defn.info.constant.value.integer == 0xFF123456u);
	// -2147483648 scans as INT_MIN and is negated: through unsigned, no overflow
	CHECK((uint32_t)lowest->defn.info.constant.value.integer == 0x80000000u);
	CHECK(half->defn.info.constant.value.real == doctest::Approx(1.5f));
	ablTestClose();
}

TEST_CASE("a call with no return value is the wrong type in an assignment or value parameter")
{
	// both read the missing type through NULL at compile time before
	const char* assign =
		"module M : integer;\n"
		"\tvar\n"
		"\t\tchar[8] s;\n"
		"\tcode\n"
		"\t\ts = fuzznone;\n"
		"\t\treturn(0);\n"
		"endmodule.\n";
	CHECK(ablTestCompile(assign) == -1);

	const char* valParam =
		"module M : integer;\n"
		"function takes (char[8] s) : integer;\n"
		"\tcode\n"
		"\t\treturn(1);\n"
		"endfunction;\n"
		"\tcode\n"
		"\t\treturn(takes(fuzznone));\n"
		"endmodule.\n";
	CHECK(ablTestCompile(valParam) == -1);
}

} // TEST_SUITE("AblScan")
