// macOS has no <malloc.h>. Legacy files include it for alloca(), which macOS
// declares in <alloca.h>; the include directory holding this file is added
// for APPLE builds only (top-level CMakeLists.txt).
#pragma once
#include <alloca.h>
#include <stdlib.h>
