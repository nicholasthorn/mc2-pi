// tests/unit/fuzz/fuzz_csv.cpp: CSVFile over any bytes.
//
// Opens the input as a CSV file in memory (File's buffer open, then the CSV
// parse CSVFile::open runs) and reads every cell (up to a cap)
// with each typed reader, into string buffers both too small and roomy, as
// the component, building and pilot tables are read.

#include "fuzz_targets.h"
#include "fuzz_support.h"

#include "csvfile.h"

#include <vector>

namespace
{

class FuzzCsv : public CSVFile
{
public:
	DWORD rows() const { return totalRows; }
	DWORD cols() const { return totalCols; }

	// CSVFile::open by name, from a buffer: a memory parent goes through
	// File::open(parent), which a memory File does not serve for a CSV
	long openImage(const char* image, int size)
	{
		long result = File::open(image, size);
		if (result != NO_ERR)
			return result;
		return afterOpen();
	}
};

} // namespace

int fuzzCsvFile(const uint8_t* data, size_t size)
{
	if (size == 0 || size > (1 << 20))
		return 0;
	std::vector<char> image(data, data + size);

	fuzzGuarded([&] {
		FuzzCsv csv;
		if (csv.openImage(image.data(), (int)image.size()) == NO_ERR)
		{
			// Rows and columns are 1-based; one past the end is read too, as
			// logisticsdata.cpp scans until a read fails.
			const DWORD rows = csv.rows() < 64 ? csv.rows() : 64;
			const DWORD cols = csv.cols() < 32 ? csv.cols() : 32;
			for (DWORD r = 1; r <= rows + 1; r++)
			{
				for (DWORD c = 1; c <= cols + 1; c++)
				{
					char small[4];
					char roomy[256];
					csv.readString(r, c, small, sizeof(small));
					csv.readString(r, c, roomy, sizeof(roomy));
					long l = 0;
					int i = 0;
					unsigned long u = 0;
					short s = 0;
					unsigned short us = 0;
					char ch = 0;
					unsigned char uc = 0;
					float f = 0.0f;
					bool b = false;
					csv.readLong(r, c, l);
					csv.readInt(r, c, i);
					csv.readULong(r, c, u);
					csv.readShort(r, c, s);
					csv.readUShort(r, c, us);
					csv.readChar(r, c, ch);
					csv.readUChar(r, c, uc);
					csv.readFloat(r, c, f);
					csv.readBoolean(r, c, b);
				}
			}
			csv.close();
		}
	});
	return 0;
}
