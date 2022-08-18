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

	//enum class eActionButton : uint32_t
	//{
	//	eActionButton_Left = 1,		
	//	eActionButton_Right = 2,
	//	eActionButton_Middle = 3
	//};

	enum class eButtonState : uint8_t
	{
		eButtonState_Up = 0x01,
		eButtonState_Down = 0x02
	};

	enum eTargetOffsetDirection : uint32_t
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
		eRotateMode_Dual_Axis = 0x01,
		eRotateMode_Bi_Axial = 0x02,
		eRotateMode_XZ_Axis = 0x04,
		eRotateMode_Y_Axis = 0x08,
		eRotateMode_Spin_View = 0x10
	};

	vec3				lookAt_;

	float				radCurrXZAngle_;  // angle around the y-axis in radians
	float				radCurrYAngle_;   // angle above the xz plane in radians
	float				radCurrSpin_;		// angle spin around the look-at direction. Changes up-vector
	float				fRadius_;		
	
	RadiusDef			radiusDef_;

	eRotateMode			rotateMode_;

//	eActionButton		actionButton_;
	uint32_t			actionButton_;
	bool				isDragging_;

	ivec2				initialPoint_;

	float				radInitXZAngle_;
	float				radInitYAngle_;
	float				radInitSpin_;

	float				xConversionFactor_;
	float				yConversionFactor_;
	float				spinConversionFactor_;
public:
//	MousePole ( const vec3& target, const RadiusDef& radiusDef, eActionButton eButton = eActionButton::eActionButton_Left );
	MousePole ( const vec3& target, const RadiusDef& radiusDef, uint32_t actionButton = 0 );
	~MousePole ();

	void	getCurrentState ( vec3& pos, vec3& lookAt, vec3& upVec );
	mat4	getMatrix () const;

	vec3	getLookAtPos () const { return lookAt_; }
	float	getLookAtDistance () const { return radiusDef_.fCurrRadius_; }

	void	mouseMove ( const ivec2& pos );
	void	mouseButton ( int button, eButtonState btnState, const ivec2& pos );
	void	mouseWheel ( int direction, const ivec2& position );
	void	keyOffset ( char key, float largeIncrement, float smallIncrement );

	bool	isDragging () const { return isDragging_;  }

	inline float	xConversionFactor () const { return xConversionFactor_; }
	inline float	yConversionFactor () const { return yConversionFactor_; }
	inline float	spinConversionFactor () const { return spinConversionFactor_; }

	inline void setXConversionFactor ( float convFactor ) { xConversionFactor_ = convFactor; }
	inline void setYConversionFactor ( float convFactor ) { yConversionFactor_ = convFactor; }
	inline void setSpinConversionFactor ( float convFactor ) { spinConversionFactor_ = convFactor; }

	void	offsetTargetPos ( eTargetOffsetDirection eDir, float worldDistance );
	void	offsetTargetPos ( const vec3& cameraOffset );
protected:
	void				processXChange ( int iXDiff, bool bClearY = false );
	void				processYChange ( int iYDiff, bool bClearXZ = false );
	void				processSpinAxis ( int iXDiff, int iYDiff );
	void				beginDragRotate ( const ivec2& ptStart, eRotateMode rotMode = eRotateMode::eRotateMode_Dual_Axis );
	void				onDragRotate ( const ivec2& ptCurr );
	void				endDragRotate ( const ivec2& ptEnd, bool bKeepResults = true );
	bool				isDragging () { return isDragging_; }

	void				moveCloser ( bool bLargeStep = true );
	void				moveAway ( bool bLargeStep = true );
};

#endif // !__MOUSE_POLE_H__
