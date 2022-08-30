#include "WaveSystem1D.h"
#include <cmath>
//#include <jcGLframework/debug.h>
#include <jcCommonFramework/ResourceString.h>

static constexpr uint32_t kWaveSystemMaxN = 32 * 1024 * 1024;	// max points 64M
static constexpr uint32_t kWaveSystemMinN = 32;					// min points 32
static constexpr float kWaveSystemMinTimeStep = 1E-5;
static constexpr float kWaveSystemMaxTimeStep = 1E5;			// TODO: this probably needs to be tweaked
static constexpr float kWaveSystemMinWaveSpeed = 1E-5;
static constexpr float kWaveSystemMaxWaveSpeed = 1E5;

WaveSystem1D::WaveSystem1D ( float dt, float c, uint32_t n )
	: m_dt ( std::max ( std::min ( dt, kWaveSystemMaxTimeStep ), kWaveSystemMinTimeStep ) )
	, m_wave_speed ( std::max ( std::min ( c, kWaveSystemMaxWaveSpeed ), kWaveSystemMinWaveSpeed ) )
	, m_n ( std::max ( std::min ( n, kWaveSystemMaxN ), kWaveSystemMinN ) )
	, m_write_idx ( 1 )
{
	m_vertices_ssbo [ 0 ] = 0;
	m_vertices_ssbo [ 1 ] = 0;
	m_initial_state.clear ();
	initBuffers ();
	initShaderProgram ();
}

WaveSystem1D::~WaveSystem1D ()
{
	if ( m_vertices_ssbo [ 0 ] )
	{
		destroyBuffers ();
	}

	if ( m_update_program )
	{
		delete m_update_program;
	}
}

bool WaveSystem1D::setN ( uint32_t n )
{
	if ( n == m_n ) return true; // no need to change anything
	if ( n < kWaveSystemMinN || n > kWaveSystemMaxN ) return false; // out of bounds
	m_n = n;
	m_initial_state.clear ();
	if ( m_vertices_ssbo [ 0 ] )
	{
		destroyBuffers ();
	}
	return initBuffers ();
}

bool WaveSystem1D::setTimeStep ( float dt )
{
	if ( dt < kWaveSystemMinTimeStep || dt > kWaveSystemMaxTimeStep ) return false;
	m_dt = dt;
	return true;
}

bool WaveSystem1D::setWaveSpeed ( float c )
{
	if ( c < kWaveSystemMinWaveSpeed || c > kWaveSystemMaxWaveSpeed ) return false;
	m_wave_speed = c;
	return true;
}

bool WaveSystem1D::initializeVertices ( const std::function<void ( std::vector<Wave1DVert>&, uint32_t )>& initVerticesFunc )
{
	m_initial_state.resize ( m_n );
	initVerticesFunc ( m_initial_state, m_n );
	return fillBuffers ();
}

void WaveSystem1D::updateSystem ()
{
	const GLuint kBindIdx_READ = 0;
	const GLuint kBindIdx_WRITE = 1;
	const GLint kUniLoc_C = 0;
	const GLint kUniLoc_dt = 1;

	GLuint readIdx = ( m_write_idx + 1 ) % 2;

	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kBindIdx_READ, m_vertices_ssbo [ readIdx ] );
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kBindIdx_WRITE, m_vertices_ssbo [ m_write_idx ] );

	m_update_program->useProgram ();

	glUniform1f ( kUniLoc_C, m_wave_speed );
	glUniform1f ( kUniLoc_dt, m_dt );

	glDispatchCompute ( m_n, 1, 1 );

	glMemoryBarrier ( GL_SHADER_STORAGE_BARRIER_BIT );

	glUseProgram ( 0 );

	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kBindIdx_READ, 0 );
	glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kBindIdx_WRITE, 0 );

	m_write_idx = readIdx;
}

bool WaveSystem1D::initBuffers ()
{
	if ( m_vertices_ssbo [ 0 ] ) return false; // return false if the buffers haven't been destroyed // TODO: should this be an assert?

	const size_t kSizeVertices = m_n * sizeof ( Wave1DVert );

	glCreateBuffers ( 2, m_vertices_ssbo );
	glNamedBufferStorage ( m_vertices_ssbo [ 0 ], kSizeVertices, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glNamedBufferStorage ( m_vertices_ssbo [ 1 ], kSizeVertices, nullptr, GL_DYNAMIC_STORAGE_BIT );

#ifndef NDEBUG
	const GLchar* kLabelWaveSystem_SSBO_0 = "WaveSystemSSBO_0";
	const GLchar* kLabelWaveSystem_SSBO_1 = "WaveSystemSSBO_1";

	glObjectLabel ( GL_BUFFER, m_vertices_ssbo [ 0 ], 0, kLabelWaveSystem_SSBO_0 );
	glObjectLabel ( GL_BUFFER, m_vertices_ssbo [ 1 ], 0, kLabelWaveSystem_SSBO_1 );

	printf ( "WaveSystem1D created vertices ssbo's\n" );

	//QOpenGLDebugMessage::createApplicationMessage ( "WaveSystem1D created vertices ssbo's" );
#endif 

	// TODO: implement QOpenGLDebugLogger and check for errors
	return true;
}

bool WaveSystem1D::destroyBuffers ()
{
	if ( m_vertices_ssbo [ 0 ] )
	{
		glDeleteBuffers ( 2, m_vertices_ssbo );
		m_vertices_ssbo [ 0 ] = 0;
		m_vertices_ssbo [ 1 ] = 0;
	}

	// TODO: debugger

	m_write_idx = 1;

	return true;
}

bool WaveSystem1D::fillBuffers ()
{
	if ( m_vertices_ssbo [ 0 ] == 0 ||
		m_vertices_ssbo [ 1 ] == 0 ||
		m_initial_state.empty() ) return false; // something went wrong here

	const size_t kSizeVertices = m_n * sizeof ( Wave1DVert );
	// we just need to fill the read buffer with data here
	glNamedBufferSubData ( m_vertices_ssbo [ 0 ], 0, kSizeVertices, m_initial_state.data () );
	m_write_idx = 1;
	// TODO: debugging

#ifndef NDEBUG
	printf ( "WaveSystem1D filled buffers\n" );
//	QOpenGLDebugMessage::createApplicationMessage ( "WaveSystem1D filled buffers" );

#endif

	// clear the initial state now
//	m_initial_state.clear ();
	return true;
}

bool WaveSystem1D::initShaderProgram ()
{
	//	QOpenGLContext* context = QOpenGLContext::currentContext ();

	//	m_update_program = std::make_unique<QOpenGLShaderProgram> ( context );
//	m_update_program = new QOpenGLShaderProgram;
	GLShader updateShader ( appendToRoot ( "assets/shaders/wave_system1D_update.comp" ).c_str () );
	m_update_program = new GLProgram ( updateShader );

#ifndef NDEBUG
	const GLchar* kLabelWaveSystem_program = "WaveSystemShaderProgram";

	GLuint handle = m_update_program->getHandle ();

	glObjectLabel ( GL_PROGRAM, handle, 0, kLabelWaveSystem_program );

//	QOpenGLDebugMessage::createApplicationMessage ( "WaveSystem1D created update shader program" );
	printf ( "WaveSystem1D created update shader program" );

#endif

	//	success = m_update_program->link ();
	return true;
}

WaveSystem1DCpu::WaveSystem1DCpu ( float dt, float c, uint32_t n )
	: m_dt(dt)
	, m_wave_speed(c)
	, m_n(n)
	, m_write_idx(1)
{}

WaveSystem1DCpu::~WaveSystem1DCpu ()
{}

bool WaveSystem1DCpu::setN ( uint32_t n )
{
	if ( n == m_n ) return true; // no need to change anything
	if ( n < kWaveSystemMinN || n > kWaveSystemMaxN ) return false; // out of bounds
	m_n = n;
	m_wave_state [ 0 ].resize ( m_n );
	m_wave_state [ 1 ].resize ( m_n );
	m_write_idx = 1;
	return true;
}

bool WaveSystem1DCpu::setTimeStep ( float dt )
{
	if ( dt < kWaveSystemMinTimeStep || dt > kWaveSystemMaxTimeStep ) return false;
	m_dt = dt;
	return true;
}

bool WaveSystem1DCpu::setWaveSpeed ( float c )
{
	if ( c < kWaveSystemMinWaveSpeed || c > kWaveSystemMaxWaveSpeed ) return false;
	m_wave_speed = c;
	return true;
}

bool WaveSystem1DCpu::initializeVertices ( const std::function<void ( std::vector<Wave1DVert>&, uint32_t )>& initVerticesFunc )
{
	m_wave_state [ 0 ].resize ( m_n );
	m_wave_state [ 1 ].resize ( m_n );
	initVerticesFunc ( m_wave_state [ 0 ], m_n );
	initVerticesFunc ( m_wave_state [ 1 ], m_n );
	m_write_idx = 1;
	return true;
}

void WaveSystem1DCpu::updateSystem ()
{
	if ( m_wave_state [ 0 ].empty ())
	{
		printf ( "Error: WaveSystem1DCpu update system called before being initialized\n" );
		exit ( 255 );
	}
	for ( uint32_t i = 0; i < m_n; ++i )
	{
		m_wave_state [ m_write_idx ][ i ] = calculateNewVert ( i );
	}

	m_write_idx = ( m_write_idx + 1 ) % 2;
}

float WaveSystem1DCpu::spacialLaplacian ( uint32_t i )
{
	uint32_t readIdx = ( m_write_idx + 1 ) % 2;
	float dx = 1.0f / static_cast< float >( m_n - 1 );
	float invDXSq = 1.0f / ( dx * dx );
	float hCenter = m_wave_state [ readIdx ][ i ].height;
	float hLeft = m_wave_state [ readIdx ][ i - 1 ].height;
	float hRight = m_wave_state [ readIdx ][ i + 1 ].height;
	float lap = hLeft + hRight - 2.0f * hCenter;
	return lap * invDXSq;
}

Wave1DVert WaveSystem1DCpu::calculateNewVert ( uint32_t i )
{
	Wave1DVert v;
	
	if ( i == 0 || i == ( m_n - 1 ) )
	{
		v.height = 0.0f;
		v.velocity = 0.0f;
		return v;
	}

	uint32_t readIdx = ( m_write_idx + 1 ) % 2;

	const float Csq = m_wave_speed * m_wave_speed;
	const float dtSq = m_dt * m_dt;

	float a = Csq * spacialLaplacian ( i );
	float v_old = m_wave_state [ readIdx ][ i ].velocity;
	v.velocity = v_old + a * m_dt;

	float h_old = m_wave_state [ readIdx ][ i ].height;
	v.height = h_old + v_old * m_dt + 0.5 * a * dtSq;

	return v;
}
