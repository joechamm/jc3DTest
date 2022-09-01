#ifndef __MY_CAMERA_H__
#define __MY_CAMERA_H__

#include <cassert>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>

using glm::mat4;
using glm::mat3;
using glm::mat2;
using glm::vec4;
using glm::vec3;
using glm::vec2;
using glm::quat;
using glm::ivec2;

// abstract helper class for positioning and orienting our camera
class AbstractCameraPositionerInterface
{
public:
	virtual ~AbstractCameraPositionerInterface () = default;
	virtual mat4 getViewMatrix () const = 0;
	virtual vec3 getPosition () const = 0;
};

class MyCamera
{
public:
	explicit MyCamera ( AbstractCameraPositionerInterface& positioner )
		: pPositioner_ ( &positioner )
		, projectionMatrix_ ( 1.0f )
	{}

	MyCamera ( const MyCamera& ) = default;
	MyCamera& operator = ( const MyCamera& ) = default;

	mat4 getViewMatrix () const
	{
		return pPositioner_->getViewMatrix ();
	}

	vec3 getPosition () const
	{
		return pPositioner_->getPosition ();
	}

	mat4 getProjectionMatrix () const
	{
		return projectionMatrix_;
	}

	mat4 getViewProjectionMatrix () const
	{
		return projectionMatrix_ * pPositioner_->getViewMatrix ();
	}

	void setProjectionMatrix ( const mat4& proj )
	{
		projectionMatrix_ = proj;
	}

	void setOrtho ( float left, float right, float bottom, float top )
	{
		projectionMatrix_ = glm::ortho ( left, right, bottom, top );
	}

	void setOrtho ( float left, float right, float bottom, float top, float zNear, float zFar )
	{
		projectionMatrix_ = glm::ortho ( left, right, bottom, top, zNear, zFar );
	}

	void setFrustum ( float left, float right, float bottom, float top, float zNear, float zFar )
	{
		projectionMatrix_ = glm::frustum ( left, right, bottom, top, zNear, zFar );
	}

	void setPerspective ( float fovRadians, float aspectRatio, float zNear, float zFar )
	{
		projectionMatrix_ = glm::perspective ( fovRadians, aspectRatio, zNear, zFar );
	}

	void setPerspectiveFOV ( float fovRadians, float width, float height, float zNear, float zFar )
	{
		projectionMatrix_ = glm::perspectiveFov ( fovRadians, width, height, zNear, zFar );
	}

	void setInfinitePerspective ( float fovRadians, float aspectRatio, float zNear )
	{
		projectionMatrix_ = glm::infinitePerspective ( fovRadians, aspectRatio, zNear );
	}

	void setPerspectiveDegs ( float fovDegs, float aspectRatio, float zNear, float zFar )
	{
		projectionMatrix_ = glm::perspective ( glm::radians ( fovDegs ), aspectRatio, zNear, zFar );
	}

	void setPerspectiveDegsFOV ( float fovDegs, float width, float height, float zNear, float zFar )
	{
		projectionMatrix_ = glm::perspectiveFov ( glm::radians ( fovDegs ), width, height, zNear, zFar );
	}

	void setInfinitePerspectiveDegs ( float fovDegs, float aspectRatio, float zNear )
	{
		projectionMatrix_ = glm::infinitePerspective ( glm::radians ( fovDegs ), aspectRatio, zNear );
	}

private:
	const AbstractCameraPositionerInterface* pPositioner_;

	mat4 projectionMatrix_;
};

class CameraPositioner_FirstPerson : public AbstractCameraPositionerInterface
{
public:
	struct Movement
	{
		bool forward_ = false;
		bool backward_ = false;
		bool left_ = false;
		bool right_ = false;
		bool up_ = false;
		bool down_ = false;
		bool fastSpeed_ = false;
	} movement_;

	float mouseSpeed_ = 4.0f;
	float acceleration_ = 150.0f;
	float damping_ = 0.2f;
	float maxSpeed_ = 10.0f;
	float fastCoef_ = 10.0f;

	int screenWidth_ = 800;
	int screenHeight_ = 600;

private:
	bool mouseDown_ = false;
	vec2 mouseDownPosition_ = vec2 ( 0.0f );
	vec2 mousePosition_ = vec2 ( 0.0f );
	vec3 cameraPosition_ = vec3 ( 0.0f, 0.0f, -10.0f );
	quat cameraOrientation_ = quat ( 1.0f, 0.0f, 0.0f, 0.0f );
	quat currentOrientation_ = quat ( 1.0f, 0.0f, 0.0f, 0.0f );
	quat previousOrientation_ = quat ( 1.0f, 0.0f, 0.0f, 0.0f );
	vec3 moveSpeed_ = vec3 ( 0.0f );
	vec3 up_ = vec3 ( 0.0f, 1.0f, 0.0f );

public:
	CameraPositioner_FirstPerson () = default;
	CameraPositioner_FirstPerson(const vec3& pos, const vec3& target, const vec3& up)
		: cameraPosition_(pos)
		, cameraOrientation_(glm::lookAt(pos, target, up))
		, currentOrientation_(1.0f, 0.0f, 0.0f, 0.0f)
		, previousOrientation_(cameraOrientation_)
		, up_(up) {}
	
	void update ( double deltaSeconds, const vec2& mousePos, bool mousePressed )
	{
		if ( mousePressed )
		{
			if ( mouseDown_ )
			{
				mouseMove ( mousePos );
			}
			else
			{
				mousePress ( mousePos );
			}
		}
		else
		{
			if ( mouseDown_ )
			{
				mouseRelease ( mousePos );
			}
		}

		cameraOrientation_ = currentOrientation_ * previousOrientation_;

		// Establish camera's coordinate system 
		const mat4 v = glm::mat4_cast ( cameraOrientation_ );
		const vec3 forward = -vec3 ( v [ 0 ][ 2 ], v [ 1 ][ 2 ], v [ 2 ][ 2 ] );
		const vec3 right = vec3 ( v [ 0 ][ 0 ], v [ 1 ][ 0 ], v [ 2 ][ 0 ] );
		const vec3 up = glm::cross ( right, forward );

		// user input only controls acceleration
		vec3 accel = vec3 ( 0.0f );
		if ( movement_.forward_ ) accel += forward;
		if ( movement_.backward_ ) accel -= forward;
		if ( movement_.left_ ) accel -= right;
		if ( movement_.right_ ) accel += right;
		if ( movement_.up_ ) accel += up;
		if ( movement_.down_ ) accel -= up;
		if ( movement_.fastSpeed_ ) accel *= fastCoef_;

		if ( vec3 ( 0.0f ) == accel )
		{
			moveSpeed_ -= moveSpeed_ * std::min ( ( 1.0f / damping_ ) * static_cast< float >( deltaSeconds ), 1.0f );
		}
		else
		{
			moveSpeed_ += accel * acceleration_ * static_cast< float >( deltaSeconds );
			const float maxSpeed = movement_.fastSpeed_ ? maxSpeed_ * fastCoef_ : maxSpeed_;
			if ( glm::length ( moveSpeed_ ) > maxSpeed )
			{
				moveSpeed_ = glm::normalize ( moveSpeed_ ) * maxSpeed;
			}
		}

		cameraPosition_ += moveSpeed_ * static_cast< float >( deltaSeconds );
	}

	virtual mat4 getViewMatrix () const override
	{
		const mat4 t = glm::translate ( mat4 ( 1.0f ), -cameraPosition_ );
		const mat4 r = glm::mat4_cast ( cameraOrientation_ );
		return r * t;
	}

	virtual vec3 getPosition () const override
	{
		return cameraPosition_;
	}

	void setPosition ( const vec3& pos )
	{
		cameraPosition_ = pos;
	}

	void resetMousePosition ( const vec2& p )
	{
		mousePosition_ = p;
		mouseDownPosition_ = p;
	}

	void setUpVector ( const vec3& up )
	{
		const mat4 view = getViewMatrix ();
		const vec3 dir = -vec3 ( view [ 0 ][ 2 ], view [ 1 ][ 2 ], view [ 2 ][ 2 ] );
		cameraOrientation_ = glm::lookAt ( cameraPosition_, cameraPosition_ + dir, up );
		previousOrientation_ = cameraOrientation_;
		currentOrientation_ = glm::quat_identity<float, glm::packed_highp> ();
	}

	inline void lookAt ( const vec3& pos, const vec3& target, const vec3& up )
	{
		cameraPosition_ = pos;
		cameraOrientation_ = glm::lookAt ( pos, target, up );
		previousOrientation_ = cameraOrientation_;
		currentOrientation_ = glm::quat_identity<float, glm::packed_highp> ();
	}
private:

	ivec2 getScreenCenter () const
	{
		return ivec2 ( ( screenWidth_ - 1 ) / 2, ( screenHeight_ - 1 ) / 2 );
	}

	vec2 getScaledCoords ( float px, float py ) const
	{
		float s = static_cast< float >( glm::min ( screenWidth_, screenHeight_ ) );
		float oneOverS = 1.0f / s;
		float x = 2.0f * px + static_cast< float >( 1 - screenWidth_ );
		float y = 2.0f * py + static_cast< float >( 1 - screenHeight_ );
		return vec2 ( oneOverS * x, -oneOverS * y );
	}

	float getZCoord ( const vec2& scaledCoords ) const
	{
		const float hemiSphereRadius = 1.0f;
		const float rsq = hemiSphereRadius * hemiSphereRadius;
		const float rsqOverTwo = 0.5f * rsq;
		float pdotp = glm::dot ( scaledCoords, scaledCoords );
		if ( pdotp <= rsqOverTwo )
			return glm::sqrt ( rsq - pdotp );
		return rsqOverTwo * glm::inversesqrt ( pdotp );
	}

	vec3 getProjectedPoint ( float mx, float my ) const
	{
		vec2 scaledCoords = getScaledCoords ( mx, my );
		float z = getZCoord ( scaledCoords );
		return vec3 ( scaledCoords.x, scaledCoords.y, z );
	}

	void mousePress ( const vec2& mousePos )
	{
		mouseDownPosition_ = mousePos;
		mousePosition_ = mousePos;
		mouseDown_ = true;
	}

	void mouseRelease ( const vec2& mousePos )
	{
		if ( !mouseDown_ )
			return;

		previousOrientation_ = currentOrientation_ * previousOrientation_;
		currentOrientation_ = glm::quat_identity<float, glm::packed_highp> ();
		mousePosition_ = mousePos;
		mouseDown_ = false;
	}

	void mouseMove ( const vec2& mousePos )
	{
		if ( !mouseDown_ )
			return;

		vec3 orig = getProjectedPoint ( mouseDownPosition_.x, mouseDownPosition_.y );
		vec3 dest = getProjectedPoint ( mousePos.x, mousePos.y );

		currentOrientation_ = glm::rotation ( glm::normalize ( orig ), glm::normalize ( dest ) );
		mousePosition_ = mousePos;
	}
};

//
//class CameraPositioner_OrbitTarget : public AbstractCameraPositionerInterface
//{
//public:
//	float radius_ = 10.0f;
//	vec3 target_ = vec3 ( 0.0f );
//
//	float mouseSpeed_ = 4.0f;
//	float acceleration_ = 150.0f;
//	float damping_ = 0.2f;
//	float maxSpeed_ = 10.0f;
//	float fastCoef_ = 10.0f;
//
//	int screenWidth_ = 800;
//	int screenHeight_ = 600;
//
//private:
//	bool mouseDown_ = false;
//	vec2 mouseDownPosition_ = vec2 ( 0.0f );
//	vec2 mousePosition_ = vec2 ( 0.0f );
//	vec3 cameraPosition_ = vec3 ( 0.0f, 0.0f, -10.0f );
//	quat cameraOrientation_ = quat ( 1.0f, 0.0f, 0.0f, 0.0f );
//	quat currentOrientation_ = quat ( 1.0f, 0.0f, 0.0f, 0.0f );
//	quat previousOrientation_ = quat ( 1.0f, 0.0f, 0.0f, 0.0f );
//	quat deltaRotation_ = quat ( 1.0f, 0.0f, 0.0f, 0.0f );
//	vec3 moveSpeed_ = vec3 ( 0.0f );
//	vec3 up_ = vec3 ( 0.0f, 1.0f, 0.0f );
//
//public:
//	CameraPositioner_OrbitTarget () = default;
//	CameraPositioner_OrbitTarget ( const vec3& pos, const vec3& target, const vec3& up )
//		: cameraPosition_ ( pos )
//		, cameraOrientation_ ( glm::lookAt ( pos, target, up ) )
//		, currentOrientation_ ( 1.0f, 0.0f, 0.0f, 0.0f )
//		, previousOrientation_ ( cameraOrientation_ )
//		, up_ ( up )
//	{}
//
//	void update ( double deltaSeconds, const vec2& mousePos, bool mousePressed )
//	{
//		if ( mousePressed )
//		{
//			if ( mouseDown_ )
//			{
//				mouseMove ( mousePos );
//			}
//			else
//			{
//				mousePress ( mousePos );
//			}
//		}
//		else
//		{
//			if ( mouseDown_ )
//			{
//				mouseRelease ( mousePos );
//			}
//		}
//
//		cameraOrientation_ = currentOrientation_ * previousOrientation_;
//
//		// Establish camera's coordinate system 
//		const mat4 v = glm::mat4_cast ( cameraOrientation_ );
//		const vec3 forward = -vec3 ( v [ 0 ][ 2 ], v [ 1 ][ 2 ], v [ 2 ][ 2 ] );
//		const vec3 right = vec3 ( v [ 0 ][ 0 ], v [ 1 ][ 0 ], v [ 2 ][ 0 ] );
//		const vec3 up = glm::cross ( right, forward );
//
//		// user input only controls acceleration
//		vec3 accel = vec3 ( 0.0f );
//		if ( movement_.forward_ ) accel += forward;
//		if ( movement_.backward_ ) accel -= forward;
//		if ( movement_.left_ ) accel -= right;
//		if ( movement_.right_ ) accel += right;
//		if ( movement_.up_ ) accel += up;
//		if ( movement_.down_ ) accel -= up;
//		if ( movement_.fastSpeed_ ) accel *= fastCoef_;
//
//		if ( vec3 ( 0.0f ) == accel )
//		{
//			moveSpeed_ -= moveSpeed_ * std::min ( ( 1.0f / damping_ ) * static_cast< float >( deltaSeconds ), 1.0f );
//		}
//		else
//		{
//			moveSpeed_ += accel * acceleration_ * static_cast< float >( deltaSeconds );
//			const float maxSpeed = movement_.fastSpeed_ ? maxSpeed_ * fastCoef_ : maxSpeed_;
//			if ( glm::length ( moveSpeed_ ) > maxSpeed )
//			{
//				moveSpeed_ = glm::normalize ( moveSpeed_ ) * maxSpeed;
//			}
//		}
//
//		cameraPosition_ += moveSpeed_ * static_cast< float >( deltaSeconds );
//	}
//
//	virtual mat4 getViewMatrix () const override
//	{
//		const mat4 t = glm::translate ( mat4 ( 1.0f ), -cameraPosition_ );
//		const mat4 r = glm::mat4_cast ( cameraOrientation_ );
//		return r * t;
//	}
//
//	virtual vec3 getPosition () const override
//	{
//		return cameraPosition_;
//	}
//
//	void setPosition ( const vec3& pos )
//	{
//		cameraPosition_ = pos;
//	}
//
//	void resetMousePosition ( const vec2& p )
//	{
//		mousePosition_ = p;
//		mouseDownPosition_ = p;
//	}
//
//	void setUpVector ( const vec3& up )
//	{
//		const mat4 view = getViewMatrix ();
//		const vec3 dir = -vec3 ( view [ 0 ][ 2 ], view [ 1 ][ 2 ], view [ 2 ][ 2 ] );
//		cameraOrientation_ = glm::lookAt ( cameraPosition_, cameraPosition_ + dir, up );
//		previousOrientation_ = cameraOrientation_;
//		currentOrientation_ = glm::quat_identity<float, glm::packed_highp> ();
//	}
//
//	inline void lookAt ( const vec3& pos, const vec3& target, const vec3& up )
//	{
//		cameraPosition_ = pos;
//		cameraOrientation_ = glm::lookAt ( pos, target, up );
//		previousOrientation_ = cameraOrientation_;
//		currentOrientation_ = glm::quat_identity<float, glm::packed_highp> ();
//	}
//private:
//
//	ivec2 getScreenCenter () const
//	{
//		return ivec2 ( ( screenWidth_ - 1 ) / 2, ( screenHeight_ - 1 ) / 2 );
//	}
//
//	vec2 getScaledCoords ( float px, float py ) const
//	{
//		float s = static_cast< float >( glm::min ( screenWidth_, screenHeight_ ) );
//		float oneOverS = 1.0f / s;
//		float x = 2.0f * px + static_cast< float >( 1 - screenWidth_ );
//		float y = 2.0f * py + static_cast< float >( 1 - screenHeight_ );
//		return vec2 ( oneOverS * x, -oneOverS * y );
//	}
//
//	float getZCoord ( const vec2& scaledCoords ) const
//	{
//		const float hemiSphereRadius = 1.0f;
//		const float rsq = hemiSphereRadius * hemiSphereRadius;
//		const float rsqOverTwo = 0.5f * rsq;
//		float pdotp = glm::dot ( scaledCoords, scaledCoords );
//		if ( pdotp <= rsqOverTwo )
//			return glm::sqrt ( rsq - pdotp );
//		return rsqOverTwo * glm::inversesqrt ( pdotp );
//	}
//
//	vec3 getProjectedPoint ( float mx, float my ) const
//	{
//		vec2 scaledCoords = getScaledCoords ( mx, my );
//		float z = getZCoord ( scaledCoords );
//		return vec3 ( scaledCoords.x, scaledCoords.y, z );
//	}
//
//	void mousePress ( const vec2& mousePos )
//	{
//		mouseDownPosition_ = mousePos;
//		mousePosition_ = mousePos;
//		mouseDown_ = true;
//	}
//
//	void mouseRelease ( const vec2& mousePos )
//	{
//		if ( !mouseDown_ )
//			return;
//
//		previousOrientation_ = currentOrientation_ * previousOrientation_;
//		currentOrientation_ = glm::quat_identity<float, glm::packed_highp> ();
//		mousePosition_ = mousePos;
//		mouseDown_ = false;
//	}
//
//	void mouseMove ( const vec2& mousePos )
//	{
//		if ( !mouseDown_ )
//			return;
//
//		vec3 orig = getProjectedPoint ( mouseDownPosition_.x, mouseDownPosition_.y );
//		vec3 dest = getProjectedPoint ( mousePos.x, mousePos.y );
//
//		currentOrientation_ = glm::rotation ( glm::normalize ( orig ), glm::normalize ( dest ) );
//		mousePosition_ = mousePos;
//	}
//};

#endif // !__MY_CAMERA_H__
