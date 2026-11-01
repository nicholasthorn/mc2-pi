#include "stereo.h"

namespace stereo {

void eyeViewport(int eye, int width, int height, int out[4])
{
	const int half = width / 2;
	out[0] = (eye == 1) ? half : 0;
	out[1] = 0;
	out[2] = (eye == 0) ? half : (eye == 1) ? width - half : width;
	out[3] = height;
}

float eyeOffset(int eye, float separation)
{
	if (eye == 0)
		return -0.5f * separation;
	if (eye == 1)
		return 0.5f * separation;
	return 0.0f;
}

}
