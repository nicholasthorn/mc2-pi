// aseconv: an ASE vertex colour component (0..1) to a channel value, as
// mclib/tgl.cpp packs them into TG_TypeVertex::aRGBLight.
//
// 222 stock models hold vertex colours far outside 0..1 (bridge5dam.ase has
// -22.9 and 45.6). Converting such a float straight to an unsigned is
// undefined: an x86 build wraps it through a 64-bit signed conversion, an
// arm64 build clamps it, so .tgl data depended on the machine that built it.
// This reproduces x86, which built every .tgl shipped so far, with defined
// behaviour. GL-free so it can be unit tested.
#ifndef ASECOLOUR_H
#define ASECOLOUR_H

#include <cstdint>

inline uint32_t aseColourToChannel(float component)
{
	const float scaled = component * 255.0f;
	// x86's conversion of NaN or a value outside int64 gives 0x8000000000000000,
	// whose low 32 bits are 0
	if (!(scaled > -9.2e18f && scaled < 9.2e18f))
		return 0;
	return static_cast<uint32_t>(static_cast<int64_t>(scaled));
}

#endif
