#ifndef __MOUSE_POLE_H__
#define __MOUSE_POLE_H__

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::mat4;
using glm::mat3;
using glm::mat2;
using glm::vec4;
using glm::vec3;
using glm::vec2;
using glm::ivec2;
using glm::quat;
using glm::dvec2;

class MousePole
{
public:
	struct RadiusDef
	{
		float fCurrRadius_;
		float fMinRadius_;
		float fMaxRadius_;
		float fLargeDelta_;
		float fSmallDelta_;
	};

	enum eTargetOffsetDirection : uint8_t
	{
		eTargetOffsetDirection_Up = 0,
		eTargetOffsetDirection_Down = 1,
		eTargetOffsetDirection_Backwards = 2,
		eTargetOffsetDirection_Forward = 3,
		eTargetOffsetDirection_Right = 4,
		eTargetOffsetDirection_Left = 5,
		eTargetOffsetDirection_Count = 6
	};
protected:
	enum class eRotateMode : uint8_t
	{
		eRotateMode_None = 0,
		eRotateMode_Dual_Axis = 1,
		eRotateMode_Bi_Axial = 2,
		eRotateMode_XZ_Axis = 3,
		eRotateMode_Y_Axis = 4,
		eRotateMode_Spin_View = 5,
		eRotateMode_Count = 6
	};

	vec3				lookAt_;

	float				radCurrXZAngle_;  // angle around the y-axis in radians
	float				radCurrYAngle_;   // angle above the xz plane in radians
	float				radCurrSpin_;		// angle spin around the look-at direction. Changes up-vector
	float				fRadius_;		
	
	RadiusDef			radiusDef_;

	eRotateMode			rotateMode_;

	int32_t				actionButton_;
	int32_t				buttonMods_;

	bool				isDragging_;

	vec2				initialPoint_;

	float				radInitXZAngle_;
	float				radInitYAngle_;
	float				radInitSpin_;

	static float		sXconversionFactor_;
	static float		sYconversionFactor_;
	static float		sSpinConversionFactor_;
	
public:
	MousePole ( const vec3& target, const RadiusDef& radiusDef, uint32_t actionButton = 0 );
	~MousePole ();

	void	getCurrentState ( vec3& pos, vec3& lookAt, vec3& upVec );
	mat4	getMatrix () const;

	vec3	getLookAtPos () const { return lookAt_; }
	float	getLookAtDistance () const { return radiusDef_.fCurrRadius_; }

	void	mouseMove ( float x, float y );
	void	mouseButton ( int button, int action, int mods, float x, float y );
	void	mouseWheel ( float xoffset, float yoffset );
	void	key ( int key, int scancode, int action, int mods );

	bool	isDragging () const { return isDragging_;  }

	void	offsetTargetPos ( eTargetOffsetDirection eDir, float worldDistance );
	void	offsetTargetPos ( const vec3& cameraOffset );

	static float getXConversionFactor () { return sXconversionFactor_; }
	static float getYConversionFactor () { return sYconversionFactor_; }
	static float getSpinConversionFactor () { return sSpinConversionFactor_; }
	static void setXConversionFactor ( float convFactor ) { sXconversionFactor_ = convFactor; }
	static void setYConversionFactor ( float convFactor ) { sYconversionFactor_ = convFactor; }
	static void setSpinConversionFactor ( float convFactor ) { sSpinConversionFactor_ = convFactor; }
	
protected:
	void				processXChange ( float xDiff, bool bClearY = false );
	void				processYChange ( float yDiff, bool bClearXZ = false );
	void				processSpinAxis ( float xDiff, float yDiff );
	void				beginDragRotate ( const vec2& ptStart, eRotateMode rotMode = eRotateMode::eRotateMode_Dual_Axis );
	void				onDragRotate ( const vec2& ptCurr );
	void				endDragRotate ( const vec2& ptEnd, bool bKeepResults = true );

	void  				moveForward ( bool bLargeStep = true );	
	void  				moveBackward ( bool bLargeStep = true );
	void  				moveRight ( bool bLargeStep = true );
	void				moveLeft ( bool bLargeStep = true );
	void				moveUp ( bool bLargeStep = true );
	void				moveDown ( bool bLargeStep = true );
};

#endif // !__MOUSE_POLE_H__
