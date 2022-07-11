#pragma once

#include <jc3DTestSharedLibs/Bitmap.h>

Bitmap convertEquirectangularMapToVerticalCross(const Bitmap& b);
Bitmap convertVerticalCrossToCubeMapFaces(const Bitmap& b);
