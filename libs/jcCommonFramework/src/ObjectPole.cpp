#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <jcCommonFramework/ObjectPole.h>
#include <jcCommonFramework/MousePole.h>
#include <cmath>

static constexpr float twopi = 2.0f * glm::pi<float> ();
static constexpr float pi = glm::pi<float> ();
static constexpr float piovertwo = 0.5f * glm::pi<float> ();
static constexpr float pioverfour = 0.25f * glm::pi<float> ();

float ObjectPole::sXconversionFactor_ = piovertwo / 4.0f;
float ObjectPole::sYconversionFactor_ = piovertwo / 4.0f;
float ObjectPole::sSpinConversionFactor_ = piovertwo / 4.0f;

static constexpr vec3 sBasis [] = {
	vec3 ( 1.0f, 0.0f, 0.0f ),
	vec3 ( 0.0f, 1.0f, 0.0f ),
	vec3 ( 0.0f, 0.0f, 1.0f )
};

static quat CalculateRotation ( ObjectPole::eAxis axis, float rads )
{
	return glm::angleAxis ( rads, sBasis [ axis ] );
}
//
//ObjectPole::ObjectPole ( const vec3& initialPosition, MousePole* pView, uint32_t actionButton )
//	: pMousePole_ ( pView )
//	, position_ ( initialPosition )
//	, isDragging_ ( false )
//	, actionButton_ ( actionButton )
//	, rotateMode_ ( ObjectPole::eRotateMode::eRotateMode_Dual_Axis )
//	, previousPosition_ ( 0 )
//	, initialPosition_ ( 0 )
//	, initialOrientation_ ( mat3 ( 1.0f ) )
//	, orientation_ ( mat3 ( 1.0f ) )
//	, xConversionFactor_(piovertwo / 4.0f)
//	, yConversionFactor_(piovertwo / 4.0f)
//	, spinConversionFactor_(piovertwo / 4.0f)
//{
//}

ObjectPole::ObjectPole ( const vec3& initialPosition, MousePole* pView, int32_t actionButton )
	: pMousePole_(pView)
	, position_(initialPosition)
	, isDragging_ ( false )
	, actionButton_ ( actionButton )
	, rotateMode_ ( ObjectPole::eRotateMode::eRotateMode_Dual_Axis )
	, previousPosition_(initialPosition)
	, initialPosition_ ( initialPosition )
	, initialOrientation_ ( mat3 ( 1.0f ) )
	, orientation_ ( mat3 ( 1.0f ) )	
{}

ObjectPole::~ObjectPole ()
{
	if ( pMousePole_ )
	{
		pMousePole_.reset ();
	}
}

mat4 ObjectPole::getMatrix () const
{
	mat4 translation = glm::translate ( mat4 ( 1.0f ), position_ );
	return translation * glm::mat4_cast ( orientation_ );
}

void ObjectPole::rotateWorld ( const quat& rot, bool bFromInitial )
{
	if ( !isDragging_ ) bFromInitial = false;
	orientation_ = glm::normalize ( rot * ( bFromInitial ? initialOrientation_ : orientation_ ) );
}

void ObjectPole::rotateLocal ( const quat& rot, bool bFromInitial )
{
	if ( !isDragging_ ) bFromInitial = false;
	orientation_ = glm::normalize ( ( bFromInitial ? initialOrientation_ : orientation_ ) * rot );
}

void ObjectPole::rotateView ( const quat& rot, bool bFromInitial )
{
	if ( !isDragging_ ) bFromInitial = false;
	if ( pMousePole_ )
	{
		quat viewQuat = glm::quat_cast ( pMousePole_->getMatrix () );
		quat invViewQuat = glm::conjugate ( viewQuat );
		orientation_ = glm::normalize ( ( invViewQuat * rot * viewQuat ) * ( bFromInitial ? initialOrientation_ : orientation_ ) );
	}
	else
	{
		rotateWorld ( rot, bFromInitial );
	}
}

void ObjectPole::mouseMove ( float x, float y )
{
	if ( isDragging_ )
	{
		const vec2 pos = vec2 ( x, y );
		
		vec2 diff;
		quat rotation;
		
		switch ( rotateMode_ )
		{
		case eRotateMode::eRotateMode_Dual_Axis:
			diff = pos - previousPosition_;
			rotation = CalculateRotation ( eAxis_Y, diff.x * sXconversionFactor_ );
			rotation = glm::normalize ( CalculateRotation ( eAxis_X, diff.y * sYconversionFactor_ ) * rotation );
			rotateView ( rotation );
			break;
		case eRotateMode::eRotateMode_Biaxial:
			diff = pos - initialPosition_;
			rotation = ( abs ( diff.x ) > abs ( diff.y ) ) ? CalculateRotation ( eAxis_Y, diff.x * sXconversionFactor_ ) : CalculateRotation ( eAxis_X, diff.y * sYconversionFactor_ );			
			rotateView ( rotation );
			break;
		case eRotateMode::eRotateMode_Spin:
			diff = pos - previousPosition_;
			rotateView ( CalculateRotation ( eAxis_Z, -diff.x * sSpinConversionFactor_ ) );
			break;
		default:
			break;
		}

		previousPosition_ = pos;
	}
}

void ObjectPole::mouseButton ( int button, int action, int mods, float x, float y )
{
	if (action == GLFW_PRESS)
	{
		// ignore button presses when dragging
		if ( button == actionButton_ )
		{
			previousPosition_.x = x;
			previousPosition_.y = y;
			initialPosition_.x = x;
			initialPosition_.y = y;
			initialOrientation_ = orientation_;
			isDragging_ = true;
		}
	}
	else
	{
		// ignore releases if not dragging
		if ( isDragging_ )
		{
			if ( button == actionButton_ )
			{
				mouseMove ( x, y );
				isDragging_ = false;
			}
		}
	}
}
