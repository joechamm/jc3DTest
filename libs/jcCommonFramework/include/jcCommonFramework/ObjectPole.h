#ifndef __OBJECT_POLE_H__
#define __OBJECT_POLE_H__

#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using glm::mat4;
using glm::mat3;
using glm::mat2;
using glm::vec4;
using glm::vec3;
using glm::vec2;
using glm::ivec2;
using glm::quat;

class MousePole;

class ObjectPole
{
public:
	/*enum class eActionButtons : uint8_t
	{
		eActionButton_Left = 0x01,
		eActionButton_Middle = 0x02,
		eActionButton_Right = 0x04
	};*/

	enum class eButtonState : uint8_t
	{
		eButtonState_Up = 0x01,
		eButtonState_Down = 0x02
	};

	enum eAxis : uint8_t
	{
		eAxis_X = 0,
		eAxis_Y = 1,
		eAxis_Z = 2,
		eAxis_Count = 3
	};

	enum class eRotateMode : uint8_t
	{
		eRotateMode_Dual_Axis = 0x01,
		eRotateMode_Biaxial = 0x02,
		eRotateMode_Spin = 0x04
	};
protected:
	std::shared_ptr<MousePole> pMousePole_;
	
	quat orientation_;
	vec3 position_;

//	eActionButtons actionButton_;
	uint32_t actionButton_;

	eRotateMode rotateMode_;

	bool isDragging_;

	ivec2 previousPosition_;
	ivec2 initialPosition_;
	quat initialOrientation_;

	float xConversionFactor_;
	float yConversionFactor_;
	float spinConversionFactor_;

public:
//	ObjectPole ( const vec3& initialPosition, const MousePole* pView = nullptr, eActionButtons actionButton = eActionButtons::eActionButton_Right );
	ObjectPole ( const vec3& initialPosition, MousePole* pView = nullptr, uint32_t actionButton = 1 /* Right Mouse Button */ );
	~ObjectPole ();

	mat4 getMatrix () const;

	void rotateWorld ( const quat& rot, bool bFromInitial = false );
	void rotateLocal ( const quat& rot, bool bFromInitial = false );
	void rotateView ( const quat& rot, bool bFromInitial = false );

	void mouseMove ( const ivec2& position );
	void mouseButton ( int button, eButtonState btnState, const ivec2& position );
	void mouseWheel ( int direction, const ivec2& position );

	bool isDragging () const { return isDragging;  }

	inline float	xConversionFactor () const { return xConversionFactor_; }
	inline float	yConversionFactor () const { return yConversionFactor_; }
	inline float	spinConversionFactor () const { return spinConversionFactor_; }

	inline void setXConversionFactor ( float convFactor ) { xConversionFactor_ = convFactor; }
	inline void setYConversionFactor ( float convFactor ) { yConversionFactor_ = convFactor; }
	inline void setSpinConversionFactor ( float convFactor ) { spinConversionFactor_ = convFactor; }
};

#endif // !__OBJECT_POLE_H__
