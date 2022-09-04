#ifndef __GL_TIMER_QUERY_H__
#define __GL_TIMER_QUERY_H__

#include <glad/gl.h>
#include <jcGLframework/jcGLframework.h>

namespace jcGLframework
{
	class GLTimerQuery
	{
	public:
		GLTimerQuery ( const char* beginLabel = nullptr, const char* endLabel = nullptr );
		~GLTimerQuery ();

		void beginQuery () const;
		void endQuery () const;

		GLuint64 getElapsedTime ();

	private:
		GLuint queryID_ [ 2 ] = { 0, 0 };
		GLuint64 startTime_ { 0 };
		GLuint64 endTime_ { 0 };
	};
}

#endif // !__GL_TIMER_QUERY_H__
