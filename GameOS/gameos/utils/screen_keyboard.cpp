#include "screen_keyboard.h"

namespace screen_kb {

namespace {

enum class Kind { Char, Shift, Close };

struct Key {
	const char* lower;
	const char* upper;
	int scancode;
	float width;            // in key widths
	Kind kind;
	bool letter;
};

const int kRows = 5;

// letters capitalise with Shift; the rest send their own key, unshifted
#define LETTER(l, u, s) { l, u, s, 1.0f, Kind::Char, true }
#define PLAIN(l, s) { l, l, s, 1.0f, Kind::Char, false }

const Key kKeyTable[] = {
	// row 0
	PLAIN("1", kScan1), PLAIN("2", kScan1 + 1), PLAIN("3", kScan1 + 2), PLAIN("4", kScan1 + 3),
	PLAIN("5", kScan1 + 4), PLAIN("6", kScan1 + 5), PLAIN("7", kScan1 + 6), PLAIN("8", kScan1 + 7),
	PLAIN("9", kScan1 + 8), PLAIN("0", kScan0), PLAIN("-", kScanMinus),
	{ "Back", "Back", kScanBackspace, 1.5f, Kind::Char, false },
	// row 1
	LETTER("q", "Q", kScanA + 16), LETTER("w", "W", kScanA + 22), LETTER("e", "E", kScanA + 4),
	LETTER("r", "R", kScanA + 17), LETTER("t", "T", kScanA + 19), LETTER("y", "Y", kScanA + 24),
	LETTER("u", "U", kScanA + 20), LETTER("i", "I", kScanA + 8), LETTER("o", "O", kScanA + 14),
	LETTER("p", "P", kScanA + 15),
	// row 2
	LETTER("a", "A", kScanA + 0), LETTER("s", "S", kScanA + 18), LETTER("d", "D", kScanA + 3),
	LETTER("f", "F", kScanA + 5), LETTER("g", "G", kScanA + 6), LETTER("h", "H", kScanA + 7),
	LETTER("j", "J", kScanA + 9), LETTER("k", "K", kScanA + 10), LETTER("l", "L", kScanA + 11),
	{ "Enter", "Enter", kScanReturn, 2.0f, Kind::Char, false },
	// row 3
	{ "Shift", "Shift", 0, 1.5f, Kind::Shift, false },
	LETTER("z", "Z", kScanA + 25), LETTER("x", "X", kScanA + 23), LETTER("c", "C", kScanA + 2),
	LETTER("v", "V", kScanA + 21), LETTER("b", "B", kScanA + 1), LETTER("n", "N", kScanA + 13),
	LETTER("m", "M", kScanA + 12), PLAIN(".", kScanPeriod),
	// row 4
	{ "Close", "Close", 0, 2.0f, Kind::Close, false },
	{ "Space", "Space", kScanSpace, 6.0f, Kind::Char, false },
};

#undef LETTER
#undef PLAIN

const int kRowKeys[kRows] = { 12, 10, 10, 9, 2 };

static_assert(sizeof(kKeyTable) / sizeof(kKeyTable[0]) == Keyboard::kKeys, "a key in the table for each key");
static_assert(12 + 10 + 10 + 9 + 2 == Keyboard::kKeys, "every key in a row");

// shares of the screen
const float kLeft = 0.08f;
const float kWidth = 0.84f;
const float kHeight = 0.40f;
const float kEdge = 0.02f;      // from the top or bottom of the screen
const float kPad = 0.012f;      // inside the panel, round the keys
const float kGap = 0.004f;      // between keys, each side

} // namespace

void Keyboard::layout(bool top)
{
	panel_ = Rect{kLeft, top ? kEdge : 1.0f - kEdge - kHeight, kWidth, kHeight};

	// the widest row fills the panel; the others are centred
	float widest = 0.0f;
	int first = 0;
	for(int row = 0; row < kRows; ++row) {
		float units = 0.0f;
		for(int i = first; i < first + kRowKeys[row]; ++i)
			units += kKeyTable[i].width;
		if(units > widest)
			widest = units;
		first += kRowKeys[row];
	}
	const float unit = (kWidth - 2.0f * kPad) / widest;
	const float rowHeight = (kHeight - 2.0f * kPad) / (float)kRows;

	first = 0;
	for(int row = 0; row < kRows; ++row) {
		float units = 0.0f;
		for(int i = first; i < first + kRowKeys[row]; ++i)
			units += kKeyTable[i].width;
		float x = kLeft + 0.5f * (kWidth - units * unit);
		const float y = panel_.y + kPad + (float)row * rowHeight;
		for(int i = first; i < first + kRowKeys[row]; ++i) {
			const float w = kKeyTable[i].width * unit;
			rects_[i] = Rect{x + kGap, y + kGap, w - 2.0f * kGap, rowHeight - 2.0f * kGap};
			x += w;
		}
		first += kRowKeys[row];
	}
}

const char* Keyboard::label(int key) const
{
	if(key < 0 || key >= kKeys)
		return "";
	return shift_ ? kKeyTable[key].upper : kKeyTable[key].lower;
}

bool Keyboard::lit(int key) const
{
	return key >= 0 && key < kKeys && kKeyTable[key].kind == Kind::Shift && shift_;
}

int Keyboard::hit(float x, float y) const
{
	for(int i = 0; i < kKeys; ++i)
		if(rects_[i].w > 0.0f && rects_[i].contains(x, y))
			return i;
	return -1;
}

Press Keyboard::press(int key)
{
	Press p;
	if(key < 0 || key >= kKeys)
		return p;
	const Key& k = kKeyTable[key];
	switch(k.kind) {
	case Kind::Shift:
		shift_ = !shift_;
		break;
	case Kind::Close:
		p.close = true;
		shift_ = false;
		break;
	case Kind::Char:
		p.scancode = k.scancode;
		p.shift = k.letter && shift_;
		if(k.letter)
			shift_ = false;
		if(k.scancode == kScanReturn) {
			p.close = true;
			shift_ = false;
		}
		break;
	}
	return p;
}

} // namespace screen_kb
