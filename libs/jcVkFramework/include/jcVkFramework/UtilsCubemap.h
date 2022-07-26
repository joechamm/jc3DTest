#pragma once

//#include "Bitmap.h"
#include <jcVkFramework/Bitmap.h>

Bitmap convertEquirectangularMapToVerticalCross ( const Bitmap& b );
Bitmap convertVerticalCrossToCubeMapFaces ( const Bitmap& b );