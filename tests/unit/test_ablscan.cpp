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
#include "fuzz/fuzz_support.h"

#include <cstdint>
#include <cstring>
#include <string>

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

// Compiles source and tears ABL down when it goes out of scope, so a failed
// REQUIRE does not leave the module allocated.
struct AblCompiled
{
	long handle;
	explicit AblCompiled(const char* source) : handle(ablTestCompile(source)) {}
	~AblCompiled()
	{
		if (handle >= 0)
			ablTestClose();
	}
};

} // namespace

TEST_SUITE("AblScan")
{

// A char array assignment copied strlen(source) + 1 bytes (bounded by the
// target), so a source filled to its last byte, with no terminator, was read
// past its end: ASan, a char[12] in a wolfman-MC2X brain (redrockoutpost).
// Each static array is its own heap block, so this one is too. The length
// is now bounded by the source array's size as well.
TEST_CASE("ABL char array assignment: a source with no terminator is read to its size only")
{
	AblCompiled c(
		"module M : integer;\n"
		"\tvar\n"
		"\t\tstatic char[4] full;\n"
		"\t\tstatic char[8] copy;\n"
		"\t\tstatic char[8] shortcopy;\n"
		"\tcode\n"
		"\t\tfull[0] = \"a\";\n"
		"\t\tfull[1] = \"b\";\n"
		"\t\tfull[2] = \"c\";\n"
		"\t\tfull[3] = \"d\";\n"
		"\t\tcopy = full;\n"
		"\t\tshortcopy = \"xy\";\n"
		"\t\treturn(0);\n"
		"endmodule.\n");
	REQUIRE(c.handle >= 0);

	ABLModule m;
	REQUIRE(m.init(c.handle) == 0);
	m.execute();

	SymTableNodePtr copy = m.findSymbol("copy");
	SymTableNodePtr shortCopy = m.findSymbol("shortcopy");
	REQUIRE(copy != nullptr);
	REQUIRE(shortCopy != nullptr);
	const char* copyText = (const char*)(m.getStaticData() + copy->defn.info.data.offset)->address;
	const char* shortText = (const char*)(m.getStaticData() + shortCopy->defn.info.data.offset)->address;
	CHECK(strcmp(copyText, "abcd") == 0);
	CHECK(strcmp(shortText, "xy") == 0);
	m.destroy();
}

// Optional trailing parameters ("|" in the registered list): wolfman-MC2X
// calls newpower(partId, false) where stock calls newpower(false), so one
// command must compile both. The callback asks ABLi_moreParams().
TEST_CASE("ABL optional parameters: both forms compile and run")
{
	AblCompiled c(
		"module M : integer;\n"
		"\tvar\n"
		"\t\tstatic integer one;\n"
		"\t\tstatic integer two;\n"
		"\tcode\n"
		"\t\tone = fuzzoptional(true);\n"
		"\t\ttwo = fuzzoptional(7, false);\n"
		"\t\treturn(0);\n"
		"endmodule.\n");
	REQUIRE(c.handle >= 0);

	ABLModule m;
	REQUIRE(m.init(c.handle) == 0);
	m.execute();
	CHECK(m.getStaticInteger((char*)"one") == 1001);
	CHECK(m.getStaticInteger((char*)"two") == 2070);
	m.destroy();
}

TEST_CASE("ABL optional parameters: too few, too many or the wrong type is an error")
{
	const char* bad[] = {
		"fuzzoptional()",
		"fuzzoptional(1, true, 2)",
		"fuzzoptional(1, 2)",
	};
	for (const char* call : bad)
	{
		CAPTURE(call);
		std::string source = std::string(
			"module M : integer;\n"
			"\tvar\n"
			"\t\tinteger x;\n"
			"\tcode\n"
			"\t\tx = ") + call + ";\n"
			"\t\treturn(x);\n"
			"endmodule.\n";
		AblCompiled c(source.c_str());
		CHECK(c.handle < 0);
	}
}

// A command registered with a result must leave exactly one value on the ABL
// stack, and one without none. setobjectdamage pushed nothing: each call as a
// statement took one slot too many off the stack, and a later push landed
// in the running frame (wolfman-MC2X's reap3m02 lost its local array's
// address). The executor now evens the stack after every callback.
TEST_CASE("ABL commands that break the stack contract cannot corrupt the frame")
{
	AblCompiled c(
		"module M : integer;\n"
		"\tvar\n"
		"\t\tstatic integer before;\n"
		"\t\tstatic integer after;\n"
		"\t\tstatic integer result;\n"
		"\t\tinteger[3] local;\n"
		"\t\tinteger x;\n"
		"\tcode\n"
		"\t\tlocal[0] = 11;\n"
		"\t\tlocal[1] = 22;\n"
		"\t\tlocal[2] = 33;\n"
		"\t\tx = 44;\n"
		"\t\tbefore = local[0] + local[1] + local[2] + x;\n"
		"\t\tfuzznopush(1);\n"
		"\t\tfuzznopush(2);\n"
		"\t\tfuzznopush(3);\n"
		"\t\tfuzzextrapush(4);\n"
		"\t\tresult = fuzznopush(5);\n"
		"\t\tx = x + 0;\n"
		"\t\tafter = local[0] + local[1] + local[2] + x;\n"
		"\t\treturn(0);\n"
		"endmodule.\n");
	REQUIRE(c.handle >= 0);

	ABLModule m;
	REQUIRE(m.init(c.handle) == 0);
	m.execute();
	CHECK(m.getStaticInteger((char*)"before") == 110);
	CHECK(m.getStaticInteger((char*)"after") == 110);
	CHECK(m.getStaticInteger((char*)"result") == 0);
	m.destroy();
}

// The runtime stack was allocated as ABL_RUNTIME_STACK_SIZE bytes, but every
// push was checked against a fixed 10240 slots: right for the 2001 build's
// 4-byte StackItem, twice what a 64-bit one has room for, so deep recursion
// wrote past the block. It now has the 2001 build's slot count, checked
// against what was allocated: this recursion goes about 2040 calls deep, as
// in 2001 (a 64-bit build ran off the end after about 1020), then stops with
// a stack overflow error.
TEST_CASE("ABL recursion stops with a stack overflow, inside the stack")
{
	AblCompiled c(
		"module M : integer;\n"
		"\tvar\n"
		"\t\tstatic integer depth;\n"
		"function dive (integer n) : integer;\n"
		"\tcode\n"
		"\t\tdepth = n;\n"
		"\t\treturn(dive(n + 1));\n"
		"endfunction;\n"
		"\tcode\n"
		"\t\treturn(dive(1));\n"
		"endmodule.\n");
	REQUIRE(c.handle >= 0);

	ABLModule m;
	REQUIRE(m.init(c.handle) == 0);
	CHECK_FALSE(fuzzGuarded([&m] { m.execute(); }));
	CHECK(m.getStaticInteger((char*)"depth") > 2000);
	m.destroy();
}

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
	AblCompiled compiled(source);
	const long handle = compiled.handle;
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
	CHECK(AblCompiled(assign).handle == -1);

	const char* valParam =
		"module M : integer;\n"
		"function takes (char[8] s) : integer;\n"
		"\tcode\n"
		"\t\treturn(1);\n"
		"endfunction;\n"
		"\tcode\n"
		"\t\treturn(takes(fuzznone));\n"
		"endmodule.\n";
	CHECK(AblCompiled(valParam).handle == -1);
}

} // TEST_SUITE("AblScan")
