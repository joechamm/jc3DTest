#ifndef __UTILS_CUBEMAP_H__
#define __UTILS_CUBEMAP_H__

#include <jcCommonFramework/Bitmap.h>

Bitmap convertEquirectangularMapToVerticalCross ( const Bitmap& b );
Bitmap convertVerticalCrossToCubeMapFaces ( const Bitmap& b );

#endif // !__UTILS_CUBEMAP_H__

