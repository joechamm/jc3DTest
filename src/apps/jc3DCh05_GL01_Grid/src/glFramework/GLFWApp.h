#ifndef __GLFWAPP_H__
#define __GLFWAPP_H__

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "debug.h"

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

class GLFWApp
{
private:
	GLFWwindow* window_ = nullptr;
	double t_ = glfwGetTime ();
	float dt_ = 0;

public:
	GLFWApp ()
	{
		glfwSetErrorCallback (
			[]( int err, const char* description ) {
				fprintf ( stderr, "Error: %s\n", description );
			}
		);

		if ( !glfwInit () )
		{
			exit ( EXIT_FAILURE );
		}

		glfwWindowHint ( GLFW_CONTEXT_VERSION_MAJOR, 4 );
		glfwWindowHint ( GLFW_CONTEXT_VERSION_MINOR, 6 );
		glfwWindowHint ( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
		glfwWindowHint ( GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE );

		const GLFWvidmode* info = glfwGetVideoMode ( glfwGetPrimaryMonitor () );

		window_ = glfwCreateWindow ( info->width, info->height, "GLFW App", nullptr, nullptr );

		if ( !window_ )
		{
			glfwTerminate ();
			exit ( EXIT_FAILURE );
		}

		glfwMakeContextCurrent ( window_ );
		gladLoadGL ( glfwGetProcAddress );
		glfwSwapInterval ( 0 );

		initDebug ();
	}

	~GLFWApp ()
	{
		glfwDestroyWindow ( window_ );
		glfwTerminate ();
	}

	GLFWwindow* getWindow () const
	{
		return window_;
	}

	float getDeltaTime () const
	{
		return dt_;
	}

	void swapBuffers ()
	{
		glfwSwapBuffers ( window_ );
		glfwPollEvents ();
		assert ( glGetError () == GL_NO_ERROR );

		const double currentTime = glfwGetTime ();
		dt_ = static_cast<float>(currentTime - t_);
		t_ = currentTime;
	}
};


#endif // __GLFWAPP_H__