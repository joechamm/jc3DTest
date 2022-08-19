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
	enum eAxis : uint8_t
	{
		eAxis_X = 0,
		eAxis_Y = 1,
		eAxis_Z = 2,
		eAxis_Count = 3
	};

	enum class eRotateMode : uint8_t
	{
		eRotateMode_None = 0,
		eRotateMode_Dual_Axis = 1,
		eRotateMode_Biaxial = 2,
		eRotateMode_Spin = 3,
		eRotateMode_Count = 4
	};
protected:
	std::shared_ptr<MousePole> pMousePole_;
	
	quat orientation_;
	vec3 position_;

	int32_t actionButton_;

	eRotateMode rotateMode_;

	bool isDragging_;

	vec2 previousPosition_;
	vec2 initialPosition_;
	quat initialOrientation_;

	static float sXconversionFactor_;
	static float sYconversionFactor_;
	static float sSpinConversionFactor_;

public:
	ObjectPole ( const vec3& initialPosition, MousePole* pView = nullptr, int32_t actionButton = 1 /* Right Mouse Button */ );
	~ObjectPole ();

	mat4 getMatrix () const;

	void rotateWorld ( const quat& rot, bool bFromInitial = false );
	void rotateLocal ( const quat& rot, bool bFromInitial = false );
	void rotateView ( const quat& rot, bool bFromInitial = false );

	void mouseMove ( float x, float y );
	void mouseButton ( int button, int action, int mods, float x, float y );

	inline bool isDragging () const { return isDragging_;  }

	static float	getXConversionFactor () { return sXconversionFactor_; }
	static float	getYConversionFactor ()  { return sYconversionFactor_; }
	static float	getSpinConversionFactor () { return sSpinConversionFactor_; }

	static void setXConversionFactor ( float convFactor ) { sXconversionFactor_ = convFactor; }
	static void setYConversionFactor ( float convFactor ) { sYconversionFactor_ = convFactor; }
	static void setSpinConversionFactor ( float convFactor ) { sSpinConversionFactor_ = convFactor; }
};

#endif // !__OBJECT_POLE_H__
