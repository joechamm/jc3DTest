#pragma once

#include <glad/glad.h>

/// The GLShader, GLProgram, and GLBuffer classes use the RAII idiom
class GLShader 
{
public:
	explicit GLShader ( const char* filename );
	GLShader ( GLenum type, const char* text, const char* debugFileName = "" );
	~GLShader ();
	GLenum getType () const { return type_;  }
	GLuint getHandle () const { return handle_;  }
private:
	GLenum type_;
	GLuint handle_;
};

class GLProgram 
{
public:
	GLProgram ( const GLShader& a );
	GLProgram ( const GLShader& a, const GLShader& b );
	GLProgram ( const GLShader& a, const GLShader& b, const GLShader& c );
	GLProgram ( const GLShader& a, const GLShader& b, const GLShader& c, const GLShader& d, const GLShader& e );
	~GLProgram ();

	void useProgram () const;
	GLuint getHandle () const { return handle_; }

private:
	GLuint handle_;
};

/// return the shader type based on file's extension
GLenum GLShaderTypeFromFileName ( const char* fileName );

class GLBuffer 
{
public:
	GLBuffer ( GLsizeiptr size, const void* data, GLbitfield flags );
	~GLBuffer ();

	GLuint getHandle () const { return handle_; }

private:
	GLuint handle_;
};