#include <jcCommonFramework/MousePole.h>
#include <cmath>

static constexpr vec3 sUnitZ = vec3 ( 0.0f, 0.0f, 1.0f );
static constexpr vec3 sOffsets[] = {
	vec3 ( 0.0f,  1.0f,  0.0f ),
	vec3 ( 0.0f, -1.0f,  0.0f ),
	vec3 ( 0.0f,  0.0f, -1.0f ),
	vec3 ( 0.0f,  0.0f,  1.0f ),
	vec3 ( 1.0f,  0.0f,  0.0f ),
	vec3 (-1.0f,  0.0f,  0.0f )
};

static constexpr float twopi = 2.0f * glm::pi<float> ();
static constexpr float pi = glm::pi<float> ();
static constexpr float piovertwo = 0.5f * glm::pi<float> ();
static constexpr float pioverfour = 0.25f * glm::pi<float> ();

MousePole::MousePole ( const vec3& target, const RadiusDef& radiusDef, uint32_t actionButton )
	: lookAt_ ( target )
	, radCurrXZAngle_ ( 0.0f )
	, radCurrYAngle_ ( -pioverfour )
	, radCurrSpin_ ( 0.0f )
	, radInitSpin_(0.0f)
	, radInitXZAngle_(0.0f)
	, radInitYAngle_(0.0f)
	, xConversionFactor_(piovertwo / 250.0f)
	, yConversionFactor_(piovertwo / 250.0f)
	, spinConversionFactor_(piovertwo / 250.0f)
	, fRadius_ ( 20.0f )
	, radiusDef_ ( radiusDef )
	, actionButton_(actionButton)
	, isDragging_ ( false )
	, rotateMode_(eRotateMode::eRotateMode_Dual_Axis)
	, initialPoint_(0)
{
}

MousePole::~MousePole() {}

mat4 MousePole::getMatrix () const
{
	// Compute the position vector along the xz plane
	float cosxz = cosf ( radCurrXZAngle_ );
	float sinxz = sinf ( radCurrXZAngle_ );

	vec3 currentPosition ( -sinxz, 0.0f, cosxz );

	// Compute the "up" rotation axis
	// This axis is a 90 degree rotation around the y axis. Just a component-swap and negate.
	vec3 upRotationAxis ( currentPosition.z, currentPosition.y, -currentPosition.x );

	mat4 transform = glm::rotate ( mat4 ( 1.0f ), radCurrYAngle_, upRotationAxis );
	currentPosition = vec3 ( transform * vec4 ( currentPosition, 0.0f ) );

	// Set the position of the camera
	vec3 tempVec = currentPosition * radiusDef_.fCurrRadius_;
	vec3 cameraPosition = tempVec + lookAt_;

	// Now compute the up-vector as the cross product of currentPosition and upRotationAxis
	// Rotate this vector around the currentPosition axis given currSpin_
	vec3 upVec = glm::cross ( currentPosition, upRotationAxis );

	transform = glm::rotate ( mat4 ( 1.0f ), radCurrSpin_, currentPosition );

	upVec = vec3 ( transform * vec4 ( upVec, 0.0f ) );

	return glm::lookAt ( cameraPosition, lookAt_, upVec );
}

void MousePole::beginDragRotate ( const ivec2& ptStart, eRotateMode rotateMode )
{
	rotateMode_ = rotateMode;

	initialPoint_ = ptStart;

	radInitXZAngle_ = radCurrXZAngle_;
	radInitYAngle_ = radCurrYAngle_;
	radInitSpin_ = radCurrSpin_;

	isDragging_ = true;
}

void MousePole::onDragRotate ( const ivec2& ptCurr )
{
	ivec2 iDiff = ptCurr - initialPoint_;

	switch ( rotateMode_ )
	{
	case MousePole::eRotateMode::eRotateMode_Dual_Axis:
		processXChange ( iDiff.x );
		processYChange ( iDiff.y );
		break;
	case MousePole::eRotateMode::eRotateMode_Bi_Axial:
		if ( abs ( iDiff.x ) > abs ( iDiff.y ) )
			processXChange ( iDiff.x, true );
		else
			processYChange ( iDiff.y, true );
		break;
	case MousePole::eRotateMode::eRotateMode_XZ_Axis:
		processXChange ( iDiff.x );
		break;
	case MousePole::eRotateMode::eRotateMode_Y_Axis:
		processYChange ( iDiff.y );
		break;
	case MousePole::eRotateMode::eRotateMode_Spin_View:
		processSpinAxis ( iDiff.x, iDiff.y );
		break;
	default:
		break;
	}
}

void MousePole::processXChange ( int iXDiff, bool bClearY )
{
	radCurrXZAngle_ = static_cast< float >( iXDiff ) * xConversionFactor_ + radInitXZAngle_;
	if ( bClearY )
		radCurrYAngle_ = radInitYAngle_;
}

void MousePole::processYChange ( int iYDiff, bool bClearXZ )
{
	radCurrYAngle_ = radInitYAngle_ - static_cast< float >( iYDiff ) * yConversionFactor_;
	if ( bClearXZ )
		radCurrXZAngle_ = radInitXZAngle_;
}

void MousePole::processSpinAxis ( int iXDiff, int iYDiff )
{
	radCurrSpin_ = static_cast< float >( iXDiff ) * spinConversionFactor_ + radInitSpin_;
}

void MousePole::endDragRotate ( const ivec2& endPoint, bool bKeepResults )
{
	if ( bKeepResults )
	{
		onDragRotate ( endPoint );
	}
	else
	{
		radCurrXZAngle_ = radInitXZAngle_;
		radCurrYAngle_ = radInitYAngle_;
	}

	isDragging_ = false;
}

void MousePole::moveCloser ( bool bLargeStep )
{
	radiusDef_.fCurrRadius_ -= bLargeStep ? radiusDef_.fLargeDelta_ : radiusDef_.fSmallDelta_;
	radiusDef_.fCurrRadius_ = glm::max ( radiusDef_.fCurrRadius_, radiusDef_.fMinRadius_ );
}

void MousePole::moveAway ( bool bLargeStep )
{
	radiusDef_.fCurrRadius_ += bLargeStep ? radiusDef_.fLargeDelta_ : radiusDef_.fSmallDelta_;
	radiusDef_.fCurrRadius_ = glm::min ( radiusDef_.fCurrRadius_, radiusDef_.fMaxRadius_ );
}

void MousePole::mouseMove ( const ivec2& position )
{
	if ( isDragging_ )
		onDragRotate ( position );
}

void MousePole::mouseButton ( int button, eButtonState btnState, const ivec2& pos )
{
	if ( btnState == eButtonState::eButtonState_Down )
	{
		// Ignore all other button presses when dragging
		if ( !isDragging_ )
		{
			if ( button == actionButton_ )
			{
				beginDragRotate ( pos, rotateMode_ );
			}
		}
	}
	else
	{
		if ( isDragging_ )
		{
			if ( button == actionButton_ )
			{
				endDragRotate ( pos );
			}
		}
	}
}

void MousePole::mouseWheel ( int direction, const ivec2& position )
{
	if ( direction > 0 )
	{
		moveCloser ( false );
	}
	else
	{
		moveAway ( false );
	}
}

void MousePole::keyOffset ( char key, float largeIncrement, float smallIncrement )
{
	switch ( key )
	{
	case 'w':
		offsetTargetPos ( MousePole::eTargetOffsetDirection::eTargetOffsetDirection_Forward, largeIncrement );
		break;
	case 's':
		offsetTargetPos ( MousePole::eTargetOffsetDirection::eTargetOffsetDirection_Backwards, largeIncrement );
		break;
	case 'd':
		offsetTargetPos ( MousePole::eTargetOffsetDirection::eTargetOffsetDirection_Right, largeIncrement );
		break;
	case 'a':
		offsetTargetPos ( MousePole::eTargetOffsetDirection::eTargetOffsetDirection_Left, largeIncrement );
		break;
	case 'e':
		offsetTargetPos ( MousePole::eTargetOffsetDirection::eTargetOffsetDirection_Up, largeIncrement );
		break;
	case 'q':
		offsetTargetPos ( MousePole::eTargetOffsetDirection::eTargetOffsetDirection_Down, largeIncrement );
		break;
	case 'W':
		offsetTargetPos ( MousePole::eTargetOffsetDirection::eTargetOffsetDirection_Forward, smallIncrement );
		break;
	case 'S':
		offsetTargetPos ( MousePole::eTargetOffsetDirection::eTargetOffsetDirection_Backwards, smallIncrement );
		break;
	case 'D':
		offsetTargetPos ( MousePole::eTargetOffsetDirection::eTargetOffsetDirection_Right, smallIncrement );
		break;
	case 'A':
		offsetTargetPos ( MousePole::eTargetOffsetDirection::eTargetOffsetDirection_Left, smallIncrement );
		break;
	case 'E':
		offsetTargetPos ( MousePole::eTargetOffsetDirection::eTargetOffsetDirection_Up, smallIncrement );
		break;
	case 'Q':
		offsetTargetPos ( MousePole::eTargetOffsetDirection::eTargetOffsetDirection_Down, smallIncrement );
		break;
	default:
		break;
	}
}

void MousePole::offsetTargetPos ( eTargetOffsetDirection dir, float worldDistance )
{
	offsetTargetPos ( sOffsets [ dir ] * worldDistance );
}

void MousePole::offsetTargetPos ( const vec3& cameraOffset )
{
	mat4 currentTransform = getMatrix ();
	quat orientation = glm::quat_cast ( currentTransform );

	quat inverseOrientation = glm::conjugate ( orientation );
	vec3 worldOffset = inverseOrientation * cameraOffset;

	lookAt_ += worldOffset;
}