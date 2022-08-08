#ifndef __LINE_CANVASGL_H__
#define __LINE_CANVASGL_H__

#include <array>

#include <jcGLframework/GLShader.h>
#include <jcGLframework/GLTexture.h>

#include <jcCommonFramework/scene/VtxData.h>

#include <jcCommonFramework/ResourceString.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

struct VertexData
{
	vec3 pos;
	vec3 n;
	vec2 tc;
};

class GLMeshPVP final
{
public:
	GLuint		vao_;
	uint32_t	numIndices_;

	std::unique_ptr<GLBuffer> bufferIndices_;
	std::unique_ptr<GLBuffer> bufferVertices_;

public:
	GLMeshPVP ( const std::vector<uint32_t>& indices, const void* vertexData, uint32_t verticesSize )
		: numIndices_ ( (uint32_t)indices.size () )
		, bufferIndices_ ( indices.size () ? std::make_unique<GLBuffer> ( indices.size () * sizeof ( uint32_t ), indices.data (), 0 ) : nullptr )
		, bufferVertices_ ( std::make_unique<GLBuffer> ( verticesSize, vertexData, GL_DYNAMIC_STORAGE_BIT ) )
	{
		glCreateVertexArrays ( 1, &vao_ );
		if ( bufferIndices_ )
		{
			glVertexArrayElementBuffer ( vao_, bufferIndices_->getHandle () );
		}
	}

	explicit GLMeshPVP ( const char* filename )
	{
		std::vector<VertexData> vertices;
		std::vector<uint32_t> indices;
		{
			const aiScene* scene = aiImportFile ( filename, aiProcess_Triangulate );
			if ( !scene || !scene->HasMeshes () )
			{
				printf ( "Unable to load '%s'\n", filename );
				exit ( 255 );
			}

			const aiMesh* mesh = scene->mMeshes[0];
			for ( unsigned i = 0; i != mesh->mNumVertices; i++ )
			{
				const aiVector3D v = mesh->mVertices[i];
				const aiVector3D n = mesh->mNormals[i];
				const aiVector3D t = mesh->mTextureCoords[0][i];
				vertices.push_back ( { .pos = vec3 ( v.x, v.y, v.z ), .n = vec3 ( n.x, n.y, n.z ), .tc = vec2 ( t.x, 1.0f - t.y ) } );
			}

			for ( unsigned i = 0; i != mesh->mNumFaces; i++ )
			{
				for ( unsigned j = 0; j != 3; j++ )
				{
					indices.push_back ( mesh->mFaces[i].mIndices[j] );
				}
			}

			aiReleaseImport ( scene );
		}

		const size_t kSizeIndices = sizeof ( uint32_t ) * indices.size ();
		const size_t kSizeVertices = sizeof ( VertexData ) * vertices.size ();
		numIndices_ = (uint32_t)indices.size ();
		bufferIndices_ = std::make_unique<GLBuffer> ( (uint32_t)kSizeIndices, indices.data (), 0 );
		bufferVertices_ = std::make_unique<GLBuffer> ( (uint32_t)kSizeVertices, vertices.data (), 0 );
		glCreateVertexArrays ( 1, &vao_ );
		glVertexArrayElementBuffer ( vao_, bufferIndices_->getHandle () );
	}

	void drawElements ( GLenum mode = GL_TRIANGLES ) const
	{
		glBindVertexArray ( vao_ );
		glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, 1, bufferVertices_->getHandle () );
		glDrawElements ( mode, static_cast<GLsizei>(numIndices_), GL_UNSIGNED_INT, nullptr );
	}

	void drawArrays ( GLenum mode, GLint first, GLint count )
	{
		glBindVertexArray ( vao_ );
		glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, 1, bufferVertices_->getHandle () );
		glDrawArrays ( mode, first, count );
	}

	~GLMeshPVP ()
	{
		glDeleteVertexArrays ( 1, &vao_ );
	}
};

class CanvasGL
{
	static constexpr uint32_t kMaxLines = 1024 * 1024;
	struct VertexData
	{
		vec3 position;
		vec4 color;
	};

	std::vector<VertexData> lines_;
	GLShader shdLinesVertex_ = GLShader ( appendToRoot ( "assets/shaders/GL08_lines.vert" ).c_str () );
	GLShader shdLinesFragment_ = GLShader ( appendToRoot ( "assets/shaders/GL08_lines.frag" ).c_str () );
	GLProgram progLines_ = GLProgram ( shdLinesVertex_, shdLinesFragment_ );
	GLMeshPVP mesh_ = GLMeshPVP ( {}, nullptr, sizeof ( VertexData ) * kMaxLines );

public:
	void line ( const vec3& p1, const vec3& p2, const vec4& c )
	{
		lines_.push_back ( { .position = p1, .color = c } );
		lines_.push_back ( { .position = p2, .color = c } );
	}

	void flush ()
	{
		if ( lines_.empty () ) return;

		assert ( lines_.size () < kMaxLines );

		glNamedBufferSubData ( mesh_.bufferVertices_->getHandle (), 0, uint32_t ( lines_.size () * sizeof ( VertexData ) ), lines_.data () );
		progLines_.useProgram ();
		mesh_.drawArrays ( GL_LINES, 0, (GLint)lines_.size () );

		lines_.clear ();
	}
};

inline void renderCameraFrustrumGL ( CanvasGL& canvas, const mat4& camView, const mat4& camProj, const vec4& color, int numSegments = 1 )
{
	using glm::normalize;

	const vec4 corners[] = {
		vec4 ( -1, -1, -1, 1 ), vec4 ( 1, -1, -1, 1 ),
		vec4 ( 1, 1, -1, 1 ), vec4 ( -1, 1, -1, 1 ),
		vec4 ( -1, -1, 1, 1 ), vec4 ( 1, -1, 1, 1 ),
		vec4 ( 1, 1, 1, 1 ), vec4 ( -1, 1, 1, 1 )
	};

	vec3 pp[8];

	const mat4 invMVP = glm::inverse ( camProj * camView );

	for ( int i = 0; i < 8; i++ )
	{
		const vec4 q = invMVP * corners[i];
		pp[i] = glm::vec3 ( q ) / q.w;
	}

	canvas.line ( pp[0], pp[4], color );
	canvas.line ( pp[1], pp[5], color );
	canvas.line ( pp[2], pp[6], color );
	canvas.line ( pp[3], pp[7], color );
	// near
	canvas.line ( pp[0], pp[1], color );
	canvas.line ( pp[1], pp[2], color );
	canvas.line ( pp[2], pp[3], color );
	canvas.line ( pp[3], pp[0], color );
	// x
	canvas.line ( pp[0], pp[2], color );
	canvas.line ( pp[1], pp[3], color );
	// far
	canvas.line ( pp[4], pp[5], color );
	canvas.line ( pp[5], pp[6], color );
	canvas.line ( pp[6], pp[7], color );
	canvas.line ( pp[7], pp[4], color );
	// x
	canvas.line ( pp[4], pp[6], color );
	canvas.line ( pp[5], pp[7], color );

	const float dimFactor = 0.7f;

	// bottom
	{
		vec3 p1 = pp[0];
		vec3 p2 = pp[1];
		const vec3 s1 = normalize ( pp[4] - pp[0] );
		const vec3 s2 = normalize ( pp[5] - pp[1] );
		for ( int i = 0; i != numSegments; i++, p1 += s1, p2 += s2 )
		{
			canvas.line ( p1, p2, color * dimFactor );
		}
	}
	// left
	{
		vec3 p1 = pp[0];
		vec3 p2 = pp[3];
		const vec3 s1 = normalize ( pp[4] - pp[0] );
		const vec3 s2 = normalize ( pp[7] - pp[3] );
		for ( int i = 0; i != numSegments; i++, p1 += s1, p2 += s2 )
		{
			canvas.line ( p1, p2, color * dimFactor );
		}
	}
	// right
	{
		vec3 p1 = pp[1];
		vec3 p2 = pp[2];
		const vec3 s1 = normalize ( pp[5] - pp[1] );
		const vec3 s2 = normalize ( pp[6] - pp[2] );
		for ( int i = 0; i != numSegments; i++, p1 += s1, p2 += s2 )
		{
			canvas.line ( p1, p2, color * dimFactor );
		}
	}
}

inline void drawBox3dGL ( CanvasGL& canvas, const mat4& m, const BoundingBox& box, const vec4& color )
{
	std::array<vec3, 8> pp = {
		vec3 ( box.max_.x, box.max_.y, box.max_.z ),
		vec3 ( box.max_.x, box.max_.y, box.min_.z ),
		vec3 ( box.max_.x, box.min_.y, box.max_.z ),
		vec3 ( box.max_.x, box.min_.y, box.min_.z ),

		vec3 ( box.min_.x, box.max_.y, box.max_.z ),
		vec3 ( box.min_.x, box.max_.y, box.min_.z ),
		vec3 ( box.min_.x, box.min_.y, box.max_.z ),
		vec3 ( box.min_.x, box.min_.y, box.min_.z )
	};

	for ( auto& p : pp )
	{
		p = vec3 ( m * vec4 ( p, 1.0f ) );
	}

	canvas.line ( pp[0], pp[1], color );
	canvas.line ( pp[2], pp[3], color );
	canvas.line ( pp[4], pp[5], color );
	canvas.line ( pp[6], pp[7], color );

	canvas.line ( pp[0], pp[2], color );
	canvas.line ( pp[1], pp[3], color );
	canvas.line ( pp[4], pp[6], color );
	canvas.line ( pp[5], pp[7], color );

	canvas.line ( pp[0], pp[4], color );
	canvas.line ( pp[1], pp[5], color );
	canvas.line ( pp[2], pp[6], color );
	canvas.line ( pp[3], pp[7], color );
}

#endif // __LINE_CANVASGL_H__