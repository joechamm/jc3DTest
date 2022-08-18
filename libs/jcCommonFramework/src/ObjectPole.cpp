#include <jcCommonFramework/ObjectPole.h>
#include <jcCommonFramework/MousePole.h>
#include <cmath>

static constexpr float twopi = 2.0f * glm::pi<float> ();
static constexpr float pi = glm::pi<float> ();
static constexpr float piovertwo = 0.5f * glm::pi<float> ();
static constexpr float pioverfour = 0.25f * glm::pi<float> ();

static constexpr vec3 sBasis [] = {
	vec3 ( 1.0f, 0.0f, 0.0f ),
	vec3 ( 0.0f, 1.0f, 0.0f ),
	vec3 ( 0.0f, 0.0f, 1.0f )
};

static quat CalculateRotation ( ObjectPole::eAxis axis, float rads )
{
	return glm::angleAxis ( rads, sBasis [ axis ] );
}

ObjectPole::ObjectPole ( const vec3& initialPosition, MousePole* pView, uint32_t actionButton )
	: pMousePole_ ( pView )
	, position_ ( initialPosition )
	, isDragging_ ( false )
	, actionButton_ ( actionButton )
	, rotateMode_ ( ObjectPole::eRotateMode::eRotateMode_Dual_Axis )
	, previousPosition_ ( 0 )
	, initialPosition_ ( 0 )
	, initialOrientation_ ( mat3 ( 1.0f ) )
	, orientation_ ( mat3 ( 1.0f ) )
	, xConversionFactor_(piovertwo / 4.0f)
	, yConversionFactor_(piovertwo / 4.0f)
	, spinConversionFactor_(piovertwo / 4.0f)
{
}

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

void ObjectPole::mouseMove ( const ivec2& position )
{
	if ( isDragging_ )
	{
		ivec2 iDiff = position - previousPosition_;

		switch ( rotateMode_ )
		{
		case ObjectPole::eRotateMode::eRotateMode_Dual_Axis:
			quat rotation = CalculateRotation ( eAxis_Y, iDiff.x * xConversionFactor_ );
			rotation = glm::normalize ( CalculateRotation ( eAxis_X, iDiff.y * yConversionFactor_ ) * rotation );
			rotateView ( rotation );
			break;
		case ObjectPole::eRotateMode::eRotateMode_Biaxial:
			ivec2 initialDifference = position - initialPosition_;
			bool xDiffGTyDiff = abs ( initialDifference.x ) > abs ( initialDifference.y );
			quat rotation = xDiffGTyDiff ? CalculateRotation ( eAxis_Y, initialDifference.x * xConversionFactor_ ) : CalculateRotation ( eAxis_X, initialDifference.y * yConversionFactor_ );
			rotateView ( rotation );
			break;
		case ObjectPole::eRotateMode::eRotateMode_Spin:
			rotateView ( CalculateRotation ( eAxis_Z, -iDiff.x * spinConversionFactor_ ) );
			break;
		default:
			break;
		}

		previousPosition_ = position;
	}
}
void ObjectPole::mouseButton ( int button, eButtonState btnState, const ivec2& position )
{
	if ( btnState == eButtonState::eButtonState_Down )
	{
		// Ignore button presses when dragging
		if ( button == actionButton_ )
		{
			previousPosition_ = position;
			initialPosition_ = position;
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
				mouseMove ( position );
				isDragging_ = false;
			}
		}
	}
}
void ObjectPole::mouseWheel ( int direction, const ivec2& position )
{

}