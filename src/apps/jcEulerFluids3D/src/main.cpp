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
#include <jcCommonFramework/UtilsFPS.h>
#include <jcCommonFramework/ResourceString.h>

#include <jcCommonFramework/MousePole.h>
#include <jcCommonFramework/ObjectPole.h>

using glm::mat4;
using glm::mat3;
using glm::mat2;
using glm::vec4;
using glm::vec3;
using glm::vec2;

bool g_DrawEyeball = false;

constexpr uint32_t g_ViewportWidth = 1024;
constexpr uint32_t g_ViewportHeight = 1024;

constexpr uint32_t g_GridWidth = 64;
constexpr uint32_t g_GridHeight = 64;
constexpr uint32_t g_GridDepth = 64;

constexpr GLuint kIndexTexBindVelocity = 0;
constexpr GLuint kIndexTexBindSourceTexture = 1;
constexpr GLuint kIndexTexBindObstacles = 2;
constexpr GLuint kIndexTexBindPressure = 3;
constexpr GLuint kIndexTexBindTemperature = 4;
constexpr GLuint kIndexTexBindDensity = 5;
constexpr GLuint kIndexTexBindDivergence = 6;
constexpr GLuint kIndexTexBindLightCache = 7;

// Fluid3D.vert
constexpr GLint kIndexLocAttribPosition = 0;

// Raycast3D.vert
//constexpr GLint kIndexLocAttribPosition = 0;
constexpr GLint kIndexLocUniformModelViewProjection = 10;

// Raycast3D.geom
constexpr GLint kIndexLocUniformProjectionMatrix = 11;
constexpr GLint kIndexLocUniformViewMatrix = 12;
constexpr GLint kIndexLocUniformModelMatrix = 13;

// Raycast3D.frag
constexpr GLint kIndexLocUniformLightPosition = 0;
constexpr GLint kIndexLocUniformLightIntensity = 1;
constexpr GLint kIndexLocUniformAbsorption = 2;
constexpr GLint kIndexLocUniformModelView = 3;
constexpr GLint kIndexLocUniformFocalLength = 4;
constexpr GLint kIndexLocUniformWindowSize = 5;
constexpr GLint kIndexLocUniformRayOrigin = 6;
constexpr GLint kIndexLocUniformAmbient = 7;
constexpr GLint kIndexLocUniformStepSize = 8;
constexpr GLint kIndexLocUniformViewSamples = 9;

// LightCache3D.frag
//constexpr GLint kIndexLocUniformLightPosition = 0;
//constexpr GLint kIndexLocUniformLightIntensity= 1;
//constexpr GLint kIndexLocUniformAbsorption = 2;
constexpr GLint kIndexLocUniformLightStep = 3;
constexpr GLint kIndexLocUniformLightSamples = 4;
constexpr GLint kIndexLocUniformInverseSize = 5;

// Advect3D.frag
constexpr GLint kIndexLocUniformTimeStep = 0;
constexpr GLint kIndexLocUniformDissipation = 1;
//constexpr GLint kIndexLocUniformInverseSize = 2;

// SubtractGradient3D.frag
constexpr GLint kIndexLocUniformGradientScale = 0;

// Splat3D.frag
constexpr GLint kIndexLocUniformRadius = 0;
constexpr GLint kIndexLocUniformFillColor = 1;
constexpr GLint kIndexLocUniformPoint = 2;

// LightBlur3D.frag
//constexpr GLint kIndexLocUniformStepSize = 0;
constexpr GLint kIndexLocUniformDensityScale = 1;
//constexpr GLint kIndexLocUniformInverseSize = 2;

// Jacobi3D.frag
constexpr GLint kIndexLocUniformAlpha = 0;
constexpr GLint kIndexLocUniformInverseBeta = 1;

// ComputeDivergence3D.frag
constexpr GLint kIndexLocUniformHalfInverseSize = 0;

// Buoyancy3D.frag
//constexpr GLint kIndexLocUniformTimeStep = 0; // already used in Advect3D.frag
constexpr GLint kIndexLocUniformAmbientTemperature = 1;
constexpr GLint kIndexLocUniformSigma = 2;
constexpr GLint kIndexLocUniformKappa = 3;

int32_t g_NumJacobiIterations = 40;
int32_t g_ViewSamples = g_GridWidth * 2;
int32_t g_LightSamples = g_GridWidth;

constexpr bool g_UseHalfFloats = true;

constexpr bool g_DrawBorder = true;
constexpr bool g_DrawCircle = true;
constexpr bool g_DrawSphere = true;

bool g_BlurAndBrighten = true;
bool g_CacheLights = true;

constexpr float g_BorderTrim = 0.9999f;
constexpr float g_DefaultThetaX = 0.0f;
constexpr float g_DefaultThetaY = 0.75f;

constexpr vec3 g_EyePosition = vec3 ( 0.0f, 0.0f, 2.0f );

float g_SplatRadius = ( float ) g_GridWidth / 8.0f;
float g_CellSize = 1.25f;
float g_TimeStep = 0.25f;
float g_AmbientTemperature = 0.0f;
float g_ImpulseTemperature = 10.0f;
float g_ImpulseDensity = 1.25f;
float g_SmokeBuoyancy = 1.0f;
float g_SmokeWeight = 0.0f;
float g_GradientScale = 1.125f / g_CellSize;
float g_TemperatureDissipation = 0.99f;
float g_VelocityDissipation = 0.99f;
float g_DensityDissipation = 0.999f;
float g_FieldOfView = 0.7f;
float g_ThetaX = g_DefaultThetaX;
float g_ThetaY = g_DefaultThetaY;
float g_Fips = -4.0f;

vec3 g_ImpulsePosition = vec3 ( g_GridWidth / 2.0f, (float)g_GridHeight - g_SplatRadius / 2.0f, (float)g_GridDepth / 2.0f );

mat4 g_ProjectionMatrix;
mat4 g_ModelViewMatrix;
mat4 g_ViewMatrix;
mat4 g_ModelViewProjection;

bool g_DisplayTemp = false;
bool g_DisplayVelocity = false;
bool g_SimulateFluid = true;

GLuint g_RaycastHandle = 0;
GLuint g_LightHandle = 0;
GLuint g_BlurHandle = 0;
GLuint g_AdvectHandle = 0;
GLuint g_BuoyancyHandle = 0;
GLuint g_ComputeDivergenceHandle = 0;
GLuint g_FillHandle = 0;
GLuint g_JacobiHandle = 0;
GLuint g_SubtractGradientHandle = 0;
GLuint g_SplatHandle = 0;

GLuint g_QuadVAO = 0;
GLuint g_CubeCenterVAO = 0;
GLuint g_PointVBO = 0;
GLuint g_QuadVBO = 0;

struct MouseState
{
	vec2 pos = vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

MousePole* g_pMousePole = nullptr;
ObjectPole* g_pObjectPole = nullptr;

int getNumMipMapLevels3D ( int w, int h, int d )
{
	int levels = 1;
	while ( ( w | h | d ) >> levels )
		levels += 1;
	return levels;
}

class GLTexture3D
{
	GLuint handle_ = 0;
	GLuint handleBindless_ = 0;

public:
	GLTexture3D ( GLint width, GLint height, GLint depth, GLenum internalFormat )
	{
		glCreateTextures ( GL_TEXTURE_3D, 1, &handle_ );
		glTextureParameteri ( handle_, GL_TEXTURE_MAX_LEVEL, 0 );
		glTextureParameteri ( handle_, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTextureParameteri ( handle_, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTextureStorage3D ( handle_, getNumMipMapLevels3D ( width, height, depth ), internalFormat, width, height, depth );
	}
	~GLTexture3D ()
	{
		if ( handleBindless_ )
			glMakeTextureHandleNonResidentARB ( handleBindless_ );

		glDeleteTextures ( 1, &handle_ );
	}
	GLTexture3D ( const GLTexture3D& ) = delete;
	GLTexture3D ( GLTexture3D&& other) :
		handle_(other.handle_)
		, handleBindless_(other.handleBindless_)
	{
		other.handle_ = 0;
		other.handleBindless_ = 0;
	}

	GLuint getHandle () const
	{
		return handle_;
	}

	GLuint64 getHandleBindless () const
	{
		return handleBindless_;
	}
};

class GLFramebuffer3D
{
	GLint width_;
	GLint height_;
	GLint depth_;
	GLuint handle_ = 0;

	std::unique_ptr<GLTexture3D> texColor_;
	std::unique_ptr<GLTexture> texDepth_;

public:
	GLFramebuffer3D ( GLint width, GLint height, GLint depth, GLenum formatColor, GLenum formatDepth )
		: width_ ( width )
		, height_ ( height )
		, depth_ ( depth )
	{
		glCreateFramebuffers ( 1, &handle_ );

		if ( formatColor )
		{
			texColor_ = std::make_unique<GLTexture3D> ( width_, height_, depth_, formatColor );
			glTextureParameteri ( texColor_->getHandle (), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTextureParameteri ( texColor_->getHandle (), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			glTextureParameteri ( texColor_->getHandle (), GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
			glNamedFramebufferTexture ( handle_, GL_COLOR_ATTACHMENT0, texColor_->getHandle (), 0 );
		}

		if ( formatDepth )
		{
			texDepth_ = std::make_unique<GLTexture> ( GL_TEXTURE_2D, width, height, formatDepth );
			const GLfloat border [] = { 0.0f, 0.0f, 0.0f, 0.0f };
			glTextureParameterfv ( texDepth_->getHandle (), GL_TEXTURE_BORDER_COLOR, border );
			glTextureParameteri ( texDepth_->getHandle (), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
			glTextureParameteri ( texDepth_->getHandle (), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
			glNamedFramebufferTexture ( handle_, GL_DEPTH_ATTACHMENT, texDepth_->getHandle (), 0 );
		}

		const GLenum status = glCheckNamedFramebufferStatus ( handle_, GL_FRAMEBUFFER );

		assert ( status == GL_FRAMEBUFFER_COMPLETE );
	}

	~GLFramebuffer3D ()
	{
		glBindFramebuffer ( GL_FRAMEBUFFER, 0 );
		glDeleteFramebuffers ( 1, &handle_ );
	}

	GLFramebuffer3D ( const GLFramebuffer3D& ) = delete;
	GLFramebuffer3D ( GLFramebuffer3D&& ) = default;

	void bind ()
	{
		glBindFramebuffer ( GL_FRAMEBUFFER, handle_ );
		glViewport ( 0, 0, width_, height_ );
	}

	void unbind ()
	{
		glBindFramebuffer ( GL_FRAMEBUFFER, 0 );
	}

	GLuint getHandle () const
	{
		return handle_;
	}

	const GLTexture3D& getTextureColor () const
	{
		return *texColor_.get ();
	}

	const GLTexture& getTextureDepth () const
	{
		return *texDepth_.get ();
	}

	const GLuint colorTex () const
	{
		return texColor_->getHandle ();
	}

	const GLuint depthTex () const
	{
		return texDepth_->getHandle ();
	}

	const GLint width () const
	{
		return width_;
	}

	const GLint height () const
	{
		return height_;
	}

	const GLint depth () const
	{
		return depth_;
	}
};

struct Slab
{
	std::unique_ptr<GLFramebuffer> ping_;
	std::unique_ptr<GLFramebuffer> pong_;
};

struct SlabPod
{
	std::unique_ptr<GLFramebuffer3D> ping_;
	std::unique_ptr<GLFramebuffer3D> pong_;
};

GLuint CreatePointVbo ( float x, float y, float z )
{
	float p [ 3 ] = { x, y, z };
	GLuint vbo;
	glGenBuffers ( 1, &vbo );
	glBindBuffer ( GL_ARRAY_BUFFER, vbo );
	glBufferData ( GL_ARRAY_BUFFER, sizeof ( p ), &p [ 0 ], GL_STATIC_DRAW );
	return vbo;
}

GLuint CreateQuadVbo ()
{
	short positions [] = {
		-1, -1,
		 1, -1,
		-1,  1,
		 1,  1
	};

	// Create the vbo 
	GLuint vbo;
	GLsizeiptr size = sizeof ( positions );
	glGenBuffers ( 1, &vbo );
	glBindBuffer ( GL_ARRAY_BUFFER, vbo );
	glBufferData ( GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW );
	return vbo;
}

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

SlabPod CreateVolume ( uint32_t width, uint32_t height, uint32_t depth, uint32_t numComponents )
{
	SlabPod slabPod;

	if ( g_UseHalfFloats )
	{
		switch ( numComponents )
		{
		case 1:
			slabPod.ping_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_R16F, 0 );
			slabPod.pong_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_R16F, 0 );
			break;
		case 2:
			slabPod.ping_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_RG16F, 0 );
			slabPod.pong_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_RG16F, 0 );
			break;
		case 3:
			slabPod.ping_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_RGB16F, 0 );
			slabPod.pong_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_RGB16F, 0 );
			break;
		case 4:
			slabPod.ping_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_RGBA16F, 0 );
			slabPod.pong_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_RGBA16F, 0 );
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
			slabPod.ping_ = std::make_unique<GLFramebuffer3D> ( width, height,	depth, GL_R32F, 0 );
			slabPod.pong_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_R32F, 0 );
			break;
		case 2:
			slabPod.ping_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_RG32F, 0 );
			slabPod.pong_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_RG32F, 0 );
			break;
		case 3:
			slabPod.ping_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_RGB32F, 0 );
			slabPod.pong_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_RGB32F, 0 );
			break;
		case 4:
			slabPod.ping_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_RGBA32F, 0 );
			slabPod.pong_ = std::make_unique<GLFramebuffer3D> ( width, height, depth, GL_RGBA32F, 0 );
			break;
		default:
			throw std::runtime_error ( "Invalid Framebuffer" );
			break;
		}
	}

	return slabPod;
}

void SwapSurfaces ( Slab* slab )
{
	std::swap ( slab->ping_, slab->pong_ );
}

void SwapPods ( SlabPod* slabPod )
{
	std::swap ( slabPod->ping_, slabPod->pong_ );
}

bool CreateObstacles3D ( GLuint prog, GLFramebuffer3D* dest )
{
	if ( dest == nullptr )
		return false;

	const uint32_t width = dest->width ();
	const uint32_t height = dest->height ();
	const uint32_t depth = dest->depth ();

	glBindFramebuffer ( GL_FRAMEBUFFER, dest->getHandle () );
	glViewport ( 0, 0, width, height );
	glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear ( GL_COLOR_BUFFER_BIT );

	glUseProgram ( prog );

	GLuint vao;
	glGenVertexArrays ( 1, &vao );
	glBindVertexArray ( vao );

	GLuint lineVbo;
	glGenBuffers ( 1, &lineVbo );
	GLuint circleVbo;
	glGenBuffers ( 1, &circleVbo );

	glEnableVertexAttribArray ( kIndexLocAttribPosition );

	for ( int slice = 0; slice < depth; ++slice )
	{
		glFramebufferTextureLayer ( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dest->colorTex (), 0, depth - 1 - slice );
		float z = ( float ) depth / 2.0f;
		z = std::abs ( ( float ) slice - z ) / z;
		float fraction = 1.0f - sqrtf ( z );
		float radius = 0.5f * fraction;

		if ( slice == 0 || slice == depth - 1 )
		{
			radius *= 100.0f;
		}

		if ( g_DrawBorder && slice != 0 && slice != depth - 1 )
		{
#define T 0.9999f
			float positions [] = { -T, -T, T, -T, T, T, -T, T, -T, -T };
#undef T
			GLsizeiptr size = sizeof ( positions );
			glBindBuffer ( GL_ARRAY_BUFFER, lineVbo );
			glBufferData ( GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW );
			GLsizeiptr stride = 2 * sizeof ( positions [ 0 ] );
			glVertexAttribPointer ( kIndexLocAttribPosition, 2, GL_FLOAT, GL_FALSE, stride, nullptr );
			glDrawArrays ( GL_LINE_STRIP, 0, 5 );
		}

		if ( g_DrawSphere || slice == 0 || slice == depth - 1 )
		{
			const int slices = 64;
			float positions [ slices * 2 * 3 ];
			float twopi = 8.0f * atanf ( 1.0f );
			float theta = 0.0f;
			float dtheta = twopi / ( float ) ( slices - 1 );
			float* pPositions = &positions [ 0 ];
			for ( int i = 0; i < slices; i++ )
			{
				*pPositions++ = 0.0f;
				*pPositions++ = 0.0f;

				*pPositions++ = radius * cosf ( theta );
				*pPositions++ = radius * sinf ( theta );
				theta += dtheta;

				*pPositions++ = radius * cosf ( theta );
				*pPositions++ = radius * sinf ( theta );
			}

			GLsizeiptr size = sizeof ( positions );
			glBindBuffer ( GL_ARRAY_BUFFER, circleVbo );
			glBufferData ( GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW );
			GLsizeiptr stride = 2 * sizeof ( positions [ 0 ] );
			glVertexAttribPointer ( kIndexLocAttribPosition, 2, GL_FLOAT, GL_FALSE, stride, nullptr );
			glDrawArrays ( GL_TRIANGLES, 0, slices * 3 );
		}
	}

	// cleanup
	glDeleteVertexArrays ( 1, &vao );
	glDeleteBuffers ( 1, &lineVbo );
	glDeleteBuffers ( 1, &circleVbo );

	return true;
}

bool ClearVolume ( GLFramebuffer3D* volumeFramebuffer, float v )
{
	if ( volumeFramebuffer == nullptr ) return false;
	glBindFramebuffer ( GL_FRAMEBUFFER, volumeFramebuffer->getHandle () );
	glClearColor ( v, v, v, v );
	glClear ( GL_COLOR_BUFFER_BIT );
	return true;
}

void Advect3D ( GLuint prog, GLuint vao, GLuint velocityTex, GLuint sourceTex, GLuint obstaclesTex, GLFramebuffer3D* destFramebuffer, float dissipation)
{
	glBindVertexArray ( vao );

	glUseProgram ( prog );

	glUniform3f ( kIndexLocUniformInverseSize, 1.0f / ( float ) g_GridWidth, 1.0f / ( float ) g_GridHeight, 1.0f / ( float ) g_GridDepth );
	glUniform1f ( kIndexLocUniformTimeStep, g_TimeStep );
	glUniform1f ( kIndexLocUniformDissipation, dissipation );

	destFramebuffer->bind ();
	glBindTextureUnit ( kIndexTexBindVelocity, velocityTex );
	glBindTextureUnit ( kIndexTexBindSourceTexture, sourceTex );
	glBindTextureUnit ( kIndexTexBindObstacles, obstaclesTex );

	glDrawArraysInstanced ( GL_TRIANGLE_STRIP, 0, 4, destFramebuffer->depth() );

	glBindTextureUnit ( kIndexTexBindVelocity, 0 );
	glBindTextureUnit ( kIndexTexBindSourceTexture, 0 );
	glBindTextureUnit ( kIndexTexBindObstacles, 0 );
	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void Jacobi3D ( GLuint prog, GLuint vao, GLuint pressureTex, GLuint divergenceTex, GLuint obstaclesTex, GLFramebuffer3D* destFramebuffer )
{
	glBindVertexArray ( vao );

	glUseProgram ( prog );

	glUniform1f ( kIndexLocUniformAlpha, -g_CellSize * g_CellSize ); // alpha
	glUniform1f ( kIndexLocUniformInverseBeta, 0.1666f); // inverse beta

	destFramebuffer->bind ();
	glBindTextureUnit ( kIndexTexBindPressure, pressureTex );
	glBindTextureUnit ( kIndexTexBindDivergence, divergenceTex );
	glBindTextureUnit ( kIndexTexBindObstacles, obstaclesTex );

	glDrawArraysInstanced ( GL_TRIANGLE_STRIP, 0, 4, destFramebuffer->depth () );

	glBindTextureUnit ( kIndexTexBindPressure, 0 );
	glBindTextureUnit ( kIndexTexBindDivergence, 0 );
	glBindTextureUnit ( kIndexTexBindObstacles, 0 );
	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void SubtractGradient3D ( GLuint prog, GLuint vao, GLuint velocityTex, GLuint pressureTex, GLuint obstaclesTex, GLFramebuffer3D* destFramebuffer )
{
	glBindVertexArray ( vao );

	glUseProgram ( prog );

	glUniform1f ( kIndexLocUniformGradientScale, g_GradientScale ); // gradient scale

	destFramebuffer->bind ();
	glBindTextureUnit (kIndexTexBindVelocity, velocityTex );
	glBindTextureUnit ( kIndexTexBindPressure, pressureTex );
	glBindTextureUnit ( kIndexTexBindObstacles, obstaclesTex );

	glDrawArraysInstanced ( GL_TRIANGLE_STRIP, 0, 4, destFramebuffer->depth () );

	glBindTextureUnit ( kIndexTexBindVelocity, 0 );
	glBindTextureUnit ( kIndexTexBindPressure, 0 );
	glBindTextureUnit ( kIndexTexBindObstacles, 0 );

	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void ComputeDivergence3D ( GLuint prog, GLuint vao, GLuint velocityTex, GLuint obstaclesTex, GLFramebuffer3D* destFramebuffer )
{
	glBindVertexArray ( vao );

	glUseProgram ( prog );

	glUniform1f ( kIndexLocUniformHalfInverseSize, 0.5f / g_CellSize ); // half inverse cell size

	destFramebuffer->bind ();
	glBindTextureUnit ( kIndexTexBindVelocity, velocityTex );
	glBindTextureUnit ( kIndexTexBindObstacles, obstaclesTex );

	glDrawArraysInstanced ( GL_TRIANGLE_STRIP, 0, 4, destFramebuffer->depth() );
	glBindTextureUnit ( kIndexTexBindVelocity, 0 );
	glBindTextureUnit ( kIndexTexBindObstacles, 0 );

	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void ApplyImpulse3D ( GLuint prog, GLuint vao, const vec3& position, float value, GLFramebuffer3D* destFramebuffer )
{
	glBindVertexArray ( vao );

	glUseProgram ( prog );

	glUniform3f ( kIndexLocUniformPoint, position.x, position.y, position.z ); // Point
	glUniform1f ( kIndexLocUniformRadius, g_SplatRadius ); // Radius
	glUniform3f ( kIndexLocUniformFillColor, value, value, value );  // Fill Color

	destFramebuffer->bind ();
	glEnable ( GL_BLEND );

	glDrawArraysInstanced ( GL_TRIANGLE_STRIP, 0, 4, destFramebuffer->depth() );

	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

void ApplyBuoyancy3D ( GLuint prog, GLuint vao, GLuint velocityTex, GLuint temperatureTex, GLuint densityTex, GLFramebuffer3D* destFramebuffer )
{
	glBindVertexArray ( vao );

	glUseProgram ( prog );

	glUniform1f ( kIndexLocUniformAmbientTemperature, g_AmbientTemperature ); // Ambient Temperature
	glUniform1f ( kIndexLocUniformTimeStep, g_TimeStep ); // Time Step
	glUniform1f ( kIndexLocUniformSigma, g_SmokeBuoyancy ); // Sigma
	glUniform1f ( kIndexLocUniformKappa, g_SmokeWeight ); // Kappa 

	destFramebuffer->bind ();
	glBindTextureUnit ( kIndexTexBindVelocity, velocityTex );
	glBindTextureUnit ( kIndexTexBindTemperature, temperatureTex );
	glBindTextureUnit ( kIndexTexBindDensity, densityTex );

	glDrawArraysInstanced ( GL_TRIANGLE_STRIP, 0, 4, destFramebuffer->depth() );

	glBindTextureUnit ( kIndexTexBindVelocity, 0 );
	glBindTextureUnit ( kIndexTexBindTemperature, 0 );
	glBindTextureUnit ( kIndexTexBindDensity, 0 );

	destFramebuffer->unbind ();
	glDisable ( GL_BLEND );
}

bool BlurAndBrighten ( GLuint prog, GLuint vao, GLuint densityTex, GLFramebuffer3D* dest )
{
	if ( nullptr == dest ) return false;

	const uint32_t width = dest->width ();
	const uint32_t height = dest->height ();
	const uint32_t depth = dest->depth ();

	glDisable ( GL_BLEND );

	glBindVertexArray ( vao );

	glBindFramebuffer ( GL_FRAMEBUFFER, dest->getHandle () );
	glViewport ( 0, 0, width, height );
	
	glUseProgram ( prog );

	glUniform1f ( kIndexLocUniformDensityScale, 5.0f );
	glUniform1f ( kIndexLocUniformStepSize, sqrtf ( 2.0f ) / ( float ) g_ViewSamples );
	glUniform3f ( kIndexLocUniformInverseSize, 1.0f / (float)width, 1.0f / (float)height, 1.0f / (float)depth);

	glBindTextureUnit ( kIndexTexBindDensity, densityTex );

	glDrawArraysInstanced ( GL_TRIANGLE_STRIP, 0, 4, depth );

	glBindTextureUnit ( kIndexTexBindDensity, 0 );

	glBindFramebuffer ( GL_FRAMEBUFFER, 0 );

	return true;
}

bool CacheLights ( GLuint prog, GLuint vao, GLuint blurTex, GLFramebuffer3D* dest )
{
	if ( nullptr == dest ) return false;

	const uint32_t width = dest->width ();
	const uint32_t height = dest->height ();
	const uint32_t depth = dest->depth ();

	glDisable ( GL_BLEND );

	glBindVertexArray ( vao );

	glBindFramebuffer ( GL_FRAMEBUFFER, dest->getHandle () );
	glViewport ( 0, 0, width, height );

	glUseProgram ( prog );

	glUniform1f ( kIndexLocUniformLightStep, sqrtf(2.0f) / (float)g_LightSamples );
	glUniform1i ( kIndexLocUniformLightSamples, g_LightSamples );
	glUniform3f ( kIndexLocUniformInverseSize, 1.0f / (float)width, 1.0f / (float)height, 1.0f / (float)depth);

	glBindTextureUnit ( kIndexTexBindDensity, blurTex );

	glDrawArraysInstanced ( GL_TRIANGLE_STRIP, 0, 4, depth );

	glBindTextureUnit ( kIndexTexBindDensity, 0 );

	glBindFramebuffer ( GL_FRAMEBUFFER, 0 );

	return true;
}

void RenderRaycasting ( GLuint prog, GLuint vao, GLuint densTex, GLuint lightTex )
{
	glBindFramebuffer ( GL_FRAMEBUFFER, 0 );
	
	glViewport ( 0, 0, g_ViewportWidth, g_ViewportHeight );
	
	glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear ( GL_COLOR_BUFFER_BIT );
	
	glEnable ( GL_BLEND );
	
	glBindVertexArray ( vao );
	
	glUseProgram ( prog );

//	mat4 modelViewTrans = glm::transpose ( g_ModelViewMatrix );
//	vec3 rayOrigin = vec3 ( modelViewTrans * vec4 ( g_EyePosition, 1.0f ) );

	mat4 viewMatrix = g_pMousePole->getMatrix ();
	mat4 modelMatrix = g_pObjectPole->getMatrix ();
	mat4 viewProjectionMatrix = g_ProjectionMatrix * viewMatrix;
	mat4 modelViewMatrix = viewMatrix * modelMatrix;
	mat4 mvp = viewProjectionMatrix * modelMatrix;
	vec3 rayOrigin = vec3 ( glm::transpose(modelViewMatrix) * vec4 ( g_EyePosition, 1.0f ) );

//	glUniformMatrix4fv ( kIndexLocUniformModelViewProjection, 1, GL_FALSE, glm::value_ptr ( g_ModelViewProjection ) );
	
//	glUniformMatrix4fv ( kIndexLocUniformViewMatrix, 1, GL_FALSE, glm::value_ptr ( g_ViewMatrix ) );
//	glUniformMatrix4fv ( kIndexLocUniformModelView, 1, GL_FALSE, glm::value_ptr ( g_ModelViewMatrix ) );
	glUniformMatrix4fv ( kIndexLocUniformProjectionMatrix, 1, GL_FALSE, glm::value_ptr ( g_ProjectionMatrix ) );
	glUniformMatrix4fv ( kIndexLocUniformModelViewProjection, 1, GL_FALSE, glm::value_ptr ( mvp ) );
	glUniformMatrix4fv ( kIndexLocUniformViewMatrix, 1, GL_FALSE, glm::value_ptr ( viewMatrix ) );
	glUniformMatrix4fv ( kIndexLocUniformModelView, 1, GL_FALSE, glm::value_ptr ( modelViewMatrix ) );
	glUniform1i ( kIndexLocUniformViewSamples, g_ViewSamples );
	glUniform1f ( kIndexLocUniformFocalLength, 1.0f / std::tanf ( g_FieldOfView / 2.0f ) );
	glUniform3fv ( kIndexLocUniformRayOrigin, 1, glm::value_ptr ( rayOrigin ) );
	glUniform2f ( kIndexLocUniformWindowSize, ( float ) g_ViewportWidth, ( float ) g_ViewportHeight );
	glUniform1f ( kIndexLocUniformStepSize, sqrtf ( 2.0f ) / ( float ) g_ViewSamples );

	glBindTextureUnit ( kIndexTexBindDensity, densTex );
	glBindTextureUnit ( kIndexTexBindLightCache, lightTex );

	glDrawArrays ( GL_POINTS, 0, 1 );

	glBindTextureUnit ( kIndexTexBindDensity, 0 );
	glBindTextureUnit ( kIndexTexBindLightCache, 0 );
}

int main ()
{
	GLFWApp app ( g_ViewportWidth, g_ViewportHeight, "Euler Fluid 3D Sim" );

	MousePole::RadiusDef initRadiusDef;
	initRadiusDef.fCurrRadius_ = glm::length ( g_EyePosition );
	initRadiusDef.fMinRadius_ = 0.5f;
	initRadiusDef.fMaxRadius_ = 50.0f;
	initRadiusDef.fSmallDelta_ = 0.001f;
	initRadiusDef.fLargeDelta_ = 0.01f;

	g_pMousePole = new MousePole ( vec3 ( 0.0f ), initRadiusDef, GLFW_MOUSE_BUTTON_LEFT );
	g_pObjectPole = new ObjectPole ( vec3 ( 0.0f ), g_pMousePole, GLFW_MOUSE_BUTTON_RIGHT );

	glfwSetCursorPosCallback (
		app.getWindow (),
		[] ( auto* window, double x, double y )
		{
			int width, height;
			glfwGetFramebufferSize ( window, &width, &height );
			mouseState.pos.x = static_cast< float >( x / width );
			mouseState.pos.y = static_cast< float >( y / height );
			g_pMousePole->mouseMove ( mouseState.pos.x, mouseState.pos.y );
			g_pObjectPole->mouseMove ( mouseState.pos.x, mouseState.pos.y );
		}
	);

	glfwSetMouseButtonCallback (
		app.getWindow (),
		[] ( auto* window, int button, int action, int mods )
		{
			const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
			if ( button == GLFW_MOUSE_BUTTON_LEFT )
				mouseState.pressedLeft = action == GLFW_PRESS;

			g_pMousePole->mouseButton ( button, action, mods, mouseState.pos.x, mouseState.pos.y );
			g_pObjectPole->mouseButton ( button, action, mods, mouseState.pos.x, mouseState.pos.y );
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
			if ( key == GLFW_KEY_C && pressed )
			{
				g_SimulateFluid = !g_SimulateFluid;
				if ( g_SimulateFluid )
				{
					printf ( "Simulating Fluid...\n" );
				}
				else
				{
					printf ( "NOT Simulating Fluid...\n" );
				}
			}
			if ( key == GLFW_KEY_B && pressed )
			{
				g_BlurAndBrighten = !g_BlurAndBrighten;
				if ( g_BlurAndBrighten )
				{
					printf ( "Blur and Brighten ON\n" );
				}
				else
				{
					printf ( "Blur and Brighten OFF\n" );
				}
			}
			if ( key == GLFW_KEY_L && pressed )
			{
				g_CacheLights = !g_CacheLights;
				if ( g_CacheLights )
				{
					printf ( "Cache Lights ON\n" );
				}
				else
				{
					printf ( "Cache Lights OFF\n" );
				}
			}
			if ( key == GLFW_KEY_R && pressed )
			{
				g_SplatRadius = ( float ) g_GridWidth / 8.0f;
				g_CellSize = 1.25f;
				g_TimeStep = 0.125f;
				g_AmbientTemperature = 0.0f;
				g_ImpulseTemperature = 10.0f;
				g_ImpulseDensity = 1.0f;
				g_SmokeBuoyancy = 1.0f;
				g_SmokeWeight = 0.05f;
				g_GradientScale = 1.125f / g_CellSize;
				g_TemperatureDissipation = 0.99f;
				g_VelocityDissipation = 0.99f;
				g_DensityDissipation = 0.9999f;
				g_ImpulsePosition = vec3 ( g_GridWidth / 2.0f, g_GridHeight - g_SplatRadius / 2.0f, g_GridDepth / 2.0f );
			}
			g_pMousePole->key ( key, scancode, action, mods );
		}
	);

	glfwSetScrollCallback ( 
		app.getWindow (), 
		[] ( GLFWwindow* window, double xoffset, double yoffset )
		{
			g_pMousePole->mouseWheel ( static_cast< float >( xoffset ), static_cast< float >( yoffset ) );
		} 
	);

	GLShader shdRaycastVert ( appendToRoot ( "assets/shaders/Raycast3D.vert" ).c_str () );
	GLShader shdRaycastGeom ( appendToRoot ( "assets/shaders/Raycast3D.geom" ).c_str () );
	GLShader shdRaycastFrag ( appendToRoot ( "assets/shaders/Raycast3D.frag" ).c_str () );
	GLProgram progRaycast ( shdRaycastVert, shdRaycastGeom, shdRaycastFrag );

	g_RaycastHandle = progRaycast.getHandle ();

	GLShader shdFluid3DVert ( appendToRoot ( "assets/shaders/Fluid3D.vert" ).c_str () );
	GLShader shdPickLayerGeom ( appendToRoot ( "assets/shaders/PickLayer3D.geom" ).c_str () );
	GLShader shdLightCacheFrag ( appendToRoot ( "assets/shaders/LightCache3D.frag" ).c_str () );
	GLProgram progLight ( shdFluid3DVert, shdPickLayerGeom, shdLightCacheFrag );

	g_LightHandle = progLight.getHandle ();

	GLShader shdLightBlurFrag ( appendToRoot ( "assets/shaders/LightBlur3D.frag" ).c_str () );
	GLProgram progBlur ( shdFluid3DVert, shdPickLayerGeom, shdLightBlurFrag );

	g_BlurHandle = progBlur.getHandle ();

	GLShader shdAdvect ( appendToRoot ( "assets/shaders/Advect3d.frag" ).c_str () );
	GLProgram progAdvect ( shdFluid3DVert, shdPickLayerGeom, shdAdvect );

	g_AdvectHandle = progAdvect.getHandle ();

	GLShader shdDivergence ( appendToRoot ( "assets/shaders/ComputeDivergence3D.frag" ).c_str () );
	GLProgram progDivergence ( shdFluid3DVert, shdPickLayerGeom, shdDivergence );

	g_ComputeDivergenceHandle = progDivergence.getHandle ();

	GLShader shdGradient ( appendToRoot ( "assets/shaders/SubtractGradient3D.frag" ).c_str () );
	GLProgram progGradient ( shdFluid3DVert, shdPickLayerGeom, shdGradient );

	g_SubtractGradientHandle = progGradient.getHandle ();

	GLShader shdJacobi ( appendToRoot ( "assets/shaders/Jacobi3D.frag" ).c_str () );
	GLProgram progJacobi ( shdFluid3DVert, shdPickLayerGeom, shdJacobi );

	g_JacobiHandle = progJacobi.getHandle ();

	GLShader shdImpulse ( appendToRoot ( "assets/shaders/Splat3D.frag" ).c_str () );
	GLProgram progImpulse ( shdFluid3DVert, shdPickLayerGeom, shdImpulse );

	g_SplatHandle = progImpulse.getHandle ();

	GLShader shdBuoyancy ( appendToRoot ( "assets/shaders/Buoyancy3D.frag" ).c_str () );
	GLProgram progBuoyancy ( shdFluid3DVert, shdPickLayerGeom, shdBuoyancy );

	g_BuoyancyHandle = progBuoyancy.getHandle ();

	GLShader shdSetup ( appendToRoot ( "assets/shaders/Fill.frag" ).c_str () );
	GLProgram progSetup ( shdFluid3DVert, shdSetup );

	g_FillHandle = progSetup.getHandle ();

	glGenVertexArrays ( 1, &g_CubeCenterVAO );
	glBindVertexArray ( g_CubeCenterVAO );
	g_PointVBO = CreatePointVbo ( 0.0f, 0.0f, 0.0f );
	GLsizeiptr stride = 3 * sizeof ( float );
	glBindBuffer ( GL_ARRAY_BUFFER, g_PointVBO );
	glEnableVertexAttribArray ( kIndexLocAttribPosition );
	glVertexAttribPointer ( kIndexLocAttribPosition, 3, GL_FLOAT, GL_FALSE, stride, nullptr );

	glGenVertexArrays ( 1, &g_QuadVAO );
	glBindVertexArray ( g_QuadVAO );
	g_QuadVBO = CreateQuadVbo ();
	stride = 2 * sizeof ( short );
	glBindBuffer ( GL_ARRAY_BUFFER, g_QuadVBO );
	glEnableVertexAttribArray ( kIndexLocAttribPosition );
	glVertexAttribPointer ( kIndexLocAttribPosition, 2, GL_SHORT, GL_FALSE, stride, nullptr );

	glBindVertexArray ( 0 );
	glBindBuffer ( GL_ARRAY_BUFFER, 0 );

	SlabPod velocityVol = CreateVolume ( g_GridWidth, g_GridHeight, g_GridDepth, 3 );
	SlabPod densityVol = CreateVolume ( g_GridWidth, g_GridHeight, g_GridDepth, 1 );
	SlabPod pressureVol = CreateVolume ( g_GridWidth, g_GridHeight, g_GridDepth, 1 );
	SlabPod temperatureVol = CreateVolume ( g_GridWidth, g_GridHeight, g_GridDepth, 1 );

	GLFramebuffer3D divergenceVol ( g_GridWidth, g_GridHeight, g_GridDepth, g_UseHalfFloats ? GL_RGB16F : GL_RGB32F, 0 );
	GLFramebuffer3D lightCacheVol ( g_GridWidth, g_GridHeight, g_GridDepth, g_UseHalfFloats ? GL_R16F : GL_R32F, 0 );
	GLFramebuffer3D blurDensityVol ( g_GridWidth, g_GridHeight, g_GridDepth, g_UseHalfFloats ? GL_R16F : GL_R32F, 0 );
	GLFramebuffer3D obstaclesVol ( g_GridWidth, g_GridHeight, g_GridDepth, g_UseHalfFloats ? GL_RGB16F : GL_RGB32F, 0 );
	
	CreateObstacles3D ( g_FillHandle, &obstaclesVol );
	
	ClearVolume ( temperatureVol.ping_.get (), g_AmbientTemperature );

	glDisable ( GL_DEPTH_TEST );
	glDisable ( GL_CULL_FACE );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	FramesPerSecondCounter fpsCounter ( 0.5f );

	float deltaSeconds = app.getDeltaSeconds ();

	while ( !glfwWindowShouldClose ( app.getWindow () ) )
	{
		// Update
		int width, height;
		glfwGetFramebufferSize ( app.getWindow (), &width, &height );

		const float ratio = width / ( float ) height;
		glViewport ( 0, 0, width, height );

		deltaSeconds = app.getDeltaSeconds ();
		fpsCounter.tick ( deltaSeconds );

		float dt = deltaSeconds * 0.0001f;

		vec3 up = vec3 ( 0.0f, 1.0f, 0.0f );
		vec3 target = vec3 ( 0.0f, 0.0f, 0.0f );
		g_ViewMatrix = glm::lookAt ( g_EyePosition, target, up );
		mat4 rotX = glm::rotate ( mat4 ( 1.0f ), g_ThetaX, vec3 ( 1.0f, 0.0f, 0.0f ) );
		mat4 rotY = glm::rotate ( rotX, g_ThetaY, vec3 ( 0.0f, 1.0f, 0.0f ) );
		g_ModelViewMatrix = g_ViewMatrix * rotY;
		g_ProjectionMatrix = glm::perspective ( g_FieldOfView, ratio, 0.0f, 1.0f );
		g_ModelViewProjection = g_ProjectionMatrix * g_ModelViewMatrix;

		float fips = 1.0f / ( dt );
		float alpha = 0.05f;
		if ( g_Fips < 0 )
		{
			g_Fips++;
		}
		else if ( g_Fips == 0 )
		{
			g_Fips = fips;
		}
		else
		{
			g_Fips = fips * alpha + g_Fips * ( 1.0f - alpha );
		}

		if ( g_SimulateFluid )
		{
			glViewport ( 0, 0, g_GridWidth, g_GridHeight );

			GLuint velTexPing = velocityVol.ping_->colorTex ();
			GLuint obsTex = obstaclesVol.colorTex ();

			Advect3D ( g_AdvectHandle, g_QuadVAO, velTexPing, velTexPing, obsTex, velocityVol.pong_.get (), g_VelocityDissipation );
			SwapPods ( &velocityVol );
			velTexPing = velocityVol.ping_->colorTex ();
			
			GLuint tempTexPing = temperatureVol.ping_->colorTex ();
			Advect3D ( g_AdvectHandle, g_QuadVAO, velTexPing, tempTexPing, obsTex, temperatureVol.pong_.get (), g_TemperatureDissipation );
			SwapPods ( &temperatureVol );
			tempTexPing = temperatureVol.ping_->colorTex ();

			GLuint densTexPing = densityVol.ping_->colorTex ();
			Advect3D ( g_AdvectHandle, g_QuadVAO, velTexPing, densTexPing, obsTex, densityVol.pong_.get (), g_DensityDissipation );
			SwapPods ( &densityVol );
			densTexPing = densityVol.ping_->colorTex ();

			ApplyBuoyancy3D ( g_BuoyancyHandle, g_QuadVAO, velTexPing, tempTexPing, densTexPing, velocityVol.pong_.get () );
			SwapPods ( &velocityVol );
			velTexPing = velocityVol.ping_->colorTex ();

			ApplyImpulse3D ( g_SplatHandle, g_QuadVAO, g_ImpulsePosition, g_ImpulseTemperature, temperatureVol.ping_.get () );
			ApplyImpulse3D ( g_SplatHandle, g_QuadVAO, g_ImpulsePosition, g_ImpulseDensity, densityVol.ping_.get () );			

			ComputeDivergence3D ( g_ComputeDivergenceHandle, g_QuadVAO, velTexPing, obsTex, &divergenceVol );
			ClearVolume ( pressureVol.ping_.get (), 0.0f );
			GLuint pressTexPing = pressureVol.ping_->colorTex ();
			GLuint divTex = divergenceVol.colorTex ();
			for ( int i = 0; i < g_NumJacobiIterations; ++i )
			{
				Jacobi3D ( g_JacobiHandle, g_QuadVAO, pressTexPing, divTex, obsTex, pressureVol.pong_.get () );
				SwapPods ( &pressureVol );
				pressTexPing = pressureVol.ping_->colorTex ();
			}

			SubtractGradient3D ( g_SubtractGradientHandle, g_QuadVAO, velTexPing, pressTexPing, obsTex, velocityVol.pong_.get () );
			SwapPods ( &velocityVol );
		}

		if ( g_BlurAndBrighten )
		{
			BlurAndBrighten ( g_BlurHandle, g_QuadVAO, densityVol.ping_->colorTex (), &blurDensityVol );
		}

		if ( g_CacheLights )
		{
			CacheLights ( g_LightHandle, g_QuadVAO, blurDensityVol.colorTex (), &lightCacheVol );
		}

		RenderRaycasting ( g_RaycastHandle, g_CubeCenterVAO, g_BlurAndBrighten ? blurDensityVol.colorTex () : densityVol.ping_->colorTex (), lightCacheVol.colorTex () );

		app.swapBuffers ();
	}

	delete g_pObjectPole;
	g_pObjectPole = nullptr;
	delete g_pMousePole;
	g_pMousePole = nullptr;
		
	glDeleteVertexArrays ( 1, &g_QuadVAO );
	glDeleteVertexArrays ( 1, &g_CubeCenterVAO );
	glDeleteBuffers ( 1, &g_QuadVBO );
	glDeleteBuffers ( 1, &g_PointVBO );	

	return 0;
}