#include "GamepadApp.h"
#include <jcGLframework/debug.h>

namespace jcGLframework 
{
	GamepadApp::GamepadApp ( int32_t width, int32_t height, const char* title )
		: camera_(vec3(0.f, 0.f, 10.f), vec3(0.f), vec3(0.f, 1.f, 0.f))
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

		if ( findFirstGamepad () )
		{
			updateGamepad ();
		}
		else
		{
			std::cout << "Failed to find GLFW gamepad" << std::endl;
		}

		const float aspect = width / ( float ) height;
		const float zNear = 0.1f;
		const float zFar = 100.0f;
		camera_.setPerspectiveDegs ( 45.0f, aspect, zNear, zFar );
	}

	GamepadApp::~GamepadApp () 
	{
		glfwDestroyWindow ( window_ );
		glfwTerminate ();	
	}

	GLFWwindow* GamepadApp::getWindow () const
	{
		return window_;
	}

	float GamepadApp::getDeltaSeconds () const
	{
		return deltaSeconds_;
	}

	void GamepadApp::swapBuffers ()
	{
		glfwSwapBuffers ( window_ );
		glfwPollEvents ();
		assert ( glGetError () == GL_NO_ERROR );

		const double newTimeStamp = glfwGetTime ();
		deltaSeconds_ = static_cast< float >( newTimeStamp - timeStamp_ );
		deltaSeconds_ = glm::fclamp ( deltaSeconds_, 0.0f, 0.05f );
		timeStamp_ = newTimeStamp;
	}

	void GamepadApp::processInput ()
	{
		updateGamepad ();

		constexpr float kThresh = 0.001f;
		if ( glm::abs ( gamepadState_.axes [ GLFW_GAMEPAD_AXIS_RIGHT_Y ] ) > kThresh )
		{
			camera_.impulseForward ( gamepadState_.axes [ GLFW_GAMEPAD_AXIS_RIGHT_Y ] );
		}
		if ( glm::abs ( gamepadState_.axes [ GLFW_GAMEPAD_AXIS_RIGHT_X ] ) > kThresh )
		{
			camera_.impulseRight ( gamepadState_.axes [ GLFW_GAMEPAD_AXIS_RIGHT_X ] );
		}
		if ( glm::abs ( gamepadState_.axes [ GLFW_GAMEPAD_AXIS_LEFT_X ] ) > kThresh )
		{
			camera_.addYawDegs ( gamepadState_.axes [ GLFW_GAMEPAD_AXIS_LEFT_X ] );
		}
		if ( glm::abs ( gamepadState_.axes [ GLFW_GAMEPAD_AXIS_LEFT_Y ] ) > kThresh )
		{
			camera_.addPitchDegs ( gamepadState_.axes [ GLFW_GAMEPAD_AXIS_LEFT_Y ] );
		}
	}

	void GamepadApp::updateCamera ( float dt )
	{
		camera_.update ( dt );
	}

	void GamepadApp::setCameraPerspectiveDegs ( float fovDegs, float aspect, float zNear, float zFar )
	{
		camera_.setPerspectiveDegs ( fovDegs, aspect, zNear, zFar );
	}

	const mat4& GamepadApp::getViewMatrix () const
	{
		return camera_.getViewMatrix ();
	}

	const mat4& GamepadApp::getProjectionMatrix () const
	{
		return camera_.getProjectionMatrix ();
	}

	void GamepadApp::updateGamepad ()
	{
		assert ( jid_ != -1 );

		glfwGetGamepadState ( jid_, &gamepadState_ );
	}

	float GamepadApp::getAxisState ( int axisId ) const
	{
		assert ( axisId <= GLFW_GAMEPAD_AXIS_LAST && axisId >= 0 );

		return gamepadState_.axes [ axisId ];
	}

	bool GamepadApp::buttonPressed ( int buttonId ) const
	{
		assert ( buttonId <= GLFW_GAMEPAD_BUTTON_LAST && buttonId >= 0 );

		return gamepadState_.buttons [ buttonId ];
	}

	bool GamepadApp::findFirstGamepad ()
	{
		for ( int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; jid++ )
		{
			if ( glfwJoystickIsGamepad ( jid ) )
			{
				jid_ = jid;
				return true;
			}
		}

		jid_ = -1;
		return false;
	}
}
