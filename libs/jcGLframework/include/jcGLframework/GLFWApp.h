#ifndef __GLFW_APP_H__
#define __GLFW_APP_H__

#include <glad/gl.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <jcGLframework/debug.h>

#include <assert.h>

namespace jcGLframework
{
	class GLFWApp
	{
		GLFWwindow* window_ = nullptr;
		double timeStamp_ = glfwGetTime ();
		float deltaSeconds_ = 0;
		
	public:
		GLFWApp ( int32_t width = 1600, int32_t height = 900, const char* title = "GLFW App Example" )
		{
			glfwSetErrorCallback (
				[] ( int error, const char* description )
				{
					fprintf ( stderr, "Error: %s\n", description );
				}
			);

			if ( !glfwInit () )
				exit ( EXIT_FAILURE );

			glfwWindowHint ( GLFW_CONTEXT_VERSION_MAJOR, 4 );
			glfwWindowHint ( GLFW_CONTEXT_VERSION_MINOR, 6 );
			glfwWindowHint ( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
#ifndef NDEBUG
			glfwWindowHint ( GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE );
#else
			glfwWindowHint ( GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE );
#endif
			if ( width <= 0 || height <= 0 )
			{
				const GLFWvidmode* info = glfwGetVideoMode ( glfwGetPrimaryMonitor () );
				window_ = glfwCreateWindow ( info->width, info->height, ( title ? title : "Full Screen Example" ), nullptr, nullptr );
			}
			else
			{
				window_ = glfwCreateWindow ( width, height, ( title ? title : "Windowed Example" ), nullptr, nullptr );
			}

			if ( !window_ )
			{
				glfwTerminate ();
				exit ( EXIT_FAILURE );
			}

			glfwMakeContextCurrent ( window_ );
			gladLoadGL ( glfwGetProcAddress );
			glfwSwapInterval ( 1 );

#ifndef NDEBUG
			initDebug ();
#endif
		}

		virtual ~GLFWApp ()
		{
			glfwDestroyWindow ( window_ );
			glfwTerminate ();
		}

		GLFWwindow* getWindow () const
		{
			return window_;
		}

		float getDeltaSeconds () const
		{
			return deltaSeconds_;
		}

		void swapBuffers ()
		{
			glfwSwapBuffers ( window_ );
			glfwPollEvents ();
			assert ( glGetError () == GL_NO_ERROR );

			const double newTimeStamp = glfwGetTime ();
			deltaSeconds_ = static_cast< float >( newTimeStamp - timeStamp_ );
			deltaSeconds_ = glm::fclamp ( deltaSeconds_, 0.0f, 0.05f );
			timeStamp_ = newTimeStamp;
		}
	};
}

#endif // !__GLFW_APP_H__
