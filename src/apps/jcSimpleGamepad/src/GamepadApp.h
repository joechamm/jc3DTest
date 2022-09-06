#ifndef __GAMEPAD_APP_H__
#define __GAMEPAD_APP_H__

#include <glad/gl.h>
#include <glfw/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

namespace jcGLframework
{
	using glm::mat4;
	using glm::vec4;
	using glm::vec3;
	using glm::quat;

	class GamepadCamera
	{
		vec3 cameraPosition_ = vec3 ( 0.f, 0.f, 5.f );
		
		vec3 linearVelocity_ = vec3 ( 0.f );

		quat cameraOrientation_ = quat ( vec3 ( 0.f ) );
		quat deltaRotation_ = quat ( vec3 ( 0.f ) );

		float linearVelocityScale_ = 1.0f;
		float angularVelocityScale_ = 1.0f;

		float linearVelocityDamping_ = 0.95f;
		float angularVelocityDamping_ = 0.95f;

		mat4 projectionMatrix_ = mat4 ( 1.f );
		mat4 viewMatrix_ = mat4 ( 1.f );

	public:
		GamepadCamera ( const vec3& pos, const vec3& target, const vec3& up )
			: cameraPosition_ ( pos )
			, cameraOrientation_ ( glm::lookAt ( pos, target, up ) )		
			, viewMatrix_ ( glm::lookAt ( pos, target, up ) )
		{}

		void setPerspectiveDegs ( float fovDegs, float aspect, float zNear, float zFar )
		{
			projectionMatrix_ = glm::perspective ( glm::radians ( fovDegs ), aspect, zNear, zFar );
		}

		void setInfinitePerspectiveDegs ( float fovDegs, float aspectRatio, float zNear )
		{
			projectionMatrix_ = glm::infinitePerspective ( glm::radians ( fovDegs ), aspectRatio, zNear );
		}

		void setOrthoProjection ( float left, float right, float bottom, float top )
		{
			projectionMatrix_ = glm::ortho ( left, right, bottom, top );
		}

		const mat4& getProjectionMatrix () const
		{
			return projectionMatrix_;
		}

		const mat4& getViewMatrix () const
		{
			return viewMatrix_;
		}

		void update ( float dt )
		{
			const vec3 newPosition = cameraPosition_ + linearVelocity_ * linearVelocityScale_ * dt;
			const quat newOrientation = ( 0.5f * dt * deltaRotation_ + glm::quat_identity<float, glm::packed_highp> () ) * cameraOrientation_;

			linearVelocity_ *= linearVelocityDamping_;
			deltaRotation_ *= angularVelocityDamping_;

			constexpr float kEpsilon = 0.01f;

			// clamp
			if ( glm::length ( linearVelocity_ ) < kEpsilon )
			{
				linearVelocity_ = vec3 ( 0.f );
			}

			if ( glm::length ( deltaRotation_ ) < kEpsilon )
			{
				deltaRotation_ = quat ( vec3 ( 0.f ) );
			}

			const mat4 v = glm::mat4_cast ( newOrientation );
			const vec3 forward = -vec3 ( v [ 0 ][ 2 ], v [ 1 ][ 2 ], v [ 2 ][ 2 ] );
			const vec3 right = vec3 ( v [ 0 ][ 0 ], v [ 1 ][ 0 ], v [ 2 ][ 0 ] );
			const vec3 up = glm::cross ( right, forward );

			const mat4 t = glm::translate ( mat4 ( 1.f ), -newPosition );
			
			viewMatrix_ = v * t;
			cameraPosition_ = newPosition;
			cameraOrientation_ = glm::normalize ( newOrientation );			
		}

		void impulseForward ( float dp )
		{
			const mat4 v = glm::mat4_cast ( cameraOrientation_ );
			const vec3 forward = vec3 ( v [ 0 ][ 2 ], v [ 1 ][ 2 ], v [ 2 ][ 2 ] );
			const vec3 newLinearVelocity = linearVelocity_ + glm::normalize ( forward ) * dp * linearVelocityScale_;
			const float linVelMag = glm::length ( newLinearVelocity );
			constexpr float kMaxVelocity = 10.0f;

			if ( linVelMag > kMaxVelocity )
			{
				linearVelocity_ = glm::normalize ( newLinearVelocity ) * kMaxVelocity;
			}
			else
			{
				linearVelocity_ = newLinearVelocity;
			}
		}

		void impulseUp ( float dp )
		{
			const mat4 v = glm::mat4_cast ( cameraOrientation_ );
			const vec3 forward = vec3 ( v [ 0 ][ 2 ], v [ 1 ][ 2 ], v [ 2 ][ 2 ] );
			const vec3 right = vec3 ( v [ 0 ][ 0 ], v [ 1 ][ 0 ], v [ 2 ][ 0 ] );
			const vec3 up = glm::cross ( right, forward );
			const vec3 newLinearVelocity = linearVelocity_ + glm::normalize ( up ) * dp * linearVelocityScale_;
			const float linVelMag = glm::length ( newLinearVelocity );
			constexpr float kMaxVelocity = 10.0f;

			if ( linVelMag > kMaxVelocity )
			{
				linearVelocity_ = glm::normalize ( newLinearVelocity ) * kMaxVelocity;
			}
			else
			{
				linearVelocity_ = newLinearVelocity;
			}
		}

		void impulseRight ( float dp )
		{
			const mat4 v = glm::mat4_cast ( cameraOrientation_ );
			const vec3 right = vec3 ( v [ 0 ][ 0 ], v [ 1 ][ 0 ], v [ 2 ][ 0 ] );
			const vec3 newLinearVelocity = linearVelocity_ + glm::normalize ( right ) * dp * linearVelocityScale_;
			const float linVelMag = glm::length ( newLinearVelocity );
			constexpr float kMaxVelocity = 10.0f;

			if ( linVelMag > kMaxVelocity )
			{
				linearVelocity_ = glm::normalize ( newLinearVelocity ) * kMaxVelocity;
			}
			else
			{
				linearVelocity_ = newLinearVelocity;
			}
		}

		void impulse ( float dright, float dup, float dforward )
		{
			const mat4 v = glm::mat4_cast ( cameraOrientation_ );
			const vec3 forward = vec3 ( v [ 0 ][ 2 ], v [ 1 ][ 2 ], v [ 2 ][ 2 ] );
			const vec3 right = vec3 ( v [ 0 ][ 0 ], v [ 1 ][ 0 ], v [ 2 ][ 0 ] );
			const vec3 up = glm::cross ( right, forward );
			const vec3 newLinearVelocity = linearVelocity_ + ( glm::normalize ( right ) * dright + glm::normalize ( up ) * dup + glm::normalize ( forward ) * dforward ) * linearVelocityScale_;
			const float linVelMag = glm::length ( newLinearVelocity );
			constexpr float kMaxVelocity = 10.0f;

			if ( linVelMag > kMaxVelocity )
			{
				linearVelocity_ = glm::normalize ( newLinearVelocity ) * kMaxVelocity;
			}
			else
			{
				linearVelocity_ = newLinearVelocity;
			}
		}

		void addPitchDegs ( float deltaPitchDegs )
		{
			deltaRotation_ *= glm::angleAxis ( glm::radians ( deltaPitchDegs ), vec3 ( 1.f, 0.f, 0.f ) );
		}

		void addYawDegs ( float deltaYawDegs )
		{
			deltaRotation_ *= glm::angleAxis ( glm::radians ( deltaYawDegs ), vec3 ( 0.f, 1.f, 0.f ) );
		}

		void addRollDegs ( float deltaRollDegs )
		{
			deltaRotation_ *= glm::angleAxis ( glm::radians ( deltaRollDegs ), vec3 ( 0.f, 0.f, -1.f ) );
		}
	};

	class GamepadCameraAlt
	{
		vec3 cameraPosition_ = vec3 ( 0.f, 0.f, 5.f );
		vec3 linearVelocity_ = vec3 ( 0.f );

		quat cameraOrientation_ = quat ( vec3 ( 0.f ) );
		
		float linearVelocityScale_ = 1.0f;	
		float linearVelocityDamping_ = 0.9f;

		float maxPitchDegrees_ = 30.0f;
		float maxYawDegrees_ = 60.0f;
		float maxRollDegrees_ = 60.0f;

		mat4 projectionMatrix_ = mat4 ( 1.f );
		mat4 viewMatrix_ = mat4 ( 1.f );

	public:
		GamepadCameraAlt ( const vec3& pos, const vec3& target, const vec3& up )
			: cameraPosition_ ( pos )
			, cameraOrientation_ ( glm::lookAt ( pos, target, up ) )
			, viewMatrix_ ( glm::lookAt ( pos, target, up ) )
		{}

		void setPerspectiveDegs ( float fovDegs, float aspect, float zNear, float zFar )
		{
			projectionMatrix_ = glm::perspective ( glm::radians ( fovDegs ), aspect, zNear, zFar );
		}

		void setInfinitePerspectiveDegs ( float fovDegs, float aspectRatio, float zNear )
		{
			projectionMatrix_ = glm::infinitePerspective ( glm::radians ( fovDegs ), aspectRatio, zNear );
		}

		void setOrthoProjection ( float left, float right, float bottom, float top )
		{
			projectionMatrix_ = glm::ortho ( left, right, bottom, top );
		}

		const mat4& getProjectionMatrix () const
		{
			return projectionMatrix_;
		}

		const mat4& getViewMatrix () const
		{
			return viewMatrix_;
		}

		void update ( float dt )
		{
			const vec3 newPosition = cameraPosition_ + linearVelocity_ * linearVelocityScale_ * dt;
			
			linearVelocity_ *= linearVelocityDamping_;
			
			constexpr float kEpsilon = 0.01f;

			// clamp
			if ( glm::length ( linearVelocity_ ) < kEpsilon )
			{
				linearVelocity_ = vec3 ( 0.f );
			}

			const mat4 v = glm::mat4_cast ( cameraOrientation_ );
			const vec3 forward = -vec3 ( v [ 0 ][ 2 ], v [ 1 ][ 2 ], v [ 2 ][ 2 ] );
			const vec3 right = vec3 ( v [ 0 ][ 0 ], v [ 1 ][ 0 ], v [ 2 ][ 0 ] );
			const vec3 up = glm::cross ( right, forward );

			const mat4 t = glm::translate ( mat4 ( 1.f ), -newPosition );

			viewMatrix_ = v * t;
			cameraPosition_ = newPosition;
		}

		void impulse ( float dright, float dup, float dforward )
		{
			const mat4 v = glm::mat4_cast ( cameraOrientation_ );
			const vec3 forward = vec3 ( v [ 0 ][ 2 ], v [ 1 ][ 2 ], v [ 2 ][ 2 ] );
			const vec3 right = vec3 ( v [ 0 ][ 0 ], v [ 1 ][ 0 ], v [ 2 ][ 0 ] );
			const vec3 up = glm::cross ( right, forward );
			const vec3 newLinearVelocity = linearVelocity_ + ( glm::normalize ( right ) * dright + glm::normalize ( up ) * dup + glm::normalize ( forward ) * dforward ) * linearVelocityScale_;
			const float linVelMag = glm::length ( newLinearVelocity );
			constexpr float kMaxVelocity = 10.0f;

			if ( linVelMag > kMaxVelocity )
			{
				linearVelocity_ = glm::normalize ( newLinearVelocity ) * kMaxVelocity;
			}
			else
			{
				linearVelocity_ = newLinearVelocity;
			}
		}

		void setYawPitchRoll ( float yaw, float pitch, float roll )
		{
			cameraOrientation_ = glm::yawPitchRoll ( glm::radians ( yaw * maxYawDegrees_ ), glm::radians ( pitch * maxPitchDegrees_ ), glm::radians ( roll * maxRollDegrees_ ) );
		}
	};

	class GamepadApp 
	{
		GLFWwindow* window_ = nullptr;
		double timeStamp_ = glfwGetTime ();
		float deltaSeconds_ = 0;

		GLFWgamepadstate gamepadState_;
		int jid_ = -1;

//		GamepadCamera camera_;
		GamepadCameraAlt camera_;

	public:
		GamepadApp ( int32_t width = 1600, int32_t height = 900, const char* title = "Gamepad App Example" );
		virtual ~GamepadApp ();

		virtual void processInput ();

		float getAxisState ( int axisId ) const;

		bool buttonPressed ( int buttonId ) const;

		const mat4& getProjectionMatrix () const;
		const mat4& getViewMatrix () const;

		void updateCamera ( float dt );

		void setCameraPerspectiveDegs ( float fovDegs, float aspect, float zNear, float zFar );
		void setInfinitePerspectiveDegs ( float fovDegs, float aspectRatio, float zNear );
		void setOrthoProjection ( float left, float right, float bottom, float top );
	
		GLFWwindow* getWindow () const;

		float getDeltaSeconds () const;	

		void swapBuffers ();		

	protected:
		virtual void updateGamepad ();		

		bool findFirstGamepad ();
				
	};
}


#endif // __GAMEPAD_APP_H__