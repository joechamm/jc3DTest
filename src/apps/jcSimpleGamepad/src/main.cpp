#include <jcGLframework/GLFWApp.h>
#include <jcGLframework/GLShader.h>
#include <jcGLframework/GLOcclusionQuery.h>
#include <jcGLframework/GLPrimitivesQuery.h>
#include <jcGLframework/GLTimerQuery.h>

#include <jcCommonFramework/ResourceString.h>
//#include <jcCommonFramework/Camera.h>
#include <jcCommonFramework/MyCamera.h>
#include <jcCommonFramework/Utils.h>
#include <jcCommonFramework/UtilsFPS.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <map>

using glm::mat4;
using glm::mat3;
using glm::mat2;
using glm::vec4;
using glm::vec3;
using glm::vec2;

constexpr GLuint kIdxBind_TriBuffer = 0;
constexpr GLuint kIdxBind_PerFrameData = 7;
constexpr GLuint kIdxBind_TriangleOffsets = 8;
constexpr GLint kIdxAttr_inPosition = 0;
constexpr GLint kIdxAttr_inColor = 1;

struct PerFrameData
{
	mat4 proj;
	mat4 view;
	mat4 model;
};

struct TriangleOffsets
{
	vec2 offsets [ 16 ];
};

struct MouseState
{
	vec2 pos = vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

struct Vertex
{
	float pos [ 4 ];
	float color [ 4 ];
};

//CameraPositioner_FirstPerson positioner ( vec3 ( 0.f, 0.f, -2.f ), vec3 ( 0.f, 0.f, 0.f ), vec3 ( 0.f, 1.f, 0.f ) );
//Camera camera ( positioner );

MyCameraPositioner_FirstPerson positioner ( vec3 ( 0.f, 0.f, 2.f ), vec3 ( 0.f ), vec3 ( 0.f, 1.f, 0.f ) );
MyCamera camera( positioner );

FramesPerSecondCounter fpsCounter;

float g_theta = 0.0f;
float g_deltaTheta = 0.01f;

GLuint g_vao = 0;
GLuint g_vbo = 0;
GLuint g_ebo = 0;
GLuint g_per_frame_ubo = 0;
GLuint g_offsets_ubo = 0;

GLuint g_prog_tri = 0;

constexpr float kMaxPitchDegrees = 30.0f;
constexpr float kMaxYawDegrees = 60.0f;
constexpr float kMaxRollDegrees = 90.0f;
constexpr float kMaxStrafe = 2.0f;

float g_pitchDegrees = 0.0f;
float g_yawDegrees = 0.0f;
float g_rollDegrees = 0.0f;
float g_strafe = 0.0f;

int g_joysticksPresent [ 16 ];

std::string getGlfwGamepadButtonName ( int btnId )
{
	auto const btn_str = [btnId] () {
		switch ( btnId )
		{
		case GLFW_GAMEPAD_BUTTON_A:
			return "GLFW_GAMEPAD_BUTTON_A";
		case GLFW_GAMEPAD_BUTTON_B:
			return "GLFW_GAMEPAD_BUTTON_B";
		case GLFW_GAMEPAD_BUTTON_X:
			return "GLFW_GAMEPAD_BUTTON_X";
		case GLFW_GAMEPAD_BUTTON_Y:
			return "GLFW_GAMEPAD_BUTTON_Y";
		case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER:
			return "GLFW_GAMEPAD_BUTTON_LEFT_BUMPER";
		case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER:
			return "GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER";
		case GLFW_GAMEPAD_BUTTON_BACK:
			return "GLFW_GAMEPAD_BUTTON_BACK";
		case GLFW_GAMEPAD_BUTTON_START:
			return "GLFW_GAMEPAD_BUTTON_START";
		case GLFW_GAMEPAD_BUTTON_GUIDE:
			return "GLFW_GAMEPAD_BUTTON_GUIDE";
		case GLFW_GAMEPAD_BUTTON_LEFT_THUMB:
			return "GLFW_GAMEPAD_BUTTON_LEFT_THUMB";
		case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB:
			return "GLFW_GAMEPAD_BUTTON_RIGHT_THUMB";
		case GLFW_GAMEPAD_BUTTON_DPAD_UP:
			return "GLFW_GAMEPAD_BUTTON_DPAD_UP";
		case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT:
			return "GLFW_GAMEPAD_BUTTON_DPAD_RIGHT";
		case GLFW_GAMEPAD_BUTTON_DPAD_DOWN:
			return "GLFW_GAMEPAD_BUTTON_DPAD_DOWN";
		case GLFW_GAMEPAD_BUTTON_DPAD_LEFT:
			return "GLFW_GAMEPAD_BUTTON_DPAD_LEFT";			
		default:
			return "UNKNOWN";
			break;
		}
	}( );

	return btn_str;
}

std::string getGlfwGamepadAxisName ( int axisId )
{
	auto const axis_str = [axisId] () {
		switch ( axisId )
		{
		case GLFW_GAMEPAD_AXIS_LEFT_X:
			return "GLFW_GAMEPAD_AXIS_LEFT_X";
		case GLFW_GAMEPAD_AXIS_LEFT_Y:
			return "GLFW_GAMEPAD_AXIS_LEFT_Y";
		case GLFW_GAMEPAD_AXIS_RIGHT_X:
			return "GLFW_GAMEPAD_AXIS_RIGHT_X";
		case GLFW_GAMEPAD_AXIS_RIGHT_Y:
			return "GLFW_GAMEPAD_AXIS_RIGHT_Y";
		case GLFW_GAMEPAD_AXIS_LEFT_TRIGGER:
			return "GLFW_GAMEPAD_AXIS_LEFT_TRIGGER";
		case GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER:
			return "GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER";
		default:
			return "UNKNOWN";
			break;
		}
	}( );

	return axis_str;
}

std::string getGlfwJoystickName ( int joyId )
{
	auto const joy_str = [joyId] () {
		switch ( joyId )
		{
		case GLFW_JOYSTICK_1:
			return "GLFW_JOYSTICK_1";
		case GLFW_JOYSTICK_2:
			return "GLFW_JOYSTICK_2";
		case GLFW_JOYSTICK_3:
			return "GLFW_JOYSTICK_3";
		case GLFW_JOYSTICK_4:
			return "GLFW_JOYSTICK_4";
		case GLFW_JOYSTICK_5:
			return "GLFW_JOYSTICK_5";
		case GLFW_JOYSTICK_6:
			return "GLFW_JOYSTICK_6";
		case GLFW_JOYSTICK_7:
			return "GLFW_JOYSTICK_7";
		case GLFW_JOYSTICK_8:
			return "GLFW_JOYSTICK_8";
		case GLFW_JOYSTICK_9:
			return "GLFW_JOYSTICK_9";
		case GLFW_JOYSTICK_10:
			return "GLFW_JOYSTICK_10";
		case GLFW_JOYSTICK_11:
			return "GLFW_JOYSTICK_11";
		case GLFW_JOYSTICK_12:
			return "GLFW_JOYSTICK_12";
		case GLFW_JOYSTICK_13:
			return "GLFW_JOYSTICK_13";
		case GLFW_JOYSTICK_14:
			return "GLFW_JOYSTICK_14";
		case GLFW_JOYSTICK_15:
			return "GLFW_JOYSTICK_15";
		case GLFW_JOYSTICK_16:
			return "GLFW_JOYSTICK_16";
		default:
			return "UNKNOWN";
			break;
		}
	}( );

	return joy_str;
}

std::string getGlfwJoystickHatName ( int hatId )
{
	auto const hat_str = [hatId] () {
		switch ( hatId )
		{
		case GLFW_HAT_CENTERED:
			return "GLFW_HAT_CENTERED";
		case GLFW_HAT_UP:
			return "GLFW_HAT_UP";
		case GLFW_HAT_RIGHT:
			return "GLFW_HAT_RIGHT";
		case GLFW_HAT_DOWN:
			return "GLFW_HAT_DOWN";
		case GLFW_HAT_LEFT:
			return "GLFW_HAT_LEFT";
		case GLFW_HAT_RIGHT_UP:
			return "GLFW_HAT_RIGHT_UP";
		case GLFW_HAT_RIGHT_DOWN:
			return "GLFW_HAT_RIGHT_DOWN";
		case GLFW_HAT_LEFT_UP:
			return "GLFW_HAT_LEFT_UP";
		case GLFW_HAT_LEFT_DOWN:
			return "GLFW_HAT_LEFT_DOWN";
		default:
			return "UNKNOWN";
		}
	}( );

	return hat_str;
}

std::string getGlfwPressStateName ( int stateId )
{
	auto const press_str = [stateId] () {
		switch ( stateId )
		{
		case GLFW_PRESS:
			return "GLFW_PRESS";
		case GLFW_RELEASE:
			return "GLFW_RELEASE";
		default:
			return "UNKNOWN";
		}		
	}( );

	return press_str;
}

class AbstractJoystick
{
protected:
	int jid_;
	const char* name_;
public:
	AbstractJoystick(int jid = GLFW_JOYSTICK_1);
	AbstractJoystick ( const AbstractJoystick& ) = delete;
	~AbstractJoystick ();

	virtual const char* getName () const
	{
		return name_;
	}

protected:
	virtual void init () = 0;
};

AbstractJoystick::AbstractJoystick ( int jid )
	: jid_ ( jid )
	, name_ ( nullptr )
{
}

AbstractJoystick::~AbstractJoystick()
{}

class Joystick : public AbstractJoystick
{
public:
	int axisCount_;
	const float* pAxisValues_;
	int buttonCount_;
	const unsigned char* pButtonStates_;
	int hatCount_;
	const unsigned char* pHatStates_;
public:
	Joystick ( int jid = GLFW_JOYSTICK_1 );
	Joystick ( const Joystick& ) = delete;
	~Joystick ();

protected:
	virtual void init () override;
};

Joystick::Joystick ( int jid ) 
	: AbstractJoystick ( jid )
	, axisCount_ ( 0 )
	, pAxisValues_ ( nullptr )
	, buttonCount_ ( 0 )
	, pButtonStates_ ( nullptr )
	, hatCount_ ( 0 )
	, pHatStates_ ( nullptr )
{
	if ( glfwJoystickPresent ( jid ) )
	{
		init ();
	}
}

Joystick::~Joystick ()
{
}

void Joystick::init ()
{
	pAxisValues_ = glfwGetJoystickAxes ( jid_, &axisCount_ );
	pButtonStates_ = glfwGetJoystickButtons ( jid_, &buttonCount_ );
	pHatStates_ = glfwGetJoystickHats ( jid_, &hatCount_ );
	name_ = glfwGetJoystickName ( jid_ );
	glfwSetJoystickUserPointer ( jid_, this );
}

class Gamepad : public AbstractJoystick
{
public:
	GLFWgamepadstate state_;
public:
	Gamepad ( int jid = GLFW_JOYSTICK_1 );
	Gamepad ( const Gamepad& ) = delete;
	~Gamepad ();

	bool updateState ()
	{
		return glfwGetGamepadState ( jid_, &state_ );
	}

	void printState () const
	{
		std::cout << "Printing gamepad " << name_ << " state" << std::endl;
		std::cout << getGlfwGamepadAxisName ( GLFW_GAMEPAD_AXIS_LEFT_X ) << ": " << state_.axes [ GLFW_GAMEPAD_AXIS_LEFT_X ] << std::endl;
		std::cout << getGlfwGamepadAxisName ( GLFW_GAMEPAD_AXIS_LEFT_Y ) << ": " << state_.axes [ GLFW_GAMEPAD_AXIS_LEFT_Y ] << std::endl;
		std::cout << getGlfwGamepadAxisName ( GLFW_GAMEPAD_AXIS_RIGHT_X ) << ": " << state_.axes [ GLFW_GAMEPAD_AXIS_RIGHT_X ] << std::endl;
		std::cout << getGlfwGamepadAxisName ( GLFW_GAMEPAD_AXIS_RIGHT_Y ) << ": " << state_.axes [ GLFW_GAMEPAD_AXIS_RIGHT_Y ] << std::endl;
		std::cout << getGlfwGamepadAxisName ( GLFW_GAMEPAD_AXIS_LEFT_TRIGGER ) << ": " << state_.axes [ GLFW_GAMEPAD_AXIS_LEFT_TRIGGER ] << std::endl;
		std::cout << getGlfwGamepadAxisName ( GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER ) << ": " << state_.axes [ GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER ] << std::endl;
		for ( int i = 0; i < 15; i++ )
		{
			std::cout << getGlfwGamepadButtonName ( i ) << ": " << getGlfwPressStateName ( state_.buttons [ i ] ) << std::endl;
		}
	}

protected:
	virtual void init () override;	
};

Gamepad::Gamepad ( int jid )
	: AbstractJoystick ( jid )
{
	if ( glfwJoystickIsGamepad ( jid ) )
	{
		init ();
	}
}

Gamepad::~Gamepad ()
{
}

void Gamepad::init ()
{
	name_ = glfwGetGamepadName ( jid_ );
	if ( glfwGetGamepadState ( jid_, &state_ ) )
	{
		glfwSetJoystickUserPointer ( jid_, this );
	}
}

//
//struct Joystick
//{
//	int jid_;
//	int axisCount_;
//	const float* pAxisValues_;
//	int buttonCount_;
//	const unsigned char * pButtonStates_;
//	int hatCount_;
//	const unsigned char * pHatStates_;
//	const char* name_;
//	const char* guid_;
//
//public:
//	Joystick ( int jid = GLFW_JOYSTICK_1 );
//	~Joystick ();
//
//	bool init ();
//};
//
//Joystick::Joystick ( int jid )
//	: jid_ ( jid )
//	, axisCount_ ( 0 )
//	, pAxisValues_(nullptr)
//	, buttonCount_ ( 0 )
//	, pButtonStates_(nullptr)
//	, hatCount_ ( 0 )
//	, pHatStates_(nullptr)
//	, name_ ( nullptr )
//{
//	if ( glfwJoystickPresent ( jid ) )
//	{
//		init ();
//	}
//}
//
//Joystick::~Joystick()
//{}
//
//bool Joystick::init ()
//{
//	pAxisValues_ = glfwGetJoystickAxes ( jid_, &axisCount_ );
//	pButtonStates_ = glfwGetJoystickButtons ( jid_, &buttonCount_ );
//	pHatStates_ = glfwGetJoystickHats ( jid_, &hatCount_ );
//	name_ = glfwGetJoystickName ( jid_ );
//	glfwSetJoystickUserPointer ( jid_, this );
//}
//
//struct Gamepad
//{
//	int jid_;
//	const char* name_;
//	GLFWgamepadstate state_;
//
//public:
//	Gamepad ( int jid = GLFW_JOYSTICK_1 );
//	~Gamepad ();
//
//	bool init ();
//};
//
//Gamepad::Gamepad ( int jid )
//	: jid_ ( jid )
//	, name_ ( nullptr )
//{
//	if ( glfwJoystickIsGamepad ( jid ) )
//	{
//		init ();
//	}
//}
//
//Gamepad::~Gamepad ()
//{}
//
//bool Gamepad::init ()
//{
//	name_ = glfwGetGamepadName ( jid_ );
//	return glfwGetGamepadState ( jid_, &state_ );
//	glfwSetJoystickUserPointer ( jid_, this );
//}

void queryJoysticksPresent ( int joysticks[], int count )
{
	for ( int i = 0; i < count; i++ )
	{
		joysticks [ i ] = glfwJoystickPresent ( GLFW_JOYSTICK_1 + i );
	}
}

Joystick* getJoystickPtr ( int jid )
{
	if ( glfwJoystickPresent ( jid ) )
	{
		return new Joystick ( jid );		
	}
	else
	{
		return nullptr;
	}
}

Gamepad* getGamepad ( int jid )
{
	if ( glfwJoystickPresent ( jid ) )
	{
		return new Gamepad ( jid );
	}
	else
	{
		return nullptr;
	}
}

void updateGamepadValues ( int jid )
{
	GLFWgamepadstate state;
	if ( glfwGetGamepadState ( jid, &state ) )
	{
		g_pitchDegrees = kMaxPitchDegrees * state.axes [ GLFW_GAMEPAD_AXIS_LEFT_Y ];
		g_yawDegrees = kMaxYawDegrees * state.axes [ GLFW_GAMEPAD_AXIS_LEFT_X ];
		g_rollDegrees = kMaxRollDegrees * state.axes [ GLFW_GAMEPAD_AXIS_RIGHT_X ];
	}
}

int main ( int argc, char** argv )
{
	GLFWApp app;

	GLShader shdTriVert ( appendToRoot ( "assets/shaders/gamepad_simple_triangle.vert" ).c_str () );
	GLShader shdTriFrag ( appendToRoot ( "assets/shaders/gamepad_simple_triangle.frag" ).c_str () );
	GLProgram progTri ( shdTriVert, shdTriFrag );

	g_prog_tri = progTri.getHandle ();

#ifndef NDEBUG
	const GLchar* kLabelTriProg = "TriProgram";
	glObjectLabel ( GL_PROGRAM, g_prog_tri, std::strlen ( kLabelTriProg ) + 1, kLabelTriProg );
#endif

	const Vertex triVerts [ 3 ] = {
		{.pos = { -0.2f, -0.2f, 0.0f, 1.0f }, .color = { 1.0f, 0.0f, 0.0f, 1.0f }},
		{.pos = {  0.2f, -0.2f, 0.0f, 1.0f }, .color = { 0.0f, 1.0f, 0.0f, 1.0f }},
		{.pos = {  0.0f,  0.2f, 0.0f, 1.0f }, .color = { 0.0f, 0.0f, 1.0f, 1.0f }}
	};

	const TriangleOffsets triOffsets = {
		.offsets = {
			vec2 ( -0.6f, -0.6f ), vec2 ( -0.2f, -0.6f ), vec2 ( 0.2f, -0.6f ), vec2 ( 0.6f, -0.6f ),
			vec2 ( -0.6f, -0.2f ), vec2 ( -0.2f, -0.2f ), vec2 ( 0.2f, -0.2f ), vec2 ( 0.6f, -0.2f ),
			vec2 ( -0.6f,  0.2f ), vec2 ( -0.2f,  0.2f ), vec2 ( 0.2f,  0.2f ), vec2 ( 0.6f,  0.2f ),
			vec2 ( -0.6f,  0.6f ), vec2 ( -0.2f,  0.6f ), vec2 ( 0.2f,  0.6f ), vec2 ( 0.6f,  0.6f )
		}
	};

	const GLuint triIndices [ 3 ] = { 0, 1, 2 };

	const GLsizeiptr kVertexBufferSize = 3 * sizeof ( Vertex );
	const GLsizeiptr kPerFrameBufferSize = sizeof ( PerFrameData );
	const GLsizeiptr kOffsetsBufferSize = 16 * sizeof ( vec2 );
	const GLsizeiptr kIndexBufferSize = 3 * sizeof ( GLuint );

	GLBuffer perFrameDataBuffer ( kPerFrameBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	GLBuffer triangleOffsetsBuffer ( kOffsetsBufferSize, &triOffsets, 0 );

	g_per_frame_ubo = perFrameDataBuffer.getHandle ();
	g_offsets_ubo = triangleOffsetsBuffer.getHandle ();

#ifndef NDEBUG
	const GLchar* kLabelPerFrameDataBuffer = "PerFrameDataBuffer";
	const GLchar* kLabelOffsetsBuffer = "TriangleOffsetsBuffer";
	glObjectLabel ( GL_BUFFER, g_per_frame_ubo, std::strlen ( kLabelPerFrameDataBuffer ) + 1, kLabelPerFrameDataBuffer );
	glObjectLabel ( GL_BUFFER, g_offsets_ubo, std::strlen ( kLabelOffsetsBuffer ) + 1, kLabelOffsetsBuffer );
#endif

	glBindBufferBase ( GL_UNIFORM_BUFFER, kIdxBind_PerFrameData, g_per_frame_ubo );
	glBindBufferBase ( GL_UNIFORM_BUFFER, kIdxBind_TriBuffer, g_offsets_ubo );

	GLBuffer vertexBuffer ( kVertexBufferSize, triVerts, 0 );
	GLBuffer elementBuffer ( kIndexBufferSize, triIndices, 0 );

	g_vbo = vertexBuffer.getHandle ();
	g_ebo = elementBuffer.getHandle ();

#ifndef NDEBUG
	const GLchar* kLabelVertexBuffer = "VertexBuffer";
	const GLchar* kLabelIndexBuffer = "IndexBuffer";
	glObjectLabel ( GL_BUFFER, g_vbo, std::strlen ( kLabelVertexBuffer ) + 1, kLabelVertexBuffer );
	glObjectLabel ( GL_BUFFER, g_ebo, std::strlen ( kLabelIndexBuffer ) + 1, kLabelIndexBuffer );
#endif

	GLuint vao;
	glCreateVertexArrays ( 1, &vao );

	g_vao = vao;
#ifndef NDEBUG
	const GLchar* kLabelVertexArrayObject = "VertexArrayObject";
	glObjectLabel ( GL_VERTEX_ARRAY, g_vao, std::strlen ( kLabelVertexArrayObject ) + 1, kLabelVertexArrayObject );
#endif

	glVertexArrayElementBuffer ( g_vao, g_ebo );

	glVertexArrayAttribBinding ( g_vao, kIdxAttr_inPosition, kIdxBind_TriBuffer );
	glVertexArrayAttribFormat ( g_vao, kIdxAttr_inPosition, 4, GL_FLOAT, GL_FALSE, offsetof ( Vertex, pos ) );
	glEnableVertexArrayAttrib ( g_vao, kIdxAttr_inPosition );

	glVertexArrayAttribBinding ( g_vao, kIdxAttr_inColor, kIdxBind_TriBuffer );
	glVertexArrayAttribFormat ( g_vao, kIdxAttr_inColor, 4, GL_FLOAT, GL_FALSE, offsetof ( Vertex, color ) );
	glEnableVertexArrayAttrib ( g_vao, kIdxAttr_inColor );

	glVertexArrayVertexBuffer ( g_vao, kIdxBind_TriBuffer, g_vbo, 0, sizeof ( Vertex ) );

	glBindVertexArray ( g_vao );

	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );

	queryJoysticksPresent ( g_joysticksPresent, 16 );

	std::vector<int> toEnableIndices;

	for ( int i = 0; i < 16; i++ )
	{
		if ( g_joysticksPresent [ i ] == GLFW_TRUE )
		{
			std::cout << "GLFW_JOYSTICK_" << g_joysticksPresent [ i ] << " is present\n";
			toEnableIndices.push_back ( i );
		}
	}

	std::cout << "NumJoysticksPresent: " << toEnableIndices.size () << std::endl;

//	AbstractJoystick* pAbstractJoystick = getJoystickPtr ( 1 );
//	Joystick* pJoystick = nullptr;
//	Gamepad* pGamepad = nullptr;
	AbstractJoystick* pAbstractJoystick = nullptr;

	if ( glfwJoystickIsGamepad ( GLFW_JOYSTICK_1 ) )
	{
//		pGamepad = reinterpret_cast< Gamepad* >( pAbstractJoystick );
		pAbstractJoystick = (AbstractJoystick *)getGamepad ( GLFW_JOYSTICK_1 );
		std::cout << "Joystick is gamepad" << std::endl;
	}
	else
	{
//		pJoystick = reinterpret_cast< Joystick* >( pAbstractJoystick );
		pAbstractJoystick = ( AbstractJoystick* ) getJoystickPtr ( GLFW_JOYSTICK_1 );
		std::cout << "Joystick is NOT gamepad" << std::endl;
	}

	std::cout << "Joystick name: " << pAbstractJoystick->getName () << std::endl;

	reinterpret_cast< Gamepad* >( pAbstractJoystick )->printState ();

	glfwSetCursorPosCallback (
		app.getWindow (),
		[] ( auto* window, double x, double y )
		{
			int width, height;
			glfwGetFramebufferSize ( window, &width, &height );
			mouseState.pos.x = static_cast< float >( x / width );
			mouseState.pos.y = static_cast< float >( y / height );

		}
	);

	glfwSetMouseButtonCallback (
		app.getWindow (),
		[] ( auto* window, int button, int action, int mods )
		{
			const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
			if ( button == GLFW_MOUSE_BUTTON_LEFT )
			{
				mouseState.pressedLeft = action == GLFW_PRESS;
			}
		}
	);

	glfwSetKeyCallback (
		app.getWindow (),
		[] ( auto* window, int key, int scancode, int action, int mods )
		{
			const bool pressed = action != GLFW_RELEASE;
			if ( key == GLFW_KEY_ESCAPE && pressed )
			{
				glfwSetWindowShouldClose ( window, GLFW_TRUE );
			}

			if ( key == GLFW_KEY_PERIOD && pressed )
			{
				g_theta += g_deltaTheta;
				if ( g_theta > glm::two_pi<float> () )
				{
					g_theta -= glm::two_pi<float> ();
				}
			}
			if ( key == GLFW_KEY_COMMA && pressed )
			{
				g_theta -= g_deltaTheta;
				if ( g_theta < 0.0f )
				{
					g_theta += glm::two_pi<float> ();
				}
			}

			if ( key == GLFW_KEY_W )
				positioner.movement_.forward_ = pressed;
			if ( key == GLFW_KEY_S )
				positioner.movement_.backward_ = pressed;
			if ( key == GLFW_KEY_A )
				positioner.movement_.left_ = pressed;
			if ( key == GLFW_KEY_D )
				positioner.movement_.right_ = pressed;
			if ( key == GLFW_KEY_Q )
				positioner.movement_.up_ = pressed;
			if ( key == GLFW_KEY_Z )
				positioner.movement_.down_ = pressed;
			if ( key == GLFW_KEY_SPACE )
				positioner.setUpVector ( vec3 ( 0.0f, 1.0f, 0.0f ) );
			if ( key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT )
				positioner.movement_.fastSpeed_ = pressed;

		}
	);

	fpsCounter.printFPS_ = false;

	double timeStamp = glfwGetTime ();
	float deltaSeconds = 0.0f;

	while ( !glfwWindowShouldClose ( app.getWindow () ) )
	{
		positioner.update ( deltaSeconds, mouseState.pos, mouseState.pressedLeft );
		fpsCounter.tick ( deltaSeconds );

		const double newTimeStamp = glfwGetTime ();
		deltaSeconds = static_cast< float >( newTimeStamp - timeStamp );
		timeStamp = newTimeStamp;

		const float fps = fpsCounter.getFPS ();

		updateGamepadValues ( GLFW_JOYSTICK_1 );

		mat4 gamepadModel = glm::yawPitchRoll ( glm::radians ( g_yawDegrees ), glm::radians ( g_pitchDegrees ), glm::radians ( g_rollDegrees ) );

		int width, height;
		glfwGetFramebufferSize ( app.getWindow (), &width, &height );
		const float ratio = width / ( float ) height;

		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		const mat4 proj = glm::perspective ( 45.0f, ratio, 0.1f, 10.0f );
		const mat4 view = camera.getViewMatrix ();
		const mat4 model = glm::rotate ( mat4 ( 1.0f ), g_theta, vec3 ( 0.f, 0.f, 1.f ) );

		const PerFrameData perFrameData = {
			.proj = proj,
			.view = view,
			.model = model * gamepadModel
		};

		glNamedBufferSubData ( g_per_frame_ubo, 0, kPerFrameBufferSize, &perFrameData );

		glBindVertexArray ( g_vao );

		progTri.useProgram ();

		glDrawArraysInstanced ( GL_TRIANGLES, 0, 3, 4 );

		//glDrawElementsInstanced ( GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr, 16 );

		app.swapBuffers ();
	}

	glDeleteVertexArrays ( 1, &g_vao );

	return 0;
}
