#include "stereo.h"

#include <cmath>

namespace stereo {

void eyeViewport(int eye, int width, int height, int out[4])
{
	const int half = width / 2;
	out[0] = (eye == 1) ? half : 0;
	out[1] = 0;
	out[2] = (eye == 0) ? half : (eye == 1) ? width - half : width;
	out[3] = height;
}

void eyeShift(int eye, float rightX, float rightZ, float separation, float* dx, float* dy)
{
	*dx = *dy = 0.0f;
	const float len = std::sqrt(rightX * rightX + rightZ * rightZ);
	if ((eye != 0 && eye != 1) || len < 1e-6f)
		return;
	const float side = (eye == 0 ? -0.5f : 0.5f) * separation / len;
	// engine camera space: x is minus world x, z is world y
	*dx = -rightX * side;
	*dy = rightZ * side;
}

}
