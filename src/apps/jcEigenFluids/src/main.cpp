#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

//#include <jcCommonFramework/Bitmap.h>
//#include <jcCommonFramework/Camera.h>
//#include <jcCommonFramework/UtilsMath.h>
#include <jcCommonFramework/ResourceString.h>
//#include <jcCommonFramework/scene/VtxData.h>

#include <jcGLframework/GLFWApp.h>
#include <jcGLframework/GLShader.h>
//#include <jcGLframework/GLTexture.h>
//#include <jcGLframework/GLFramebuffer.h>
//#include <jcGLframework/UtilsGLImGui.h>

#include <jcFluidSim/FluidSolver.h>

using glm::mat4;
using glm::mat3;
using glm::mat2;
using glm::vec4;
using glm::vec3;
using glm::vec2;

const uint32_t kVelocityGridResolutionX = 256;
const uint32_t kVelocityGridResolutionY = 256;
const uint32_t kBasisDimension = 81;

const float kVelocityLengthScale = 30.0f / std::numbers::pi_v<float>;

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 model;
	uint32_t velocityGridResolutionX;
	uint32_t velocityGridResolutionY;	
	float velocityLengthScale;
};

float forceMultiplier = 50.0f;

bool leftMouseDown = false;
vec2 mousePoint1 = vec2 ( 0.0f );
vec2 mousePoint2 = vec2 ( 0.0f );

std::unique_ptr<FluidSolver> pFluidSolver;

//FluidSolver solver ( kBasisDimension, kVelocityGridResolutionX, kVelocityGridResolutionY );

int main ()
{
	GLFWApp app ( 1600, 900, "Fluid Simulation" );

	GLShader shdVelocityVert ( appendToRoot ( "assets/shaders/VelocityRender2D.vert" ).c_str () );
	GLShader shdVelocityGeom ( appendToRoot ( "assets/shaders/VelocityRender2D.geom" ).c_str () );
	GLShader shdVelocityFrag ( appendToRoot ( "assets/shaders/VelocityRender2D.frag" ).c_str () );
	GLProgram progVelocity ( shdVelocityVert, shdVelocityGeom, shdVelocityFrag );

	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );
	
	GLBuffer bufPerFrameData ( kUniformBufferSize, nullptr,  GL_DYNAMIC_DRAW );

	glBindBufferRange ( GL_UNIFORM_BUFFER, 0, bufPerFrameData.getHandle (), 0, kUniformBufferSize );

	pFluidSolver = std::make_unique<FluidSolver> ( kBasisDimension, kVelocityGridResolutionX, kVelocityGridResolutionY );

	pFluidSolver->initFields ();
	pFluidSolver->fillLookupTables ();
	pFluidSolver->precomputeBasisFields ();
	pFluidSolver->precomputeDynamics ();
	pFluidSolver->zeroVelocity ();
	pFluidSolver->updateTransforms ();
	pFluidSolver->expandBasis ();

	int velocityBufferSize;
	const void* velocityBufferData = pFluidSolver->getVelocityField ( &velocityBufferSize );

	const GLsizeiptr kVelocityBufferSize = static_cast< const GLsizeiptr >( velocityBufferSize );

	GLBuffer bufVelocity2D ( kVelocityBufferSize, velocityBufferData, GL_DYNAMIC_DRAW );

	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, 1, bufVelocity2D.getHandle () );

	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );
	
	glfwSetCursorPosCallback (
		app.getWindow (),
		[] ( auto* window, double x, double y )
		{
			int width, height;
			glfwGetFramebufferSize ( window, &width, &height );

			if ( leftMouseDown )
			{
				mousePoint2.x = static_cast< float >( x / width );
				mousePoint2.y = static_cast< float >( y / height );

				float fx = ( mousePoint2.x - mousePoint1.x ) * forceMultiplier;
				float fy = ( mousePoint2.y - mousePoint1.y ) * forceMultiplier;

				std::vector<vec4> forcePath ( 2, vec4 ( 0.0f ) );
				forcePath [ 0 ] = vec4 ( mousePoint1.x, mousePoint1.y, fx, fy );
				pFluidSolver->stir ( forcePath );
				mousePoint1 = mousePoint2;				
			}
		}
	);

	glfwSetMouseButtonCallback (
		app.getWindow (),
		[] ( auto* window, int button, int action, int mods )
		{
			const bool pressed = action == GLFW_PRESS;
			const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
			
			if ( pressed )
			{
				if ( button == GLFW_MOUSE_BUTTON_LEFT )
				{
					leftMouseDown = true;				
				}
			}
			else // for now, we just always release
			{
				leftMouseDown = false;			
			}
		}
	);

	glfwSetKeyCallback (
		app.getWindow (),
		[] ( auto* window, int key, int scancode, int action, int mods )
		{
			const bool pressed = action != GLFW_RELEASE;
			if ( key == GLFW_KEY_ESCAPE && pressed )
				glfwSetWindowShouldClose ( window, GLFW_TRUE );		
		}
	);
			
	while ( !glfwWindowShouldClose ( app.getWindow () ) )
	{
		pFluidSolver->zeroModes ();
		pFluidSolver->updateFields ();
		pFluidSolver->expandBasis ();
		
		int width, height;
		glfwGetFramebufferSize ( app.getWindow (), &width, &height );
		
		glViewport ( 0, 0, width, height );

		const float L = 0.0f;
		const float R = static_cast< float >( width );
		const float T = 0.0f;
		const float B = static_cast< float >( height );

		const float scaleX = 1.0f / static_cast< float >( kVelocityGridResolutionX );
		const float scaleY = 1.0f / static_cast< float >( kVelocityGridResolutionY );

		const mat4 p = glm::ortho ( L, R, T, B );
		const mat4 v = mat4 ( 1.0f );
		const mat4 m = glm::scale ( mat4 ( 1.0f ), vec3 ( scaleX, scaleY, 1.0f ) );

		const PerFrameData perFrameData = {
			.view = v,
			.proj = p,
			.model = m,
			.velocityGridResolutionX = kVelocityGridResolutionX,
			.velocityGridResolutionY = kVelocityGridResolutionY,
			.velocityLengthScale = kVelocityLengthScale
		};

		glNamedBufferSubData ( bufPerFrameData.getHandle(), 0, kUniformBufferSize, &perFrameData);

		const void* velBufData = pFluidSolver->getVelocityField ( &velocityBufferSize );

		glNamedBufferSubData ( bufVelocity2D.getHandle (), 0, static_cast< GLsizeiptr >( velocityBufferSize ), velBufData );

		progVelocity.useProgram ();

		const uint32_t numPointsToDraw = velocityBufferSize / sizeof ( vec2 );

		glDrawArrays ( GL_POINTS, 0, numPointsToDraw );

		app.swapBuffers ();		
	}

	pFluidSolver = nullptr;

	return 0;	
}