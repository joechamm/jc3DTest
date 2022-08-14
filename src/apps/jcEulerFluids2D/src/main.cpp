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

constexpr uint32_t g_ViewportWidth = 480;
constexpr uint32_t g_ViewportHeight = 853;

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

struct PerFrameData
{
	vec3 fillColor = vec3(g_ImpulseTemperature);
	vec2 inverseSize = vec2(1.0f / g_GridWidth, 1.0f / g_GridHeight);
	vec2 scale = vec2(1.0f / g_ViewportWidth, 1.0f / g_ViewportHeight);
	vec2 point = vec2(0.0f);
	float timeStep = g_TimeStep;
	float dissipation = g_VelocityDissipation;
	float alpha = -g_CellSize * g_CellSize;
	float inverseBeta = 0.25f;
	float gradientScale = g_GradientScale;
	float halfInverseCellSize = 0.5f / g_CellSize;
	float radius = g_SplatRadius;
	float ambientTemperature = g_AmbientTemperature;
	float sigma = g_SmokeBuoyancy;
	float kappa = g_SmokeWeight;
} perFrameData;

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

//
//bool CreateObstacles ( uint32_t width, uint32_t height, GLProgram* fillProgram, GLFramebuffer* fillFramebuffer )
//{
//	if(fillProgram == nullptr || fillFramebuffer == nullptr)
//		return false;
//
//	glClearNamedFramebufferfv ( fillFramebuffer->getHandle (), GL_COLOR, 0, glm::value_ptr ( vec4(0.0f, 0.0f, 0.0f, 0.0f )) );
//
//	fillFramebuffer->bind ();
//
//	GLuint fillVAO;
//	glCreateVertexArrays ( 1, &fillVAO );
//	glEnableVertexArrayAttrib ( fillVAO, 0 );
//	glVertexArrayAttribFormat ( fillVAO, 0, 4, GL_FLOAT, GL_FALSE, 0 );
//	glVertexArrayAttribBinding ( fillVAO, 0, 0 );
//
//	if ( g_DrawBorder )
//	{		
//		const vec4 positions [ 5 ] = {
//			vec4 ( -g_BorderTrim, -g_BorderTrim, 0.0f, 1.0f ),
//			vec4 ( g_BorderTrim, -g_BorderTrim, 0.0f, 1.0f ),
//			vec4 ( g_BorderTrim, g_BorderTrim, 0.0f, 1.0f ),
//			vec4 ( -g_BorderTrim, g_BorderTrim, 0.0f, 1.0f ),
//			vec4 ( -g_BorderTrim, -g_BorderTrim, 0.0f, 1.0f )
//		};
//
//		const GLsizeiptr posBufSize = sizeof ( positions );
//		GLBuffer trimFillBuffer(posBufSize, glm::value_ptr(positions[0]), GL_STATIC_DRAW);
//		glVertexArrayVertexBuffer(fillVAO, 0, trimFillBuffer.getHandle(), 0, sizeof(vec4));
//		
//		glBindVertexArray ( fillVAO );
//		fillProgram->useProgram ();
//		glDrawArrays ( GL_LINE_STRIP, 0, 5 );		
//	}
//	
//	if ( g_DrawCircle )
//	{
//		const int slices = 64;
//		vec4 positions [ slices * 3 ];
//		float twopi = 8 * atanf ( 1.0f );
//		float theta = 0.0f;
//		float dtheta = twopi / ( float ) ( slices - 1 );		
//		for ( int i = 0; i < slices; i++ )
//		{
//			positions[i * 3 + 0] = vec4 ( 0.0f, 0.0f, 0.0f, 1.0f );
//			positions[i * 3 + 1] = vec4(0.25f * cosf(theta) * height / width, 0.25f * sinf(theta), 0.0f, 1.0f);
//			positions [ i * 3 + 2 ] = vec4 ( 0.25f * cosf ( theta ) * height / width, 0.25f * sinf ( theta ), 0.0f, 1.0f );		
//		}
//
//		const GLsizeiptr posBufSize = sizeof ( positions );
//		GLBuffer circleFillBuffer ( posBufSize, glm::value_ptr(positions[0]), GL_STATIC_DRAW);
//		glVertexArrayVertexBuffer(fillVAO, 0, circleFillBuffer.getHandle(), 0, sizeof(vec4));
//		
//		glBindVertexArray ( fillVAO );
//		fillProgram->useProgram ();
//		glDrawArrays ( GL_TRIANGLES, 0, slices * 3 );
//	}
//	
//	fillFramebuffer->unbind ();
//	glDeleteVertexArrays ( 1, &fillVAO );	
//	
//	return true;
//}

bool CreateObstacles ( GLuint prog, uint32_t width, uint32_t height, GLFramebuffer* dest )
{
	if(dest == nullptr)
		return false;

	glClearNamedFramebufferfv(dest->getHandle(), GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 0.0f)));
	dest->bind ();

	glUseProgram ( prog );

	GLuint vao;
	glGenVertexArrays ( 1, &vao );
	glBindVertexArray ( vao );

	if ( g_DrawBorder )
	{
		std::vector<vec4> positions = {
			vec4 ( -g_BorderTrim, -g_BorderTrim, 0.0f, 1.0f ),
			vec4 ( g_BorderTrim, -g_BorderTrim, 0.0f, 1.0f ),
			vec4 ( g_BorderTrim,  g_BorderTrim, 0.0f, 1.0f ),
			vec4 ( -g_BorderTrim,  g_BorderTrim, 0.0f, 1.0f ),
			vec4 ( -g_BorderTrim, -g_BorderTrim, 0.0f, 1.0f )
		};

		const GLsizeiptr posBufSize = sizeof ( vec4 ) * positions.size ();
		GLuint vbo;
		glGenBuffers ( 1, &vbo );
		glBindBuffer ( GL_ARRAY_BUFFER, vbo );
		glBufferData ( GL_ARRAY_BUFFER, posBufSize, positions.data (), GL_STATIC_DRAW );
		glEnableVertexAttribArray ( 0 );
		glVertexAttribPointer ( 0, 4, GL_FLOAT, GL_FALSE, sizeof ( vec4 ), nullptr );
		glDrawArrays ( GL_LINE_STRIP, 0, 5 );
		glDeleteBuffers ( 1, &vbo );
	}

	if ( g_DrawCircle )
	{
		const int slices = 64;
		std::vector<vec4> positions ( slices * 3 );
		float twopi = 2.0f * glm::pi<float> ();
		float theta = 0.0f;
		float dtheta = twopi / static_cast< float >( slices - 1 );
		for ( int i = 0; i < slices; i++ )
		{
			positions [ 3 * i + 0 ] = vec4 ( 0.0f, 0.0f, 0.0f, 1.0f );
			positions [ 3 * i + 1 ] = vec4 ( 0.25f * cosf ( theta ) * height / width, 0.25f * sinf ( theta ), 0.0f, 1.0f );
			theta += dtheta;
			positions [ 3 * i + 2 ] = vec4 ( 0.25f * cosf ( theta ) * height / width, 0.25f * sinf ( theta ), 0.0f, 1.0f );
		}

		const GLsizeiptr posBufSize = sizeof ( vec4 ) * positions.size ();
		GLuint vbo;
		glGenBuffers ( 1, &vbo );
		glBindBuffer ( GL_ARRAY_BUFFER, vbo );
		glBufferData ( GL_ARRAY_BUFFER, posBufSize, positions.data (), GL_STATIC_DRAW );
		glEnableVertexAttribArray ( 0 );
		glVertexAttribPointer ( 0, 4, GL_FLOAT, GL_FALSE, sizeof ( vec4 ), nullptr );
		glDrawArrays ( GL_TRIANGLES, 0, slices * 3 );
		glDeleteBuffers ( 1, &vbo );		
	}

	glDeleteVertexArrays ( 1, &vao );

	return true;
}

bool ClearSurface ( GLFramebuffer* surfaceFramebuffer, float v )
{
	if(surfaceFramebuffer == nullptr) return false;

	glClearNamedFramebufferfv ( surfaceFramebuffer->getHandle (), GL_COLOR, 0, glm::value_ptr ( vec4(v, v, v, v )) );

	return true;
}

void Advect ( GLuint prog, GLuint ubo, GLuint velocityTex, GLuint sourceTex, GLuint obstaclesTex, GLFramebuffer* destFramebuffer, float dissipation )
{
	perFrameData.dissipation = dissipation;
	glNamedBufferSubData ( ubo, 0, sizeof ( perFrameData ), &perFrameData );	

	glUseProgram ( prog );

	destFramebuffer->bind ();
	glBindTextureUnit ( 5, velocityTex );
	glBindTextureUnit ( 6, sourceTex );
	glBindTextureUnit ( 7, obstaclesTex );

	// we assume that the global vertex array has been bound here
	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr );
	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void Jacobi ( GLuint prog, GLuint pressureTex, GLuint divergenceTex, GLuint obstaclesTex, GLFramebuffer* destFramebuffer )
{
	glUseProgram ( prog );
	
	destFramebuffer->bind ();
	glBindTextureUnit ( 8, pressureTex );
	glBindTextureUnit ( 9, divergenceTex );
	glBindTextureUnit ( 7, obstaclesTex );

	// we assume that the global vertex array has been bound here
	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr );
	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void SubtractGradient ( GLuint prog, GLuint velocityTex, GLuint pressureTex, GLuint obstaclesTex, GLFramebuffer* destFramebuffer )
{
	glUseProgram ( prog );
	
	destFramebuffer->bind ();
	glBindTextureUnit ( 5, velocityTex );
	glBindTextureUnit ( 8, pressureTex );
	glBindTextureUnit ( 7, obstaclesTex );

	// we assume that the global vertex array has been bound here
	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr );
	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void ComputeDivergence(GLuint prog, GLuint velocityTex, GLuint obstaclesTex, GLFramebuffer* destFramebuffer)
{
	glUseProgram ( prog );
	
	destFramebuffer->bind ();
	glBindTextureUnit ( 5, velocityTex );
	glBindTextureUnit ( 7, obstaclesTex );

	// we assume that the global vertex array has been bound here
	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr );
	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void ApplyImpulse ( GLuint prog, GLuint ubo, const vec2& position, float value, GLFramebuffer* destFramebuffer )
{
	perFrameData.point = position;
	perFrameData.fillColor = vec3 ( value, value, value );

	glNamedBufferSubData(ubo, 0, sizeof(perFrameData), &perFrameData);

	glUseProgram ( prog );

	destFramebuffer->bind ();
	glEnable ( GL_BLEND );
	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr );
	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );	
}

void ApplyBuoyancy(GLuint prog, GLuint velocityTex, GLuint temperatureTex, GLuint densityTex, GLFramebuffer* destFramebuffer)
{
	glUseProgram ( prog );
	
	destFramebuffer->bind ();
	glBindTextureUnit ( 5, velocityTex );
	glBindTextureUnit ( 10, temperatureTex );
	glBindTextureUnit ( 11, densityTex );

	// we assume that the global vertex array has been bound here
	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr );
	destFramebuffer->unbind ();
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
			if ( key == GLFW_KEY_ESCAPE && pressed )
				glfwSetWindowShouldClose ( window, GLFW_TRUE );
		}
	);
	
	GLShader shdVert ( appendToRoot ( "assets/shaders/FluidRender.vert" ).c_str () );
	GLShader shdFrag ( appendToRoot ( "assets/shaders/FluidRender.frag" ).c_str () );
	GLProgram progRender ( shdVert, shdFrag );

	GLShader shdAdvect ( appendToRoot ( "assets/shaders/FluidAdvect.frag" ).c_str () );
	GLProgram progAdvect ( shdVert, shdAdvect );

	GLShader shdDivergence ( appendToRoot ( "assets/shaders/FluidDivergence.frag" ).c_str () );
	GLProgram progDivergence ( shdVert, shdDivergence );

	GLShader shdGradient ( appendToRoot ( "assets/shaders/FluidSubtractGradient.frag" ).c_str () );
	GLProgram progGradient ( shdVert, shdGradient );

	GLShader shdJacobi( appendToRoot ( "assets/shaders/FluidJacobi.frag" ).c_str () );
	GLProgram progJacobi ( shdVert, shdJacobi );
	
	GLShader shdImpulse ( appendToRoot ( "assets/shaders/FluidImpulse.frag" ).c_str () );
	GLProgram progImpulse ( shdVert, shdImpulse );

	GLShader shdBouyancy ( appendToRoot ( "assets/shaders/FluidBouyancy.frag" ).c_str () );
	GLProgram progBouyancy ( shdVert, shdBouyancy );

	GLShader shdSetup ( appendToRoot ( "assets/shaders/FluidSetup.frag" ).c_str () );
	GLProgram progSetup ( shdVert, shdSetup );

	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );
	
	GLBuffer perFrameDataBuffer ( kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferRange ( GL_UNIFORM_BUFFER, 0, perFrameDataBuffer.getHandle (), 0, kUniformBufferSize );

	Slab velocitySlab = CreateSlab ( g_GridWidth, g_GridHeight, 2 );
	Slab densitySlab = CreateSlab ( g_GridWidth, g_GridHeight, 1 );
	Slab pressureSlab = CreateSlab ( g_GridWidth, g_GridHeight, 1 );
	Slab temperatureSlab = CreateSlab ( g_GridWidth, g_GridHeight, 1 );
	
	GLFramebuffer divergenceFramebuffer ( g_GridWidth, g_GridHeight, g_UseHalfFloats ? GL_RGB16F : GL_RGB32F, 0 );
	GLFramebuffer obstaclesFramebuffer ( g_GridWidth, g_GridHeight, g_UseHalfFloats ? GL_RGB16F : GL_RGB32F, 0 );

	CreateObstacles ( progSetup.getHandle(), g_GridWidth, g_GridHeight, &obstaclesFramebuffer );

	GLFramebuffer hiresObstaclesFramebuffer ( g_HiresViewportWidth, g_HiresViewportHeight, GL_R16F, 0 );

	CreateObstacles ( progSetup.getHandle(), g_HiresViewportWidth, g_HiresViewportHeight, &hiresObstaclesFramebuffer );

	ClearSurface ( temperatureSlab.ping_.get(), g_AmbientTemperature);

	std::vector<vec4> squarePositions = {
		vec4 ( -1.0f, -1.0f, 0.0f, 1.0f ),
		vec4 ( 1.0f, -1.0f, 0.0f, 1.0f ),
		vec4 ( -1.0f, 1.0f, 0.0f, 1.0f ),
		vec4 ( 1.0f, 1.0f, 0.0f, 1.0f )
	};

	std::vector<GLuint> indices = {
		0, 1, 3, 3, 2, 0
	};
	
	const GLsizeiptr sqPosBufSize = squarePositions.size() * sizeof (vec4);
	const GLsizeiptr sqIndBufSize = indices.size() * sizeof (GLuint);

	GLBuffer squarePositionsBuffer ( sqPosBufSize, squarePositions.data(), 0);
	GLBuffer squareIndicesBuffer ( sqIndBufSize, indices.data(), 0);
			
	GLuint vao;
	glCreateVertexArrays ( 1, &vao );

	glBindVertexArray ( vao );
	glVertexArrayElementBuffer ( vao, squareIndicesBuffer.getHandle () );

	glVertexArrayVertexBuffer ( vao, 0, squarePositionsBuffer.getHandle (), 0, 4 * sizeof ( vec4 ) );
	
	glEnableVertexArrayAttrib ( vao, 0 );
	glVertexArrayAttribFormat ( vao, 0, 4, GL_FLOAT, GL_FALSE, 0 );
	glVertexArrayAttribBinding ( vao, 0, 0 );

	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	while ( !glfwWindowShouldClose ( app.getWindow () ) )
	{
		// Update

		if ( mouseState.pressedLeft )
		{
			perFrameData.point = mouseState.pos;
			glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, sizeof ( perFrameData ), &perFrameData );
		}
		
		glViewport ( 0, 0, g_GridWidth, g_GridHeight );

		glBindVertexArray ( vao );
				
		Advect ( progAdvect.getHandle (), perFrameDataBuffer.getHandle (),
			velocitySlab.ping_->getTextureColor ().getHandle (),
			velocitySlab.ping_->getTextureColor ().getHandle (),
			obstaclesFramebuffer.getTextureColor ().getHandle (),
			velocitySlab.pong_.get (),
			g_VelocityDissipation );
		SwapSurfaces ( &velocitySlab );
		
		Advect ( progAdvect.getHandle (), perFrameDataBuffer.getHandle (),
			velocitySlab.ping_->getTextureColor ().getHandle (),
			temperatureSlab.ping_->getTextureColor ().getHandle (),
			obstaclesFramebuffer.getTextureColor ().getHandle (),
			temperatureSlab.pong_.get (),
			g_TemperatureDissipation );
		SwapSurfaces ( &temperatureSlab );

		Advect ( progAdvect.getHandle (), perFrameDataBuffer.getHandle (),
			velocitySlab.ping_->getTextureColor().getHandle(),
			densitySlab.ping_->getTextureColor ().getHandle (),
			obstaclesFramebuffer.getTextureColor ().getHandle (),
			densitySlab.pong_.get (),
			g_DensityDissipation );
		SwapSurfaces ( &densitySlab );

		ApplyBuoyancy ( progBouyancy.getHandle (), velocitySlab.ping_->getTextureColor ().getHandle (),
			temperatureSlab.ping_->getTextureColor ().getHandle (),
			densitySlab.ping_->getTextureColor ().getHandle (),
			velocitySlab.pong_.get () );
		SwapSurfaces ( &velocitySlab );

		ApplyImpulse ( progImpulse.getHandle (), perFrameDataBuffer.getHandle (),
			g_ImpulsePosition,
			g_ImpulseTemperature,
			temperatureSlab.ping_.get () );
		ApplyImpulse ( progImpulse.getHandle (), perFrameDataBuffer.getHandle (),
			g_ImpulsePosition,
			g_ImpulseDensity,
			densitySlab.ping_.get () );

		ComputeDivergence ( progDivergence.getHandle (), velocitySlab.ping_->getTextureColor ().getHandle (), obstaclesFramebuffer.getTextureColor ().getHandle (), &divergenceFramebuffer );
		ClearSurface ( pressureSlab.ping_.get (), 0.0f );

		for ( int i = 0; i < g_NumJacobiIterations; ++i )
		{
			Jacobi(progJacobi.getHandle(), pressureSlab.ping_->getTextureColor().getHandle(), divergenceFramebuffer.getTextureColor().getHandle(), obstaclesFramebuffer.getTextureColor().getHandle(), pressureSlab.pong_.get());
			SwapSurfaces ( &pressureSlab );
		}

		SubtractGradient(progGradient.getHandle(), velocitySlab.ping_->getTextureColor().getHandle(), pressureSlab.ping_->getTextureColor().getHandle(), obstaclesFramebuffer.getTextureColor().getHandle(), velocitySlab.pong_.get());
		SwapSurfaces ( &velocitySlab );		
		
		// Render

		glViewport ( 0, 0, g_ViewportWidth, g_ViewportHeight );
		progRender.useProgram ();
		glEnable ( GL_BLEND );
		glBindFramebuffer ( GL_FRAMEBUFFER, 0 );
		glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );
		glClear ( GL_COLOR_BUFFER_BIT );

		// draw ink
		glBindTextureUnit ( 12, densitySlab.ping_->getTextureColor ().getHandle () );

		perFrameData.fillColor = vec3 ( 1.0f, 1.0f, 1.0f );
		perFrameData.scale = vec2 ( 1.0f / g_ViewportWidth, 1.0f / g_ViewportHeight );
		glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, sizeof ( perFrameData ), &perFrameData );
		glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr );

		glBindTextureUnit ( 12, hiresObstaclesFramebuffer.getTextureColor ().getHandle () );
		perFrameData.fillColor = vec3 ( 0.125f, 0.4f, 0.75f );
		glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, sizeof ( perFrameData ), &perFrameData );
		glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr );

		glDisable ( GL_BLEND );		

		app.swapBuffers ();
	}

	glDeleteVertexArrays ( 1, &vao );
	
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