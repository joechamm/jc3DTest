#pragma once

#include <glad/gl.h>
#include <vector>
#include <functional>
#include <memory>
#include <jcGLframework/GLShader.h>

/**
 * @brief WaveSystem1D holds the height and velocity values of 'n' equally spaced points in a ping-pong buffer
 * scenario where the write_idx defines the current buffer index to write to, and values are read from the
 * (write_idx + 1) % 2 index. The one-dimensional wave equation d^2u / dt^2 = c^2 d^2u /dx^2 describes our system
 * evolution through time, where u(x,t) is the height of the wave at position 'x' and time 't'. We store the time-step
 * in the member variable m_dt and the wave speed 'c' in the member variable m_wave_speed. Currently we clamp the
 * boundary to 0 (unit interval topology), but it would be easy to switch to the unit circle topology by identifying
 * the end points.
 *
 * Store the initial height and velocity values here before transferring them to buffer.
 * They are initialized by a function taking QList reference of vertices and the size of the list.
 *
 * The order of events after creation (not on the stack, for right now) should be:
 *
 *		1: initializeVertices
 *		2: (loop)
 *			a) updateSystem
 *			b) getReadBufferHandle (should be bound to index 1 - read index for rendering)
*/

// store the point information in the 'Wave1DVert' struct
struct Wave1DVert
{
	float height;
	float velocity;
};

class WaveSystem1D
{

public:
	WaveSystem1D ( float dt = 0.1, float c = 1.0, uint32_t n = 256 );
	virtual ~WaveSystem1D ();

	// setters return false for invalid parameters
	bool setN ( uint32_t n );
	bool setTimeStep ( float dt );
	bool setWaveSpeed ( float c );

	uint32_t getN () const { return m_n; }
	float getTimeStep () const { return m_dt; }
	float getWaveSpeed () const { return m_wave_speed; }

	/** @brief initializeVertices should take a function that accepts a properly sized QList<Vertex> reference (our initial state) and the size of the list to fill */
	bool initializeVertices ( const std::function<void ( std::vector<Wave1DVert>&, uint32_t )>& initVerticesFunc );

	void updateSystem ();

	GLuint getReadBufferHandle () const
	{
		const uint32_t readIdx = ( m_write_idx + 1 ) % 2;
		return m_vertices_ssbo [ readIdx ];
	}

	GLuint getWriteBufferHandle () const
	{
		return m_vertices_ssbo [ m_write_idx ];
	}

	const std::vector<Wave1DVert>& getInitialState () const { return m_initial_state; }

	void clearInitialState () { m_initial_state.clear (); }

private:
	bool initBuffers ();
	bool destroyBuffers ();
	bool fillBuffers ();

	bool initShaderProgram ();

	GLProgram*								m_update_program = nullptr;

	float									m_dt;				// time step  'dt'
	float									m_wave_speed;		// wave speed 'c'
	uint32_t								m_n;				// number of points 'n' to sample in our space

	std::vector<Wave1DVert>					m_initial_state; // use tightly packed data here

	uint32_t								m_write_idx;

	GLuint									m_vertices_ssbo [ 2 ];  // GPU store for vertex data in a ping-pong paradigm
};

class WaveSystem1DCpu
{

public:
	WaveSystem1DCpu ( float dt = 0.1, float c = 1.0, uint32_t n = 256 );
	virtual ~WaveSystem1DCpu ();

	// setters return false for invalid parameters
	bool setN ( uint32_t n );
	bool setTimeStep ( float dt );
	bool setWaveSpeed ( float c );

	uint32_t getN () const { return m_n; }
	float getTimeStep () const { return m_dt; }
	float getWaveSpeed () const { return m_wave_speed; }

	/** @brief initializeVertices should take a function that accepts a properly sized QList<Vertex> reference (our initial state) and the size of the list to fill */
	bool initializeVertices ( const std::function<void ( std::vector<Wave1DVert>&, uint32_t )>& initVerticesFunc );

	void updateSystem ();

	const std::vector<Wave1DVert>& getReadState () const { return m_wave_state[(m_write_idx + 1) % 2]; }
	const std::vector<Wave1DVert>& getWriteState () const { return m_wave_state [ m_write_idx ]; }

private:
	float spacialLaplacian ( uint32_t i );
	Wave1DVert calculateNewVert ( uint32_t i );

	float									m_dt;				// time step  'dt'
	float									m_wave_speed;		// wave speed 'c'
	uint32_t								m_n;				// number of points 'n' to sample in our space

	std::vector<Wave1DVert>					m_wave_state[2]; // use tightly packed data here

	uint32_t								m_write_idx;
};