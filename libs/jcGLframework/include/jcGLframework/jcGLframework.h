#ifndef __JC_GLFRAMEWORK_H__
#define __JC_GLFRAMEWORK_H__

#include <glad/gl.h>
#include <cstdint>

namespace jcGLframework 
{
	constexpr GLuint kIdxBind_PerFrameData = 7;
	constexpr GLuint kIdxBind_Materials = 6;
	constexpr GLuint kIdxBind_ModelMatrices = 5;
	constexpr GLuint kIdxBind_Vertices = 1;
}
#endif // !__JC_GLFRAMEWORK_H__
