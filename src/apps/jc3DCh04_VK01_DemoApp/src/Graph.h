#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <deque>
#include <limits>

#include "vkRenderers/VulkanCanvas.h"

class LinearGraph
{
private:
	std::deque<float> graph_;
	const size_t maxPoints_;

public:
	explicit LinearGraph ( size_t maxGraphPoints = 256 ) : maxPoints_ ( maxGraphPoints )
	{
	}

	void addPoint ( float value )
	{
		graph_.push_back ( value );
		if ( graph_.size () > maxPoints_ )
		{
			graph_.pop_front ();
		}
	}

	void renderGraph ( VulkanCanvas& c, const glm::vec4& color = vec4 ( 1.0f ) ) const
	{
		float minfps = std::numeric_limits<float>::max ();
		float maxfps = std::numeric_limits<float>::min ();

		for ( float f : graph_ )
		{
			if ( f < minfps ) minfps = f;
			if ( f > maxfps ) maxfps = f;
		}

		const float range = maxfps - minfps;

		float x = 0.0;
		vec3 p1 = vec3 ( 0.0, 0.0, 0.0 );

		for ( float f : graph_ )
		{
			const float val = (f - minfps) / range;
			const vec3 p2 = vec3 ( x, val * 0.15f, 0.0f );
			x += 1.0f / maxPoints_;
			c.line ( p1, p2, color );
			p1 = p2;
		}
	}
};

#endif // __GRAPH_H__