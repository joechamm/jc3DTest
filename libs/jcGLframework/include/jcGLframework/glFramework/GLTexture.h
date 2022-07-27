#ifndef __GL_TEXTURE_H__
#define __GL_TEXTURE_H__

#include <glad/gl.h>
#include <string>

class GLTexture
{
	GLenum type_ = 0;
	GLuint handle_ = 0;
	GLuint64 handleBindless_ = 0;

public:
	GLTexture ( GLenum type, const char* filename );
	GLTexture ( GLenum type, const std::string& filename );
	GLTexture ( GLenum type, const char* filename, GLenum clamp );
	GLTexture ( GLenum type, const std::string& filename, GLenum clamp );
	GLTexture ( GLenum type, int width, int height, GLenum internalFormat );
	GLTexture ( int w, int h, const void* img );
	~GLTexture ();
	GLTexture ( const GLTexture& ) = delete;
	GLTexture ( GLTexture&& );
	GLenum getType () const
	{
		return type_;
	}

	GLuint getHandle () const
	{
		return handle_;
	}

	GLuint64 getHandleBindless () const
	{
		return handleBindless_;
	}
};

#endif // !__GL_TEXTURE_H__
