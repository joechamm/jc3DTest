#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <jcGLframework/GLFWApp.h>
#include <jcGLframework/GLShader.h>
#include <jcGLframework/GLTexture.h>
#include <jcGLframework/GLFramebuffer.h>
#include <jcGLframework/UtilsGLImGui.h>
#include <jcGLframework/LineCanvasGL.h>
#include <jcGLframework/debug.h>
#include <jcCommonFramework/ResourceString.h>

using glm::mat4;
using glm::mat3;
using glm::mat2;
using glm::vec4;
using glm::vec3;
using glm::vec2;

//constexpr uint32_t g_FramebufferWidth = 512;
//constexpr uint32_t g_FramebufferHeight = 512;

bool g_DrawEyeball = false;

constexpr uint32_t g_ViewportWidth = 1024;
constexpr uint32_t g_ViewportHeight = 1024;

constexpr uint32_t g_GridWidth = g_ViewportWidth / 2;
constexpr uint32_t g_GridHeight = g_ViewportHeight / 2;

constexpr uint32_t g_HiresViewportWidth = 2 * g_ViewportWidth;
constexpr uint32_t g_HiresViewportHeight = 2 * g_ViewportHeight;

constexpr uint32_t g_NumJacobiIterations = 40;

constexpr bool g_UseHalfFloats = true;

constexpr bool g_DrawBorder = true;
constexpr bool g_DrawCircle = true;

constexpr float g_BorderTrim = 0.9999f;

constexpr float g_SplatRadius = ( float ) g_GridWidth / 8.0f;
constexpr float g_CellSize = 1.25f;
constexpr float g_TimeStep = 0.125f;
constexpr float g_AmbientTemperature = 0.0f;
constexpr float g_ImpulseTemperature = 10.0f;
constexpr float g_ImpulseDensity = 1.0f;
constexpr float g_SmokeBuoyancy = 1.0f;
constexpr float g_SmokeWeight = 0.05f;
constexpr float g_GradientScale = 1.125f / g_CellSize;
constexpr float g_TemperatureDissipation = 0.99f;
constexpr float g_VelocityDissipation = 0.99f;
constexpr float g_DensityDissipation = 0.9999f;
constexpr vec2 g_ImpulsePosition = vec2 ( g_GridWidth / 2, -g_SplatRadius / 2 );

GLuint g_VisualizeHandle = 0;
GLuint g_AdvectHandle = 0;
GLuint g_BuoyancyHandle = 0;
GLuint g_ComputeDivergenceHandle = 0;
GLuint g_FillHandle = 0;
GLuint g_JacobiHandle = 0;
GLuint g_SubtractGradientHandle = 0;
GLuint g_SplatHandle = 0;

GLuint g_QuadVAO = 0;

struct MouseState
{
	vec2 pos = vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

struct Slab
{
	std::unique_ptr<GLFramebuffer> ping_;
	std::unique_ptr<GLFramebuffer> pong_;
};

GLuint CreateQuad ()
{
	short positions [] = {
		-1, -1,
		1, -1,
		-1, 1,
		1, 1
	};

	// Create the vao
	GLuint vao;
	glGenVertexArrays ( 1, &vao );
	glBindVertexArray ( vao );

	// Create the vbo 
	GLuint vbo;
	GLsizeiptr size = sizeof ( positions );
	glGenBuffers ( 1, &vbo );
	glBindBuffer ( GL_ARRAY_BUFFER, vbo );
	glBufferData ( GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW );

	// Set up the vertex layout:
	GLsizeiptr stride = 2 * sizeof ( positions [ 0 ] );
	glEnableVertexAttribArray ( 0 );
	glVertexAttribPointer ( 0, 2, GL_SHORT, GL_FALSE, stride, 0 );
	
	return vao;
}

Slab CreateSlab ( uint32_t width, uint32_t height, uint32_t numComponents )
{
	Slab slab;

	if ( g_UseHalfFloats )
	{
		switch ( numComponents )
		{
		case 1:
			slab.ping_ = std::make_unique<GLFramebuffer> ( width, height, GL_R16F, 0 );
			slab.pong_ = std::make_unique<GLFramebuffer> ( width, height, GL_R16F, 0 );
			break;
		case 2:
			slab.ping_ = std::make_unique<GLFramebuffer> ( width, height, GL_RG16F, 0 );
			slab.pong_ = std::make_unique<GLFramebuffer> ( width, height, GL_RG16F, 0 );
			break;
		case 3:
			slab.ping_ = std::make_unique<GLFramebuffer> ( width, height, GL_RGB16F, 0 );
			slab.pong_ = std::make_unique<GLFramebuffer> ( width, height, GL_RGB16F, 0 );
			break;
		case 4:
			slab.ping_ = std::make_unique<GLFramebuffer> ( width, height, GL_RGBA16F, 0 );
			slab.pong_ = std::make_unique<GLFramebuffer> ( width, height, GL_RGBA16F, 0 );
			break;
		default:
			throw std::runtime_error ( "Invalid Framebuffer" );
			break;
		}
	}
	else
	{
		switch ( numComponents )
		{
		case 1:
			slab.ping_ = std::make_unique<GLFramebuffer> ( width, height, GL_R32F, 0 );
			slab.pong_ = std::make_unique<GLFramebuffer> ( width, height, GL_R32F, 0 );
			break;
		case 2:
			slab.ping_ = std::make_unique<GLFramebuffer> ( width, height, GL_RG32F, 0 );
			slab.pong_ = std::make_unique<GLFramebuffer> ( width, height, GL_RG32F, 0 );
			break;
		case 3:
			slab.ping_ = std::make_unique<GLFramebuffer> ( width, height, GL_RGB32F, 0 );
			slab.pong_ = std::make_unique<GLFramebuffer> ( width, height, GL_RGB32F, 0 );
			break;
		case 4:
			slab.ping_ = std::make_unique<GLFramebuffer> ( width, height, GL_RGBA32F, 0 );
			slab.pong_ = std::make_unique<GLFramebuffer> ( width, height, GL_RGBA32F, 0 );
			break;
		default:
			throw std::runtime_error ( "Invalid Framebuffer" );
			break;
		}
	}

	return slab;
}

void SwapSurfaces ( Slab* slab )
{
	std::swap ( slab->ping_, slab->pong_ );
}

bool CreateObstacles ( GLuint prog, uint32_t width, uint32_t height, GLFramebuffer* dest )
{
	if(dest == nullptr)
		return false;

	glBindFramebuffer ( GL_FRAMEBUFFER, dest->getHandle () );
	glViewport ( 0, 0, width, height );
	glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear ( GL_COLOR_BUFFER_BIT );

	glUseProgram ( prog );

	GLuint vao;
	glGenVertexArrays ( 1, &vao );
	glBindVertexArray ( vao );

	if ( g_DrawBorder )
	{
#define T 0.9999f
		float positions [] = { -T, -T, T, -T, T, T, -T, T, -T, -T };
#undef T

		GLuint vbo;
		GLsizeiptr size = sizeof ( positions );
		glGenBuffers ( 1, &vbo );
		glBindBuffer ( GL_ARRAY_BUFFER, vbo );
		glBufferData ( GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW );
		GLsizeiptr stride = 2 * sizeof ( positions [ 0 ] );
		glEnableVertexAttribArray ( 0 );
		glVertexAttribPointer ( 0, 2, GL_FLOAT, GL_FALSE, stride, 0 );
		glDrawArrays ( GL_LINE_STRIP, 0, 5 );
		glDeleteBuffers ( 1, &vbo );
	}

	if ( g_DrawCircle )
	{
		const int slices = 64;
		float positions [ slices * 2 * 3 ];
		float twopi = 8 * atanf ( 1.0f );
		float theta = 0.0f;
		float dtheta = twopi / ( float ) ( slices - 1 );
		float* pPositions = &positions [ 0 ];
		for ( int i = 0; i < slices; i++ )
		{
			*pPositions++ = 0;
			*pPositions++ = 0;
			
			*pPositions++ = 0.25f * cosf ( theta ) * height / width;
			*pPositions++ = 0.25f * sinf ( theta );
			theta += dtheta;

			*pPositions++ = 0.25f * cosf ( theta ) * height / width;
			*pPositions++ = 0.25f * sinf ( theta );
		}
		
		GLuint vbo;
		GLsizeiptr size = sizeof ( positions );
		glGenBuffers ( 1, &vbo );
		glBindBuffer ( GL_ARRAY_BUFFER, vbo );
		glBufferData ( GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW );
		GLsizeiptr stride = 2 * sizeof(positions[0]);
		glEnableVertexAttribArray ( 0 );
		glVertexAttribPointer ( 0, 2, GL_FLOAT, GL_FALSE, stride, nullptr );
		glDrawArrays ( GL_TRIANGLES, 0, slices * 3 );
		glDeleteBuffers ( 1, &vbo );		
	}

	glDeleteVertexArrays ( 1, &vao );

	return true;
}

bool ClearSurface ( GLFramebuffer* surfaceFramebuffer, float v )
{
	if(surfaceFramebuffer == nullptr) return false;

//	glClearNamedFramebufferfv ( surfaceFramebuffer->getHandle (), GL_COLOR, 0, glm::value_ptr ( vec4(v, v, v, v )) );
	glBindFramebuffer ( GL_FRAMEBUFFER, surfaceFramebuffer->getHandle () );
	glClearColor ( v, v, v, v );
	glClear ( GL_COLOR_BUFFER_BIT );

	return true;
}

void Advect ( GLuint prog, GLuint quadVAO, GLuint velocityTex, GLuint sourceTex, GLuint obstaclesTex, GLFramebuffer* destFramebuffer, float dissipation )
{
	glBindVertexArray ( quadVAO );

	glUseProgram ( prog );

	glUniform2f ( 0, 1.0f / g_GridWidth, 1.0f / g_GridHeight ); // inverse size
	glUniform1f ( 1, g_TimeStep );  // time step
	glUniform1f ( 2, dissipation ); // dissipation

	destFramebuffer->bind ();
	glBindTextureUnit ( 0, velocityTex );
	glBindTextureUnit ( 1, sourceTex );
	glBindTextureUnit ( 2, obstaclesTex );

	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );

	glBindTextureUnit ( 0, 0 );
	glBindTextureUnit ( 1, 0 );
	glBindTextureUnit ( 2, 0 );
	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void Jacobi ( GLuint prog, GLuint quadVAO, GLuint pressureTex, GLuint divergenceTex, GLuint obstaclesTex, GLFramebuffer* destFramebuffer )
{
	glBindVertexArray ( quadVAO );

	glUseProgram ( prog );
	
	glUniform1f ( 0, -g_CellSize * g_CellSize ); // alpha
	glUniform1f ( 1, 0.25f ); // inverse beta
	
	destFramebuffer->bind ();
	glBindTextureUnit ( 5, pressureTex );
	glBindTextureUnit ( 6, divergenceTex );
	glBindTextureUnit ( 2, obstaclesTex );

	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );

	glBindTextureUnit ( 5, 0 );
	glBindTextureUnit ( 6, 0 );
	glBindTextureUnit ( 2, 0 );
	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void SubtractGradient ( GLuint prog, GLuint quadVAO, GLuint velocityTex, GLuint pressureTex, GLuint obstaclesTex, GLFramebuffer* destFramebuffer )
{
	glBindVertexArray ( quadVAO );

	glUseProgram ( prog );

	glUniform1f ( 0, g_GradientScale ); // gradient scale
	
	destFramebuffer->bind ();
	glBindTextureUnit ( 0, velocityTex );
	glBindTextureUnit ( 5, pressureTex );
	glBindTextureUnit ( 2, obstaclesTex );

	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );

	glBindTextureUnit ( 0, 0 );
	glBindTextureUnit ( 5, 0 );
	glBindTextureUnit ( 2, 0 );

	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void ComputeDivergence(GLuint prog, GLuint quadVAO, GLuint velocityTex, GLuint obstaclesTex, GLFramebuffer* destFramebuffer)
{
	glBindVertexArray ( quadVAO );
	
	glUseProgram ( prog );

	glUniform1f(0, 0.5f / g_CellSize); // half inverse cell size
	
	destFramebuffer->bind ();
	glBindTextureUnit ( 0, velocityTex );
	glBindTextureUnit ( 2, obstaclesTex );

	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );

	glBindTextureUnit ( 0, 0 );
	glBindTextureUnit ( 2, 0 );

	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void ApplyImpulse ( GLuint prog, GLuint quadVAO, const vec2& position, float value, GLFramebuffer* destFramebuffer )
{
	glBindVertexArray ( quadVAO );

	glUseProgram ( prog );

	glUniform2f ( 0, position.x, position.y ); // Point
	glUniform1f ( 1, g_SplatRadius ); // Radius
	glUniform3f ( 2, value, value, value );  // Fill Color

	destFramebuffer->bind ();
	glEnable ( GL_BLEND );
	
	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );

	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );	
}

void ApplyBuoyancy(GLuint prog, GLuint quadVAO, GLuint velocityTex, GLuint temperatureTex, GLuint densityTex, GLFramebuffer* destFramebuffer)
{
	glBindVertexArray ( quadVAO );

	glUseProgram ( prog );

	glUniform1f ( 0, g_AmbientTemperature ); // Ambient Temperature
	glUniform1f ( 1, g_TimeStep ); // Time Step
	glUniform1f ( 2, g_SmokeBuoyancy ); // Sigma
	glUniform1f ( 3, g_SmokeWeight ); // Kappa 
	
	destFramebuffer->bind ();
	glBindTextureUnit ( 0, velocityTex );
	glBindTextureUnit ( 3, temperatureTex );
	glBindTextureUnit ( 4, densityTex );

	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );

	glBindTextureUnit ( 0, 0 );
	glBindTextureUnit ( 3, 0 );
	glBindTextureUnit ( 4, 0 );

	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void RenderQuad ( GLuint prog, GLuint quadVAO, GLuint densityTex, GLuint hiresObsTex )
{
	glBindVertexArray ( quadVAO );
	
	glUseProgram ( prog );

	glEnable ( GL_BLEND );

	glViewport ( 0, 0, g_ViewportWidth, g_ViewportHeight );
	glBindFramebuffer ( GL_FRAMEBUFFER, 0 );
	glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear ( GL_COLOR_BUFFER_BIT );

	// Draw ink;
	glBindTextureUnit ( 7, densityTex );
	glUniform2f ( 0, 1.0f / g_ViewportWidth, 1.0f / g_ViewportHeight ); // Scale 
	glUniform3f(1, 1.0f, 1.0f, 1.0f); // Fill Color
	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );

	// Draw obstacles;
	glBindTextureUnit ( 7, hiresObsTex );
	glUniform3f(1, 0.125f, 0.4f, 0.75f); // Fill Color
	glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );

	// disable blending
	glDisable ( GL_BLEND );
}

int main ()
{
	GLFWApp app ( g_ViewportWidth, g_ViewportHeight, "Fluid Sim" );

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
				mouseState.pressedLeft = action == GLFW_PRESS;
		}
	);

	glfwSetKeyCallback (
		app.getWindow (),
		[] ( GLFWwindow* window, int key, int scancode, int action, int mods )
		{
			const bool pressed = action != GLFW_RELEASE;
			if ( key == GLFW_KEY_SPACE )
			{
				g_DrawEyeball = pressed;
			}
			if ( key == GLFW_KEY_ESCAPE && pressed )
				glfwSetWindowShouldClose ( window, GLFW_TRUE );
		}
	);
	
	GLShader shdVert ( appendToRoot ( "assets/shaders/Visualize.vert" ).c_str () );
	GLShader shdFrag ( appendToRoot ( "assets/shaders/Visualize.frag" ).c_str () );
	GLProgram progRender ( shdVert, shdFrag );

	g_VisualizeHandle = progRender.getHandle ();

	GLShader shdAdvect ( appendToRoot ( "assets/shaders/Advect.frag" ).c_str () );
	GLProgram progAdvect ( shdVert, shdAdvect );

	g_AdvectHandle = progAdvect.getHandle ();

	GLShader shdDivergence ( appendToRoot ( "assets/shaders/ComputeDivergence.frag" ).c_str () );
	GLProgram progDivergence ( shdVert, shdDivergence );

	g_ComputeDivergenceHandle = progDivergence.getHandle ();

	GLShader shdGradient ( appendToRoot ( "assets/shaders/SubtractGradient.frag" ).c_str () );
	GLProgram progGradient ( shdVert, shdGradient );

	g_SubtractGradientHandle = progGradient.getHandle ();

	GLShader shdJacobi( appendToRoot ( "assets/shaders/Jacobi.frag" ).c_str () );
	GLProgram progJacobi ( shdVert, shdJacobi );

	g_JacobiHandle = progJacobi.getHandle ();
	
	GLShader shdImpulse ( appendToRoot ( "assets/shaders/Splat.frag" ).c_str () );
	GLProgram progImpulse ( shdVert, shdImpulse );

	g_SplatHandle = progImpulse.getHandle ();

	GLShader shdBuoyancy ( appendToRoot ( "assets/shaders/Buoyancy.frag" ).c_str () );
	GLProgram progBuoyancy ( shdVert, shdBuoyancy );

	g_BuoyancyHandle = progBuoyancy.getHandle ();

	GLShader shdSetup ( appendToRoot ( "assets/shaders/Fill.frag" ).c_str () );
	GLProgram progSetup ( shdVert, shdSetup );

	g_FillHandle = progSetup.getHandle ();

//	GLShader shdRendImg ( appendToRoot ( "assets/shaders/FluidRenderImage.frag" ).c_str () );
//	GLProgram progRenderImg ( shdVert, shdRendImg );

	Slab velocitySlab = CreateSlab ( g_GridWidth, g_GridHeight, 2 );
	Slab densitySlab = CreateSlab ( g_GridWidth, g_GridHeight, 1 );
	Slab pressureSlab = CreateSlab ( g_GridWidth, g_GridHeight, 1 );
	Slab temperatureSlab = CreateSlab ( g_GridWidth, g_GridHeight, 1 );
	
	GLFramebuffer divergenceFramebuffer ( g_GridWidth, g_GridHeight, g_UseHalfFloats ? GL_RGB16F : GL_RGB32F, 0 );
	GLFramebuffer obstaclesFramebuffer ( g_GridWidth, g_GridHeight, g_UseHalfFloats ? GL_RGB16F : GL_RGB32F, 0 );

	CreateObstacles ( g_FillHandle, g_GridWidth, g_GridHeight, &obstaclesFramebuffer );

	GLFramebuffer hiresObstaclesFramebuffer ( g_HiresViewportWidth, g_HiresViewportHeight, g_UseHalfFloats ? GL_R16F : GL_R32F, 0 );

	CreateObstacles ( g_FillHandle, g_HiresViewportWidth, g_HiresViewportHeight, &hiresObstaclesFramebuffer );

	g_QuadVAO = CreateQuad ();
	
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	ClearSurface ( temperatureSlab.ping_.get(), g_AmbientTemperature);

//	GLTexture eyeBallTexture ( GL_TEXTURE_2D, appendToRoot ( "assets/images/eyeball.bmp" ) );
			
	while ( !glfwWindowShouldClose ( app.getWindow () ) )
	{
		// Update

		glViewport ( 0, 0, g_GridWidth, g_GridHeight );

		GLuint velPingTex = velocitySlab.ping_->getTextureColor ().getHandle ();
		GLuint obsTex = obstaclesFramebuffer.getTextureColor ().getHandle ();
		GLuint tempPingTex = temperatureSlab.ping_->getTextureColor ().getHandle ();
		GLuint densPingTex = densitySlab.ping_->getTextureColor ().getHandle ();
		GLuint pressPingTex = pressureSlab.ping_->getTextureColor ().getHandle ();
		GLuint divTex = divergenceFramebuffer.getTextureColor ().getHandle ();
		GLuint hiresObsTex = hiresObstaclesFramebuffer.getTextureColor ().getHandle ();

		// Advect velocity
		Advect ( g_AdvectHandle, g_QuadVAO, velPingTex, velPingTex, obsTex, velocitySlab.pong_.get (), g_VelocityDissipation );						
		SwapSurfaces ( &velocitySlab );
		velPingTex = velocitySlab.ping_->getTextureColor ().getHandle ();
		
		// Advect temperature
		Advect(g_AdvectHandle, g_QuadVAO, velPingTex, tempPingTex, obsTex, temperatureSlab.pong_.get (), g_TemperatureDissipation );
		SwapSurfaces ( &temperatureSlab );
		tempPingTex = temperatureSlab.ping_->getTextureColor ().getHandle ();

		// Advect density
		Advect(g_AdvectHandle, g_QuadVAO, velPingTex, densPingTex, obsTex, densitySlab.pong_.get(), g_DensityDissipation );
		SwapSurfaces ( &densitySlab );
		densPingTex = densitySlab.ping_->getTextureColor ().getHandle ();
		
		// Apply buoyancy
		ApplyBuoyancy ( g_BuoyancyHandle, g_QuadVAO, velPingTex, tempPingTex, densPingTex, velocitySlab.pong_.get () );
		SwapSurfaces ( &velocitySlab );
		velPingTex = velocitySlab.ping_->getTextureColor ().getHandle ();

		// Apply impulse
		ApplyImpulse ( g_SplatHandle, g_QuadVAO, g_ImpulsePosition, g_ImpulseTemperature, temperatureSlab.ping_.get () );
		ApplyImpulse ( g_SplatHandle, g_QuadVAO, g_ImpulsePosition, g_ImpulseDensity, densitySlab.ping_.get () );

		// Compute divergence
		ComputeDivergence ( g_ComputeDivergenceHandle, g_QuadVAO, velPingTex, obsTex, &divergenceFramebuffer );
		divTex = divergenceFramebuffer.getTextureColor ().getHandle ();
		
		// Clear pressure
		ClearSurface ( pressureSlab.ping_.get (), 0.0f );
		pressPingTex = pressureSlab.ping_->getTextureColor ().getHandle ();
		
		// Jacobi iterations
		for ( int i = 0; i < g_NumJacobiIterations; i++ )
		{
			Jacobi ( g_JacobiHandle, g_QuadVAO, pressPingTex, divTex, obsTex, pressureSlab.pong_.get () );
			SwapSurfaces ( &pressureSlab );
			pressPingTex = pressureSlab.ping_->getTextureColor ().getHandle ();
		}

		// Subtract Gradient
		SubtractGradient ( g_SubtractGradientHandle, g_QuadVAO, velPingTex, pressPingTex, obsTex, velocitySlab.pong_.get () );
		SwapSurfaces ( &velocitySlab );
		
		densPingTex = densitySlab.ping_->getTextureColor ().getHandle ();
		
		RenderQuad ( g_VisualizeHandle, g_QuadVAO, densPingTex, hiresObsTex );

		app.swapBuffers ();
	}

	glDeleteVertexArrays ( 1, &g_QuadVAO );
	
	velocitySlab.ping_ = nullptr;
	velocitySlab.pong_ = nullptr;

	densitySlab.ping_ = nullptr;
	densitySlab.pong_ = nullptr;
	
	temperatureSlab.ping_ = nullptr;
	temperatureSlab.pong_ = nullptr;

	pressureSlab.ping_ = nullptr;
	pressureSlab.pong_ = nullptr;

	return 0;	
}