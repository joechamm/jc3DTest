#define GLFW_INCLUDE_NONE
#include <glfw/glfw3.h>
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

float MousePole::sXconversionFactor_ = piovertwo / 250.0f;
float MousePole::sYconversionFactor_ = piovertwo / 250.0f;
float MousePole::sSpinConversionFactor_ = piovertwo / 250.0f;

MousePole::MousePole ( const vec3& target, const RadiusDef& radiusDef, uint32_t actionButton )
	: lookAt_ ( target )
	, radCurrXZAngle_ ( 0.0f )
	, radCurrYAngle_ ( -pioverfour )
	, radCurrSpin_ ( 0.0f )
	, radInitSpin_(0.0f)
	, radInitXZAngle_(0.0f)
	, radInitYAngle_(0.0f)
	, fRadius_ ( 20.0f )
	, radiusDef_ ( radiusDef )
	, actionButton_(actionButton)
	, isDragging_ ( false )
	, buttonMods_(0)
	, rotateMode_(eRotateMode::eRotateMode_Dual_Axis)
	, initialPoint_(0)
{
}

MousePole::~MousePole() {}

void MousePole::getCurrentState ( vec3& pos, vec3& lookAt, vec3& upVec )
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
	vec3 up = glm::cross ( currentPosition, upRotationAxis );

	transform = glm::rotate ( mat4 ( 1.0f ), radCurrSpin_, currentPosition );

	up = vec3 ( transform * vec4 ( up, 0.0f ) );

	pos = cameraPosition;
	lookAt = lookAt_;
	upVec = glm::normalize ( up );	
}

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

void MousePole::beginDragRotate ( const vec2& ptStart, eRotateMode rotateMode )
{
	rotateMode_ = rotateMode;

	initialPoint_ = ptStart;

	radInitXZAngle_ = radCurrXZAngle_;
	radInitYAngle_ = radCurrYAngle_;
	radInitSpin_ = radCurrSpin_;

	isDragging_ = true;
}

void MousePole::onDragRotate ( const vec2& ptCurr )
{
	vec2 diff = ptCurr - initialPoint_;

	switch ( rotateMode_ )
	{
	case MousePole::eRotateMode::eRotateMode_Dual_Axis:
		processXChange ( diff.x );
		processYChange ( diff.y );
		break;
	case MousePole::eRotateMode::eRotateMode_Bi_Axial:
		if ( abs ( diff.x ) > abs ( diff.y ) )
			processXChange ( diff.x, true );
		else
			processYChange ( diff.y, true );
		break;
	case MousePole::eRotateMode::eRotateMode_XZ_Axis:
		processXChange ( diff.x );
		break;
	case MousePole::eRotateMode::eRotateMode_Y_Axis:
		processYChange ( diff.y );
		break;
	case MousePole::eRotateMode::eRotateMode_Spin_View:
		processSpinAxis ( diff.x, diff.y );
		break;
	default:
		break;
	}
}

void MousePole::processXChange ( float xDiff, bool bClearY )
{
	radCurrXZAngle_ = xDiff * sXconversionFactor_ + radInitXZAngle_;
	if ( bClearY )
		radCurrYAngle_ = radInitYAngle_;
}

void MousePole::processYChange ( float yDiff, bool bClearXZ )
{
	radCurrYAngle_ = radInitYAngle_ - yDiff * sYconversionFactor_;
	if ( bClearXZ )
		radCurrXZAngle_ = radInitXZAngle_;
}

void MousePole::processSpinAxis ( float xDiff, float yDiff )
{
	radCurrSpin_ = xDiff * sSpinConversionFactor_ + radInitSpin_;
}

void MousePole::endDragRotate ( const vec2& endPoint, bool bKeepResults )
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

void MousePole::moveForward ( bool bLargeStep )
{
	radiusDef_.fCurrRadius_ -= bLargeStep ? radiusDef_.fLargeDelta_ : radiusDef_.fSmallDelta_;
	radiusDef_.fCurrRadius_ = glm::max ( radiusDef_.fCurrRadius_, radiusDef_.fMinRadius_ );
}

void MousePole::moveBackward ( bool bLargeStep )
{
	radiusDef_.fCurrRadius_ += bLargeStep ? radiusDef_.fLargeDelta_ : radiusDef_.fSmallDelta_;
	radiusDef_.fCurrRadius_ = glm::min ( radiusDef_.fCurrRadius_, radiusDef_.fMaxRadius_ );
}

void MousePole::moveRight ( bool bLargeStep )
{
	offsetTargetPos ( eTargetOffsetDirection_Right, bLargeStep ? radiusDef_.fLargeDelta_ : radiusDef_.fSmallDelta_ );
}

void MousePole::moveLeft ( bool bLargeStep )
{
	offsetTargetPos ( eTargetOffsetDirection_Left, bLargeStep ? radiusDef_.fLargeDelta_ : radiusDef_.fSmallDelta_ );
}

void MousePole::moveUp ( bool bLargeStep )
{
	offsetTargetPos ( eTargetOffsetDirection_Up, bLargeStep ? radiusDef_.fLargeDelta_ : radiusDef_.fSmallDelta_ );
}

void MousePole::moveDown ( bool bLargeStep )
{
	offsetTargetPos ( eTargetOffsetDirection_Down, bLargeStep ? radiusDef_.fLargeDelta_ : radiusDef_.fSmallDelta_ );
}

void MousePole::mouseMove ( float x, float y )
{
	if ( isDragging_ )
		onDragRotate (vec2(x, y));
}

void MousePole::mouseButton ( int button, int action, int mods, float x, float y )
{
	buttonMods_ = mods;
	if ( button == actionButton_ )
	{
		if ( action == GLFW_PRESS )
		{
			beginDragRotate ( vec2 (  x,  y  ), ( mods & GLFW_MOD_SHIFT ) ? eRotateMode::eRotateMode_Spin_View : eRotateMode::eRotateMode_Dual_Axis );
		}
		else if ( action == GLFW_RELEASE )
		{
			endDragRotate ( vec2 (  x , y  ), (mods & GLFW_MOD_ALT) ? false : true );
		}
	}
}

void MousePole::mouseWheel ( float xoffset, float yoffset )
{
	offsetTargetPos ( MousePole::eTargetOffsetDirection_Forward, yoffset );
}

void MousePole::key ( int key, int scancode, int action, int mods )
{
	switch ( key )
	{
	case GLFW_KEY_W:
		if ( action == GLFW_PRESS )
			moveForward ( ( mods & GLFW_MOD_SHIFT ) != 0 );
		break;
	case GLFW_KEY_S:
		if ( action == GLFW_PRESS )
			moveBackward ( ( mods & GLFW_MOD_SHIFT ) != 0 );
		break;
	case GLFW_KEY_A:
		if ( action == GLFW_PRESS )
			moveLeft ( ( mods & GLFW_MOD_SHIFT ) != 0 );
		break;
	case GLFW_KEY_D:
		if ( action == GLFW_PRESS )
			moveRight ( ( mods & GLFW_MOD_SHIFT ) != 0 );
		break;
	case GLFW_KEY_Q:
		if ( action == GLFW_PRESS )
			moveUp ( ( mods & GLFW_MOD_SHIFT ) != 0 );
		break;
	case GLFW_KEY_E:
		if ( action == GLFW_PRESS )
			moveDown ( ( mods & GLFW_MOD_SHIFT ) != 0 );
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