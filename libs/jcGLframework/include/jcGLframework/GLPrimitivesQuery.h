#ifndef __GL_PRIMITIVES_QUERY_H__
#define __GL_PRIMITIVES_QUERY_H__

#include <glad/gl.h>
#include <jcGLframework/jcGLframework.h>

namespace jcGLframework
{
	class GLPrimitivesQuery
	{
	public:
		GLPrimitivesQuery ( const char* label = nullptr );
		~GLPrimitivesQuery ();

		/** Begin the primitives generated query. Until the query is ended, any primitives that are generated are counted. */
		void beginQuery () const;

		/** Ends primitives generated query query and caches the result - number of primitives generated. */
		void endQuery ();

		/** Returns the number of primitives generated.  */
		GLint getNumPrimitivesGenerated () const;

		/** Helper method that returns true if any primitives have been generated.  */
		bool anyPrimitivesGenerated () const;

	private:
		GLuint queryID_ { 0 };
		GLint primitivesGenerated_ { 0 };
	};
}

#endif // !__GL_PRIMITIVES_QUERY_H__
