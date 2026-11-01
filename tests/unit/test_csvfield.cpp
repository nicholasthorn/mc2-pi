// csvNextField (code/csvfield.h): the field reader behind
// LogisticsComponent::extractString, which parses compbas.csv rows.
#include "doctest.h"

#include "csvfield.h"

#include <cstring>

TEST_SUITE("CsvField") {

TEST_CASE("fields split on commas and the cursor moves past each one") {
	char line[] = "PPC,10,2.5";
	char* p = line;
	char buf[16];
	CHECK(csvNextField(p, buf, sizeof(buf)) == 3);
	CHECK(std::strcmp(buf, "PPC") == 0);
	CHECK(csvNextField(p, buf, sizeof(buf)) == 2);
	CHECK(std::strcmp(buf, "10") == 0);
	CHECK(csvNextField(p, buf, sizeof(buf)) == 3);
	CHECK(std::strcmp(buf, "2.5") == 0);
}

TEST_CASE("a field that ends the line leaves the cursor on the NUL") {
	// the original stepped past the NUL, so a short row read beyond the line
	char storage[] = "a,b\0XYZ";
	char* p = storage;
	char buf[16];
	CHECK(csvNextField(p, buf, sizeof(buf)) == 1);
	CHECK(csvNextField(p, buf, sizeof(buf)) == 1);
	CHECK(std::strcmp(buf, "b") == 0);
	CHECK(p == storage + 3);
	// every later read of the short row gives empty fields, not "XYZ"
	CHECK(csvNextField(p, buf, sizeof(buf)) == 0);
	CHECK(buf[0] == '\0');
	CHECK(p == storage + 3);
}

TEST_CASE("a newline ends the field and is skipped") {
	char line[] = "x\nnext";
	char* p = line;
	char buf[16];
	CHECK(csvNextField(p, buf, sizeof(buf)) == 1);
	CHECK(std::strcmp(buf, "x") == 0);
	CHECK(std::strcmp(p, "next") == 0);
}

TEST_CASE("a field longer than the buffer is cut to fit") {
	char line[] = "abcdefgh,z";
	char* p = line;
	char buf[4];
	CHECK(csvNextField(p, buf, sizeof(buf)) == 8);
	CHECK(std::strcmp(buf, "abc") == 0);
	CHECK(csvNextField(p, buf, sizeof(buf)) == 1);
	CHECK(std::strcmp(buf, "z") == 0);
}

TEST_CASE("no separator in the first 512 bytes is an error and does not move") {
	static char line[600];
	std::memset(line, 'q', sizeof(line) - 1);
	line[sizeof(line) - 1] = '\0';
	char* p = line;
	char buf[16];
	CHECK(csvNextField(p, buf, sizeof(buf)) == 0);
	CHECK(buf[0] == '\0');
	CHECK(p == line);
}

}
