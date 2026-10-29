#include "audio_pan.h"

#include <cmath>

static uint8_t toLevel(double gain)
{
	if (gain >= 1.0)
		return 255;
	if (gain <= 0.0)
		return 0;
	return static_cast<uint8_t>(std::lround(gain * 255.0));
}

StereoLevels panToStereoLevels(float pan)
{
	if (std::isnan(pan))
		pan = 0.0f;
	if (pan < -1.0f)
		pan = -1.0f;
	else if (pan > 1.0f)
		pan = 1.0f;

	// t: 0 = hard left, 0.5 = centre, 1 = hard right. cos/sin give a
	// constant-power curve; sqrt(2) lifts the centre (cos(pi/4)) to 1.0.
	const double t = (static_cast<double>(pan) + 1.0) * 0.5;
	const double halfPi = 1.57079632679489661923;
	const double angle = t * halfPi;
	const double scale = std::sqrt(2.0);

	StereoLevels s;
	s.left = toLevel(scale * std::cos(angle));
	s.right = toLevel(scale * std::sin(angle));
	return s;
}
