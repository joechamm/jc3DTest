#include <jcGLframework/GLFWApp.h>
#include <jcGLframework/GLShader.h>
#include <jcGLframework/LineCanvasGL.h>
#include <jcGLframework/UtilsGLImGui.h>
#include <jcCommonFramework/ResourceString.h>
#include <jcCommonFramework/Camera.h>
#include <jcCommonFramework/UtilsFPS.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <iostream>
#include <istream>
#include <fstream>
#include <sstream>
#include <iomanip>

using glm::mat4;
using glm::mat3;
using glm::mat2;
using glm::vec4;
using glm::vec3;
using glm::vec2;

constexpr GLuint kIdxBind_PerFrameData = 7;
constexpr GLuint kIdxBind_CubeMatrices = 0;
constexpr GLuint kIdxBind_CubeVBO = 0;

constexpr GLint kIdxAttr_Cubein_vPosition = 0;
constexpr GLint kIdxAttr_Cubein_vColor = 1;

constexpr uint32_t kInstanceCount = 10;

constexpr vec3 kLookAtVec = vec3 ( 0.0f, 0.0f, 0.0f );
constexpr vec3 kEyePosVec = vec3 ( 0.0f, 0.0f, -2.0f );
constexpr vec3 kUpVec = vec3 ( 0.0f, 1.0f, 0.0f );

class OcclusionQuery
{
public:
	OcclusionQuery (const char* label = nullptr);
	~OcclusionQuery ();

	/** Begin the occlusion query. Until the query is ended, samples that pass the rendering pipeline are counted. */
	void beginQuery () const;

	/** Ends occlusion query and caches the result - number of samples that passed the rendering pipeline. */
	void endQuery ();

	/** Gets number of samples that have passed the rendering pipeline.  */
	GLint getNumSamplesPassed () const;

	/** Helper method that returns is any samples have passed the rendering pipeline.  */
	bool anySamplesPassed () const;

private:
	GLuint queryID_ { 0 }; // OpenGL query object ID
	GLint samplesPassed_ { 0 };
};

OcclusionQuery::OcclusionQuery (const char* label)
{
//	glGenQueries ( 1, &queryID_ );
	glCreateQueries ( GL_SAMPLES_PASSED, 1, &queryID_ );
	if ( nullptr != label )
	{
		glObjectLabel ( GL_QUERY, queryID_, std::strlen ( label ) + 1, label );
	}
//	std::cout << "Created occlusion query with ID " << queryID_ << std::endl;
}

OcclusionQuery::~OcclusionQuery ()
{
	glDeleteQueries ( 1, &queryID_ );
}

void OcclusionQuery::beginQuery () const
{
	glBeginQuery ( GL_SAMPLES_PASSED, queryID_ );
}

void OcclusionQuery::endQuery ()
{
	glEndQuery ( GL_SAMPLES_PASSED );
	glGetQueryObjectiv ( queryID_, GL_QUERY_RESULT, &samplesPassed_ );
}

GLint OcclusionQuery::getNumSamplesPassed () const
{
	return samplesPassed_;
}

bool OcclusionQuery::anySamplesPassed () const
{
	return samplesPassed_ > 0;
}

class PrimitivesQuery
{
public:
	PrimitivesQuery (const char* label = nullptr);
	~PrimitivesQuery ();
	
	void beginQuery () const;

	void endQuery ();

	GLint getNumPrimitivesGenerated () const;

	bool anyPrimitivesGenerated () const;

private:
	GLuint queryID_ { 0 };
	GLint primitivesGenerated_ { 0 };
};

PrimitivesQuery::PrimitivesQuery (const char* label)
{
//	glGenQueries ( 1, &queryID_ );
	glCreateQueries ( GL_PRIMITIVES_GENERATED, 1, &queryID_ );
	if ( nullptr != label )
	{
		glObjectLabel ( GL_QUERY, queryID_, std::strlen ( label ) + 1, label );
	}
//	std::cout << "Created primitives query with ID " << queryID_ << std::endl;
}

PrimitivesQuery::~PrimitivesQuery ()
{
	glDeleteQueries ( 1, &queryID_ );
}

void PrimitivesQuery::beginQuery () const
{
	glBeginQuery ( GL_PRIMITIVES_GENERATED, queryID_ );
}

void PrimitivesQuery::endQuery ()
{
	glEndQuery ( GL_PRIMITIVES_GENERATED );
	glGetQueryObjectiv ( queryID_, GL_QUERY_RESULT, &primitivesGenerated_ );
}

GLint PrimitivesQuery::getNumPrimitivesGenerated () const
{
	return primitivesGenerated_;
}

bool PrimitivesQuery::anyPrimitivesGenerated () const
{
	return primitivesGenerated_ > 0;
}

class TimerQuery
{
public:
	TimerQuery ( const char* beginLabel = nullptr, const char* endLabel = nullptr );
	~TimerQuery ();

	void beginQuery () const;
	void endQuery () const;

	GLuint64 getElapsedTime ();

private:
	GLuint queryID_ [ 2 ];
	GLuint64 startTime_ { 0 };
	GLuint64 endTime_ { 0 };
};

TimerQuery::TimerQuery ( const char* beginLabel, const char* endLabel )
{
//	glGenQueries ( 2, queryID_ );
	glCreateQueries ( GL_TIMESTAMP, 2, queryID_ );
	if ( nullptr != beginLabel )
	{
		glObjectLabel ( GL_QUERY, queryID_ [ 0 ], std::strlen ( beginLabel ) + 1, beginLabel );
	}
	if ( nullptr != endLabel )
	{
		glObjectLabel ( GL_QUERY, queryID_ [ 1 ], std::strlen ( endLabel ) + 1, endLabel );
	}
}

TimerQuery::~TimerQuery ()
{
	glDeleteQueries ( 2, queryID_ );
}

void TimerQuery::beginQuery () const
{
	glQueryCounter ( queryID_ [ 0 ], GL_TIMESTAMP );
}

void TimerQuery::endQuery () const 
{
	glQueryCounter ( queryID_ [ 1 ], GL_TIMESTAMP );
}

GLuint64 TimerQuery::getElapsedTime ()
{
	GLint stopTimerAvailable = 0;
	while ( !stopTimerAvailable )
	{
		glGetQueryObjectiv ( queryID_ [ 1 ], GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable );
	}

	glGetQueryObjectui64v ( queryID_ [ 0 ], GL_QUERY_RESULT, &startTime_ );
	glGetQueryObjectui64v ( queryID_ [ 1 ], GL_QUERY_RESULT, &endTime_ );
	return ( endTime_ - startTime_ );
}

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 model;
	vec4 cameraPos;
	vec4 lightPos;
	vec4 ambientLight;
	vec4 diffuseLight;
	vec4 specularLight;
};

struct MouseState
{
	vec2 pos = vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

CameraPositioner_FirstPerson positioner ( kEyePosVec, kLookAtVec, kUpVec );
Camera camera ( positioner );

FramesPerSecondCounter fpsCounter;

struct PositionalLight
{
	vec4 position;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
} g_positionalLight;

vec3 g_scaleFactor = vec3 ( 10.0f, 10.0f, 10.0f );
vec4 g_clearColor = vec4 ( 1.0f, 1.0f, 1.0f, 1.0f );

GLuint g_ubo = 0;
GLuint g_matrices_ssbo = 0;
GLuint g_cube_vbo = 0;
GLuint g_cube_ebo = 0;
GLuint g_vao = 0;

bool g_enable_depth_cube = true;
bool g_enable_depth_grid = false;
bool g_enable_blend_cube = false;
bool g_enable_blend_grid = true;

uint32_t g_current_instance_idx = 0;

std::vector<mat4> g_model_matrices;

struct InstancedCubeVertex
{
	vec4 position;
	vec4 color;
};

struct InstancedCube
{
	enum eCubeCorner : uint32_t
	{
		eCubeCorner_frontTopRight = 0,
		eCubeCorner_frontBottomRight = 1,
		eCubeCorner_frontBottomLeft = 2,
		eCubeCorner_frontTopLeft = 3,
		eCubeCorner_backTopRight = 4,
		eCubeCorner_backBottomRight = 5,
		eCubeCorner_backBottomLeft = 6,
		eCubeCorner_backTopLeft = 7,
		eCubeCorner_count = 8
	};

	InstancedCubeVertex vertices [ 8 ];
	GLuint				indices [ 36 ];
};

// unit cube with the front top right vertex at (1, 1, 1) and back bottom left vertex at (0,0,0)
InstancedCube getUnitCube ( const vec4& color )
{
	InstancedCube unitCube;
	for ( uint32_t corner = 0; corner < InstancedCube::eCubeCorner_count ; corner++ )
	{
		unitCube.vertices [ corner ].color = color;
	}

	/* FRONT */
	unitCube.vertices [ InstancedCube::eCubeCorner_frontTopRight ].position = vec4(1.f, 1.f, 1.f, 1.f);
	unitCube.vertices [ InstancedCube::eCubeCorner_frontBottomRight ].position = vec4 ( 1.f, 0.f, 1.f, 1.f );
	unitCube.vertices [ InstancedCube::eCubeCorner_frontBottomLeft ].position = vec4 ( 1.f, 0.f, 0.f, 1.f );
	unitCube.vertices [ InstancedCube::eCubeCorner_frontTopLeft ].position = vec4 ( 0.f, 1.f, 1.f, 1.f );
	/* BACK */
	unitCube.vertices [ InstancedCube::eCubeCorner_backTopRight ].position = vec4 ( 1.f, 1.f, 0.f, 1.f );
	unitCube.vertices [ InstancedCube::eCubeCorner_backBottomRight ].position = vec4 ( 1.f, 0.f, 0.f, 1.f );
	unitCube.vertices [ InstancedCube::eCubeCorner_backBottomLeft ].position = vec4 ( 0.f, 0.f, 0.f, 1.f );
	unitCube.vertices [ InstancedCube::eCubeCorner_backTopLeft ].position = vec4 ( 0.f, 1.f, 0.f, 1.f );	

	/* FRONT FACE */
	// triangle 1
	unitCube.indices [ 0 ] = InstancedCube::eCubeCorner_frontBottomLeft;
	unitCube.indices [ 1 ] = InstancedCube::eCubeCorner_frontBottomRight;
	unitCube.indices [ 2 ] = InstancedCube::eCubeCorner_frontTopLeft;
	// triangle 2
	unitCube.indices [ 3 ] = InstancedCube::eCubeCorner_frontTopLeft;
	unitCube.indices [ 4 ] = InstancedCube::eCubeCorner_frontBottomRight;
	unitCube.indices [ 5 ] = InstancedCube::eCubeCorner_frontTopRight;
	/* BACK FACE */
	// triangle 1
	unitCube.indices [ 6 ] = InstancedCube::eCubeCorner_backBottomRight;
	unitCube.indices [ 7 ] = InstancedCube::eCubeCorner_backBottomLeft;
	unitCube.indices [ 8 ] = InstancedCube::eCubeCorner_backTopLeft;
	// triangle 2
	unitCube.indices [ 9 ] = InstancedCube::eCubeCorner_backTopLeft;
	unitCube.indices [ 10 ] = InstancedCube::eCubeCorner_backTopRight;
	unitCube.indices [ 11 ] = InstancedCube::eCubeCorner_backBottomRight;
	/* LEFT FACE */
	// triangle 1
	unitCube.indices [ 12 ] = InstancedCube::eCubeCorner_frontBottomLeft;	
	unitCube.indices [ 13 ] = InstancedCube::eCubeCorner_frontTopLeft;
	unitCube.indices [ 14 ] = InstancedCube::eCubeCorner_backBottomLeft;
	// triangle 2
	unitCube.indices [ 15 ] = InstancedCube::eCubeCorner_backBottomLeft;
	unitCube.indices [ 16 ] = InstancedCube::eCubeCorner_frontTopLeft;
	unitCube.indices [ 17 ] = InstancedCube::eCubeCorner_backTopLeft;
	/* RIGHT FACE */
	// triangle 1
	unitCube.indices [ 18 ] = InstancedCube::eCubeCorner_frontBottomRight;
	unitCube.indices [ 19 ] = InstancedCube::eCubeCorner_backBottomRight;
	unitCube.indices [ 20 ] = InstancedCube::eCubeCorner_frontTopRight;
	// triangle 2
	unitCube.indices [ 21 ] = InstancedCube::eCubeCorner_frontTopRight;
	unitCube.indices [ 22 ] = InstancedCube::eCubeCorner_backBottomRight;
	unitCube.indices [ 23 ] = InstancedCube::eCubeCorner_backTopRight;
	/* TOP FACE */
	// triangle 1
	unitCube.indices [ 24 ] = InstancedCube::eCubeCorner_frontTopLeft;
	unitCube.indices [ 25 ] = InstancedCube::eCubeCorner_frontTopRight;
	unitCube.indices [ 26 ] = InstancedCube::eCubeCorner_backTopLeft;
	// triangle 2
	unitCube.indices [ 27 ] = InstancedCube::eCubeCorner_backTopLeft;
	unitCube.indices [ 28 ] = InstancedCube::eCubeCorner_frontTopRight;
	unitCube.indices [ 29 ] = InstancedCube::eCubeCorner_backTopRight;
	/* BOTTOM FACE */
	// triangle 1
	unitCube.indices [ 30 ] = InstancedCube::eCubeCorner_frontBottomLeft;
	unitCube.indices [ 31 ] = InstancedCube::eCubeCorner_backBottomLeft;
	unitCube.indices [ 32 ] = InstancedCube::eCubeCorner_frontBottomRight;
	// triangle 2
	unitCube.indices [ 33 ] = InstancedCube::eCubeCorner_frontBottomRight;
	unitCube.indices [ 34 ] = InstancedCube::eCubeCorner_backBottomLeft;
	unitCube.indices [ 35 ] = InstancedCube::eCubeCorner_backBottomRight;

	return unitCube;
}

// helper function for our model matrices
mat4 getTranslatedScaleMatrix ( float scaleX, float scaleY, float scaleZ, float transX )
{
	return glm::translate ( glm::scale ( mat4 ( 1.0f ), vec3 ( scaleX, scaleY, scaleZ ) ), vec3 ( transX, 0.f, 0.f ) );
}

// buildInstancedTransform takes the instance index of our cube transform and a height value to build the model matrix for the instanced cube
mat4 buildInstancedTransform ( uint32_t idx, float height )
{
	// divide up the unit interval [0, 1]
	float dx = 1.0f / static_cast< float >( kInstanceCount );
	// get the x location of this instance
	float x = static_cast< float >( idx ) * dx;
	// translate to [-1, 1] space
	float transX = 2.0f * x - 1.0f;
	// scale x and z by factor of 2dx
	float scaleX = g_scaleFactor.x * dx * 2.0f;
	float scaleY = g_scaleFactor.y * height;
	float scaleZ = g_scaleFactor.z * dx * 2.0f;
	return getTranslatedScaleMatrix ( scaleX, scaleY, scaleZ, transX );
}

// helper function to initialze our light
void initPositionalLight ()
{
	g_positionalLight.position = vec4 ( 0.f, 2.f, -1.f, 1.f );
	g_positionalLight.ambient = vec4 ( 0.1f, 0.1f, 0.1f, 1.0f );
	g_positionalLight.diffuse = vec4 ( 0.7f, 0.7f, 0.7f, 1.0f );
	g_positionalLight.specular = vec4 ( 1.0f );
}

// helper function to initialze model matrices
void initModelMatrices ()
{
	g_model_matrices.resize ( kInstanceCount );

	// initialize our height values on the unit interval [0, 1] to the value of 2sin(2pi x) to get one full period
	float x = 0.0f;
	const float dx = 1.0f / static_cast< float >( kInstanceCount );
	const float twopi = glm::pi<float> () * 2.0f;
	for ( uint32_t i = 0; i < kInstanceCount; i++ )
	{
		float height = 2.0f * sinf ( x );
		g_model_matrices [ i ] = buildInstancedTransform ( i, height );
		x += dx;
	}
}

void updateMatricesBuffer ( uint32_t idx )
{
	const GLsizei kUpdateByteCount = sizeof ( mat4 );
	const GLsizei kUpdateByteOffset = kUpdateByteCount * idx;	
	glNamedBufferSubData ( g_matrices_ssbo, kUpdateByteOffset, kUpdateByteCount, glm::value_ptr ( g_model_matrices [ idx ] ) );
}

int main ( int argc, char** argv )
{
	GLFWApp app;

	initPositionalLight ();
	initModelMatrices ();

	InstancedCube unitCube = getUnitCube ( vec4 ( 0.1f, 1.0f, 0.15f, 1.0f ) );

	GLShader shdRenderVert ( appendToRoot ( "assets/shaders/instanced_cube.vert" ).c_str () );
	GLShader shdRenderGeom ( appendToRoot ( "assets/shaders/instanced_cube.geom" ).c_str () );
	GLShader shdRenderFrag ( appendToRoot ( "assets/shaders/instanced_cube.frag" ).c_str () );
	GLProgram progRender ( shdRenderVert, shdRenderGeom, shdRenderFrag );

	GLShader shdGridVert ( appendToRoot ( "assets/shaders/instanced_cube_grid.vert" ).c_str () );
	GLShader shdGridFrag ( appendToRoot ( "assets/shaders/instanced_cube_grid.frag" ).c_str () );
	GLProgram progGrid ( shdGridVert, shdGridFrag );

	GLuint renderHandle = progRender.getHandle ();
	GLuint gridHandle = progGrid.getHandle ();

#ifndef NDEBUG
	const GLchar* kLabelRenderProg = "InstancedCubeRenderProgram";
	const GLchar* kLabelGridProg = "GridRenderProgram";
	glObjectLabel ( GL_PROGRAM, renderHandle, std::strlen(kLabelRenderProg), kLabelRenderProg);
	glObjectLabel ( GL_PROGRAM, gridHandle, std::strlen(kLabelGridProg),  kLabelGridProg );
#endif

	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );
	GLBuffer perFrameDataBuffer ( kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	g_ubo = perFrameDataBuffer.getHandle ();

	glBindBufferRange ( GL_UNIFORM_BUFFER, kIdxBind_PerFrameData, perFrameDataBuffer.getHandle (), 0, kUniformBufferSize );
#ifndef NDEBUG

	const GLchar* kLabelUBO = "PerFrameDataBuffer";
	glObjectLabel ( GL_BUFFER, g_ubo, std::strlen(kLabelUBO), kLabelUBO);
#endif

	const GLsizei kMatricesBufferSize = g_model_matrices.size () * sizeof ( mat4 );

	GLBuffer modelMatricesDataBuffer ( kMatricesBufferSize, g_model_matrices.data (), GL_DYNAMIC_STORAGE_BIT );
	g_matrices_ssbo = modelMatricesDataBuffer.getHandle ();

	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kIdxBind_CubeMatrices, modelMatricesDataBuffer.getHandle () );
#ifndef NDEBUG
	const GLchar* kLabelMatricesSSBO = "ModelMatricesSSBO";
	glObjectLabel ( GL_BUFFER, g_matrices_ssbo, std::strlen(kLabelMatricesSSBO), kLabelMatricesSSBO);

#endif

	GLuint vao;
	glCreateVertexArrays ( 1, &vao );

#ifndef NDEBUG
	const GLchar* kLabelVAO = "VAO";
	glObjectLabel ( GL_VERTEX_ARRAY, vao, std::strlen(kLabelVAO), kLabelVAO);
#endif

	g_vao = vao;

	const GLsizeiptr kCubeVertexBufferStride = sizeof ( InstancedCubeVertex );
	const GLsizeiptr kCubeVertexBufferSize = InstancedCube::eCubeCorner_count * kCubeVertexBufferStride;
	const GLsizeiptr kCubeVertexBufferPosOffset = 0;
	const GLsizeiptr kCubeVertexBufferColOffset = sizeof ( unitCube.vertices [ 0 ].position );
	const GLsizeiptr kCubeIndexBufferSize = 36 * sizeof ( unitCube.indices [ 0 ] );

	GLBuffer cubeVertexBuffer ( kCubeVertexBufferSize, unitCube.vertices, 0 );
	GLBuffer cubeIndexBuffer ( kCubeIndexBufferSize, unitCube.indices, 0 );

	g_cube_vbo = cubeVertexBuffer.getHandle ();
	g_cube_ebo = cubeIndexBuffer.getHandle ();
	
	glVertexArrayElementBuffer ( vao, g_cube_ebo );
	
	glVertexArrayAttribBinding ( vao, kIdxAttr_Cubein_vPosition, kIdxBind_CubeVBO );
	glVertexArrayAttribFormat ( vao, kIdxAttr_Cubein_vPosition, 4, GL_FLOAT, GL_FALSE, offsetof ( InstancedCubeVertex, position ) );
	glEnableVertexArrayAttrib ( vao, kIdxAttr_Cubein_vPosition );

	glVertexArrayAttribBinding ( vao, kIdxAttr_Cubein_vColor, kIdxBind_CubeVBO );
	glVertexArrayAttribFormat ( vao, kIdxAttr_Cubein_vColor, 4, GL_FLOAT, GL_FALSE, offsetof ( InstancedCubeVertex, color ) );
	glEnableVertexArrayAttrib ( vao, kIdxAttr_Cubein_vColor );

	glVertexArrayVertexBuffer ( vao, kIdxBind_CubeVBO, g_cube_vbo, 0, kCubeVertexBufferStride );

#ifndef NDEBUG
	const GLchar* kLabelVBO = "CubeVertexBuffer";
	const GLchar* kLabelEBO = "CubeIndexBuffer";
	glObjectLabel ( GL_BUFFER, g_cube_vbo, std::strlen(kLabelVBO), kLabelVBO);
	glObjectLabel ( GL_BUFFER, g_cube_ebo, std::strlen(kLabelEBO), kLabelEBO);
#endif

	glBindVertexArray ( g_vao );

	GLint vaoElemArrayBufBinding, vaoAttrArrBufBndPos, vaoAttrArrBufBndCol, vaoAttrRelOffsetPos, vaoAttrRelOffsetCol, elemBuffSize, vertBuffSize, elemBuffStFlags, vertBuffStFlags, elemBuffUsage, vertBuffUsage;
	glGetVertexArrayiv ( g_vao, GL_ELEMENT_ARRAY_BUFFER_BINDING, &vaoElemArrayBufBinding);
	glGetVertexAttribiv ( kIdxAttr_Cubein_vPosition, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &vaoAttrArrBufBndPos );
	glGetVertexAttribiv ( kIdxAttr_Cubein_vColor, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &vaoAttrArrBufBndCol );

	glGetVertexArrayIndexediv ( g_vao, kIdxAttr_Cubein_vPosition, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, &vaoAttrRelOffsetPos );
	glGetVertexArrayIndexediv ( g_vao, kIdxAttr_Cubein_vColor, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, &vaoAttrRelOffsetCol );

	glGetNamedBufferParameteriv ( g_cube_vbo, GL_BUFFER_SIZE, &vertBuffSize );
	glGetNamedBufferParameteriv ( g_cube_ebo, GL_BUFFER_SIZE, &elemBuffSize );
	glGetNamedBufferParameteriv ( g_cube_vbo, GL_BUFFER_STORAGE_FLAGS, &vertBuffStFlags );
	glGetNamedBufferParameteriv ( g_cube_ebo, GL_BUFFER_STORAGE_FLAGS, &elemBuffStFlags );
	glGetNamedBufferParameteriv ( g_cube_vbo, GL_BUFFER_USAGE, &vertBuffUsage );
	glGetNamedBufferParameteriv ( g_cube_ebo, GL_BUFFER_USAGE, &elemBuffUsage );

	std::cout << "vao element array buffer binding " << vaoElemArrayBufBinding << "  g_cube_ebo " << g_cube_ebo << std::endl;
	std::cout << "vao position buffer binding " << vaoAttrArrBufBndPos << "  g_cube_vbo " << g_cube_vbo << " kIdxBind_CubeVBO " << kIdxBind_CubeVBO << std::endl;
	std::cout << "vao color buffer binding " << vaoAttrArrBufBndCol << "  g_cube_vbo " << g_cube_vbo << " kIdxBind_CubeVBO " << kIdxBind_CubeVBO << std::endl;
	std::cout << "vao position relative offset " << vaoAttrRelOffsetPos << std::endl;
	std::cout << "vao color relative offset " << vaoAttrRelOffsetCol << std::endl;
	std::cout << "cube vbo buffer size " << vertBuffSize << std::endl;
	std::cout << "cube ebo buffer size " << elemBuffSize << std::endl;

	std::string vertBuffStorageFlagString = "cube vertex buffer storage flags: ";
	if ( vertBuffStFlags & GL_MAP_READ_BIT )
	{
		vertBuffStorageFlagString += " GL_MAP_READ_BIT ";
	}
	if ( vertBuffStFlags & GL_MAP_WRITE_BIT )
	{
		vertBuffStorageFlagString += " GL_MAP_WRITE_BIT ";
	}
	if ( vertBuffStFlags & GL_DYNAMIC_STORAGE_BIT )
	{
		vertBuffStorageFlagString += " GL_DYNAMIC_STORAGE_BIT ";
	}
	if ( vertBuffStFlags & GL_CLIENT_STORAGE_BIT )
	{
		vertBuffStorageFlagString += " GL_CLIENT_STORAGE_BIT ";
	}

	std::cout << vertBuffStorageFlagString << std::endl;

	std::string elemBuffStorageFlagString = "cube index buffer storage flags: ";
	if ( elemBuffStFlags & GL_MAP_READ_BIT )
	{
		elemBuffStorageFlagString += " GL_MAP_READ_BIT ";
	}
	if ( elemBuffStFlags & GL_MAP_WRITE_BIT )
	{
		elemBuffStorageFlagString += " GL_MAP_WRITE_BIT ";
	}
	if ( elemBuffStFlags & GL_DYNAMIC_STORAGE_BIT )
	{
		elemBuffStorageFlagString += " GL_DYNAMIC_STORAGE_BIT ";
	}
	if ( elemBuffStFlags & GL_CLIENT_STORAGE_BIT )
	{
		elemBuffStorageFlagString += " GL_CLIENT_STORAGE_BIT ";
	}

	std::cout << elemBuffStorageFlagString << std::endl;

	std::string vertBuffUsageStr = "cube vertex buffer usage ";
	if ( vertBuffUsage == GL_STATIC_DRAW )
	{
		vertBuffUsageStr += " GL_STATIC_DRAW ";
	}
	else if ( vertBuffUsage == GL_DYNAMIC_DRAW )
	{
		vertBuffUsageStr += " GL_DYNAMIC_DRAW ";
	}
	else if ( vertBuffUsage == GL_DYNAMIC_READ )
	{
		vertBuffUsageStr += " GL_DYNAMIC_DRAW ";
	}
	else if ( vertBuffUsage == GL_DYNAMIC_COPY )
	{
		vertBuffUsageStr += " GL_DYNAMIC_COPY ";
	}
	else if ( vertBuffUsage == GL_STREAM_DRAW )
	{
		vertBuffUsageStr += " GL_STREAM_DRAW ";
	}
	else if ( vertBuffUsage == GL_STREAM_READ )
	{
		vertBuffUsageStr += " GL_STREAM_READ ";
	}
	else if ( vertBuffUsage == GL_STREAM_COPY )
	{
		vertBuffUsageStr += " GL_STREAM_COPY ";
	}
	else if ( vertBuffUsage == GL_STATIC_READ )
	{
		vertBuffUsageStr += " GL_STATIC_READ ";
	}
	else if ( vertBuffUsage == GL_STATIC_COPY )
	{
		vertBuffUsageStr += " GL_STATIC_COPY ";
	}
	else
	{
		vertBuffUsageStr += " UNKNOWN ";
	}

	std::cout << vertBuffUsageStr << std::endl;

	std::string elemBuffUsageStr = "cube index buffer usage ";
	if ( elemBuffUsage == GL_STATIC_DRAW )
	{
		elemBuffUsageStr += " GL_STATIC_DRAW ";
	}
	else if ( elemBuffUsage == GL_DYNAMIC_DRAW )
	{
		elemBuffUsageStr += " GL_DYNAMIC_DRAW ";
	}
	else if ( elemBuffUsage == GL_DYNAMIC_READ )
	{
		elemBuffUsageStr += " GL_DYNAMIC_DRAW ";
	}
	else if ( elemBuffUsage == GL_DYNAMIC_COPY )
	{
		elemBuffUsageStr += " GL_DYNAMIC_COPY ";
	}
	else if ( elemBuffUsage == GL_STREAM_DRAW )
	{
		elemBuffUsageStr += " GL_STREAM_DRAW ";
	}
	else if ( elemBuffUsage == GL_STREAM_READ )
	{
		elemBuffUsageStr += " GL_STREAM_READ ";
	}
	else if ( elemBuffUsage == GL_STREAM_COPY )
	{
		elemBuffUsageStr += " GL_STREAM_COPY ";
	}
	else if ( elemBuffUsage == GL_STATIC_READ )
	{
		elemBuffUsageStr += " GL_STATIC_READ ";
	}
	else if ( elemBuffUsage == GL_STATIC_COPY )
	{
		elemBuffUsageStr += " GL_STATIC_COPY ";
	}
	else
	{
		elemBuffUsageStr += " UNKNOWN ";
	}

	std::cout << elemBuffUsageStr << std::endl;

	glEnable ( GL_DEPTH_TEST );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );

	glfwSetCursorPosCallback (
		app.getWindow (),
		[] ( auto* window, double x, double y )
		{
			int width, height;
			glfwGetFramebufferSize ( window, &width, &height );
			mouseState.pos.x = static_cast< float >( x / width );
			mouseState.pos.y = static_cast< float >( y / height );
			ImGui::GetIO ().MousePos = ImVec2 ( ( float ) x, ( float ) y );
		}
	);

	glfwSetMouseButtonCallback (
		app.getWindow (),
		[] ( auto* window, int button, int action, int mods )
		{
			auto& io = ImGui::GetIO ();
			const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
			io.MouseDown [ idx ] = action == GLFW_PRESS;
			if ( !io.WantCaptureMouse )
			{
				if ( button == GLFW_MOUSE_BUTTON_LEFT )
				{
					mouseState.pressedLeft = action == GLFW_PRESS;
				}
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
				g_current_instance_idx = ( g_current_instance_idx + 1 ) % kInstanceCount;
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

	ImGuiGLRenderer rendererUI;

	double timeStamp = glfwGetTime ();
	float deltaSeconds = 0.0f;

	while ( !glfwWindowShouldClose ( app.getWindow () ) )
	{
		ImGuiIO& io = ImGui::GetIO ();

		TimerQuery timerQuery ( "drawFrameQueryBegin", "drawFrameQueryEnd" );
		OcclusionQuery occlusionQuery ( "drawFrameOcclusionQuery" );
		PrimitivesQuery primitivesQuery ( "drawFramePrimitivesQuery" );

		positioner.update ( deltaSeconds, mouseState.pos, mouseState.pressedLeft && !io.WantCaptureMouse );
		fpsCounter.tick ( deltaSeconds );

		const double newTimeStamp = glfwGetTime ();
		deltaSeconds = static_cast< float >( newTimeStamp - timeStamp );
		timeStamp = newTimeStamp;

		const float fps = fpsCounter.getFPS ();

		int width, height;
		glfwGetFramebufferSize ( app.getWindow (), &width, &height );
		const float ratio = width / ( float ) height;

		timerQuery.beginQuery ();

		glClearColor ( g_clearColor.r, g_clearColor.g, g_clearColor.b, g_clearColor.a );

		glViewport ( 0, 0, width, height );
		glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		occlusionQuery.beginQuery ();
		primitivesQuery.beginQuery ();

		const mat4 proj = glm::perspective ( 45.0f, ratio, 0.1f, 1000.0f );
		const mat4 view = camera.getViewMatrix ();
		
		const PerFrameData perFrameData = {
			.view = view,
			.proj = proj,
			.cameraPos = vec4 ( camera.getPosition (), 1.f ),
			.lightPos = g_positionalLight.position,
			.ambientLight = g_positionalLight.ambient,
			.diffuseLight = g_positionalLight.diffuse,
			.specularLight = g_positionalLight.specular
		};

		glNamedBufferSubData ( g_ubo, 0, kUniformBufferSize, &perFrameData );

		glBindVertexArray ( g_vao );

		if ( g_enable_blend_cube )
		{
			glEnable ( GL_BLEND );
		}
		else
		{
			glDisable ( GL_BLEND );
		}

		if ( g_enable_depth_cube )
		{
			glEnable ( GL_DEPTH_TEST );
		}
		else
		{
			glDisable ( GL_DEPTH_TEST );
		}

		progRender.useProgram ();
		glDrawElementsInstanced ( GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr, kInstanceCount );

		if ( g_enable_blend_grid )
		{
			glEnable ( GL_BLEND );
		}
		else
		{
			glDisable ( GL_BLEND );
		}
		if ( g_enable_depth_grid )
		{
			glEnable ( GL_DEPTH_TEST );
		}
		else
		{
			glDisable ( GL_DEPTH_TEST );
		}

		progGrid.useProgram ();
		glDrawArraysInstancedBaseInstance ( GL_TRIANGLES, 0, 6, 1, 0 );

		primitivesQuery.endQuery ();
		occlusionQuery.endQuery ();
		timerQuery.endQuery ();

		GLint primitivesCount = primitivesQuery.getNumPrimitivesGenerated ();
		GLint occlusionCount = occlusionQuery.getNumSamplesPassed ();
		GLuint64 renderTime = timerQuery.getElapsedTime ();
		float elapsedMS = static_cast<float>(renderTime) / 1'000'000.0f;

		io.DisplaySize = ImVec2 ( ( float ) width, ( float ) height );
		ImGui::NewFrame ();
		ImGui::Begin ( "Control", nullptr );
		ImGui::Text ( "FPS: %f", fps );
		ImGui::Text ( "Primitives Generated: %i", primitivesCount );
		ImGui::Text ( "Samples Passed: %i", occlusionCount );
		ImGui::Text ( "Time Elapsed: %f ms", elapsedMS );
		ImGui::Separator ();
		ImGui::Checkbox ( "Cube Blend:", &g_enable_blend_cube );
		ImGui::Checkbox ( "Cube Depth:", &g_enable_depth_cube );
		ImGui::Checkbox ( "Grid Blend:", &g_enable_blend_grid );
		ImGui::Checkbox ( "Grid Depth:", &g_enable_depth_grid );
		ImGui::Separator ();
		ImGui::ColorEdit4 ( "Clear Color:", glm::value_ptr ( g_clearColor ) );
		ImGui::Separator ();
		ImGui::Text ( "Selected Instance: %i", g_current_instance_idx );
		ImGui::Separator ();
		ImGui::Text ( "Positional Light:" );
		ImGui::SliderFloat3 ( "Light Position:", glm::value_ptr ( g_positionalLight.position ), -10.0f, 10.0f );
		ImGui::End ();
		ImGui::Render ();
		rendererUI.render ( width, height, ImGui::GetDrawData () );

		app.swapBuffers ();
	}

	glDeleteVertexArrays ( 1, &g_vao );
	glDeleteBuffers ( 1, &g_cube_vbo );
	glDeleteBuffers ( 1, &g_cube_ebo );

	return 0;
}