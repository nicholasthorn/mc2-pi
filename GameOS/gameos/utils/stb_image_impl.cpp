// The one translation unit holding the stb_image implementation, JPEG only.
// thirdparty/stb/stb_image.h is vendored unmodified (v2.30, nothings/stb
// commit 003501f6d7a8668ce8b6d875e34dd97ad0d5d5b5); it is third-party code,
// so this file keeps the legacy warning flags rather than the strict ones.
// Used for MechCommander Omnitech's .burnin.jpg terrain colour maps.

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_MAX_DIMENSIONS 16384
#include "stb/stb_image.h"
