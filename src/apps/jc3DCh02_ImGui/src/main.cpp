#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>

#include <helpers/RootDir.h>

using glm::mat4;

int main ( void )
{
	glfwSetErrorCallback (
		[]( int err, const char* description )
		{
			fprintf ( stderr, "Error: %s\n", description );
		}
	);

	if ( !glfwInit () )
		exit ( EXIT_FAILURE );

	glfwWindowHint ( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint ( GLFW_CONTEXT_VERSION_MINOR, 6 );
	glfwWindowHint ( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

	GLFWwindow* window = glfwCreateWindow ( 1280, 720, "IMGUI example", nullptr, nullptr );
	if ( !window )
	{
		glfwTerminate ();
		exit ( EXIT_FAILURE );
	}

	/// SETUP callbacks
	glfwSetKeyCallback (
		window,
		[]( GLFWwindow* window, int key, int scancode, int action, int mods )
		{
			if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
				glfwSetWindowShouldClose ( window, GLFW_TRUE );
		}
	);

	glfwSetCursorPosCallback (
		window,
		[]( auto* window, double x, double y )
		{
			ImGui::GetIO ().MousePos = ImVec2 ( (float)x, (float)y );
		}
	);

	glfwSetMouseButtonCallback (
		window,
		[]( auto* window, int button, int action, int mods )
		{
			auto& io = ImGui::GetIO ();
			const int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
			io.MouseDown[idx] = action == GLFW_PRESS;
		}
	);

	glfwMakeContextCurrent ( window );
	//	gladLoadGL(glfwGetProcAddress);
	gladLoadGL ();
	glfwSwapInterval ( 1 );

	/// SETUP vertex and element buffers and associate with vertex array object
	GLuint vao;
	glCreateVertexArrays ( 1, &vao );

	GLuint handleVBO;
	glCreateBuffers ( 1, &handleVBO );
	glNamedBufferStorage ( handleVBO, 128 * 1024, nullptr, GL_DYNAMIC_STORAGE_BIT );

	GLuint handleElements;
	glCreateBuffers ( 1, &handleElements );
	glNamedBufferStorage ( handleElements, 256 * 1024, nullptr, GL_DYNAMIC_STORAGE_BIT );

	glVertexArrayElementBuffer ( vao, handleElements );
	glVertexArrayVertexBuffer ( vao, 0, handleVBO, 0, sizeof ( ImDrawVert ) );

	glEnableVertexArrayAttrib ( vao, 0 );
	glEnableVertexArrayAttrib ( vao, 1 );
	glEnableVertexArrayAttrib ( vao, 2 );

	glVertexArrayAttribFormat ( vao, 0, 2, GL_FLOAT, GL_FALSE, IM_OFFSETOF ( ImDrawVert, pos ) );
	glVertexArrayAttribFormat ( vao, 1, 2, GL_FLOAT, GL_FALSE, IM_OFFSETOF ( ImDrawVert, uv ) );
	glVertexArrayAttribFormat ( vao, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, IM_OFFSETOF ( ImDrawVert, col ) );

	glVertexArrayAttribBinding ( vao, 0, 0 );
	glVertexArrayAttribBinding ( vao, 1, 0 );
	glVertexArrayAttribBinding ( vao, 2, 0 );

	glBindVertexArray ( vao );

	/// SETUP shader programs
	const GLchar* shaderCodeVertex = R"(
		#version 460 core
		layout (location = 0) in vec2 Position;
		layout (location = 1) in vec2 UV;
		layout (location = 2) in vec4 Color;
		layout(std140, binding = 0) uniform PerFrameData
		{
			uniform mat4 MVP;
		};
		out vec2 Frag_UV;
		out vec4 Frag_Color;
		void main()
		{
			Frag_UV = UV;
			Frag_Color = Color;
			gl_Position = MVP * vec4(Position.xy, 0, 1);
		}
	)";

	const GLchar* shaderCodeFragment = R"(
		#version 460 core
		in vec2 Frag_UV;
		in vec4 Frag_Color;
		layout (binding = 0) uniform sampler2D Texture;
		layout (location = 0) out vec4 Out_Color;
		void main()
		{
			Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
		}
	)";


#ifdef _DEBUG
	int success;
	char infoLog[512];
#endif // _DEBUG

	const GLuint shaderVertex = glCreateShader ( GL_VERTEX_SHADER );
	glShaderSource ( shaderVertex, 1, &shaderCodeVertex, nullptr );
	glCompileShader ( shaderVertex );

#ifdef _DEBUG
	glGetShaderiv ( shaderVertex, GL_COMPILE_STATUS, &success );

	if ( !success )
	{
		glGetShaderInfoLog ( shaderVertex, 512, nullptr, infoLog );
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	} else
	{
		std::cout << "SUCCESS::SHADER::VERTEX::COMPILATION_SUCCESS!" << std::endl;
	}
#endif // _DEBUG

	const GLuint shaderFragment = glCreateShader ( GL_FRAGMENT_SHADER );
	glShaderSource ( shaderFragment, 1, &shaderCodeFragment, nullptr );
	glCompileShader ( shaderFragment );

#ifdef _DEBUG
	glGetShaderiv ( shaderFragment, GL_COMPILE_STATUS, &success );

	if ( !success )
	{
		glGetShaderInfoLog ( shaderFragment, 512, nullptr, infoLog );
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	} else
	{
		std::cout << "SUCCESS::SHADER::FRAGMENT::COMPILATION_SUCCESS!" << std::endl;
	}
#endif // _DEBUG

	const GLuint program = glCreateProgram ();
	glAttachShader ( program, shaderVertex );
	glAttachShader ( program, shaderFragment );
	glLinkProgram ( program );

#ifdef _DEBUG
	glGetProgramiv ( program, GL_LINK_STATUS, &success );
	if ( !success )
	{
		glGetProgramInfoLog ( program, 512, nullptr, infoLog );
		std::cout << "ERROR:SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	} else
	{
		std::cout << "SUCCESS::SHADER::PROGRAM::LINKING_SUCCESS!" << std::endl;
	}
#endif // _DEBUG

	glUseProgram ( program );

	/// SETUP uniform buffers for MVP matrix
	GLuint perFrameDataBuffer;
	glCreateBuffers ( 1, &perFrameDataBuffer );
	glNamedBufferStorage ( perFrameDataBuffer, sizeof ( mat4 ), nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferBase ( GL_UNIFORM_BUFFER, 0, perFrameDataBuffer );

	/// SETUP ImGUI

	ImGui::CreateContext ();

	ImGuiIO& io = ImGui::GetIO ();
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	// Build texture atlas
	ImFontConfig cfg = ImFontConfig ();
	cfg.FontDataOwnedByAtlas = false;
	cfg.RasterizerMultiply = 1.5f;
	cfg.SizePixels = 720.0f / 32.0f;
	cfg.PixelSnapH = true;
	cfg.OversampleH = 4;
	cfg.OversampleV = 4;

	// Load the font file
	std::string assetLoc = ROOT_DIR + std::string("assets/fonts/OpenSans-Light.ttf");
	ImFont* Font = io.Fonts->AddFontFromFileTTF ( assetLoc.c_str(), cfg.SizePixels, &cfg );

	// Load fonts into OpenGL
	uint8_t* pixels = nullptr;
	GLint width, height;
	io.Fonts->GetTexDataAsRGBA32 ( &pixels, &width, &height );

	GLuint texture;
	glCreateTextures ( GL_TEXTURE_2D, 1, &texture );
	glTextureParameteri ( texture, GL_TEXTURE_MAX_LEVEL, 0 );
	glTextureParameteri ( texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTextureParameteri ( texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTextureStorage2D ( texture, 1, GL_RGBA8, width, height );
	// scanlines in ImGui bitmap are not padded, so disable pixel unpack alignment
	glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
	glTextureSubImage2D ( texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
	glBindTextures ( 0, 1, &texture );

	// pass the texture handle to ImGui for use in subsequent draw calls
	io.Fonts->TexID = (ImTextureID)(intptr_t)texture;
	io.FontDefault = Font;
	io.DisplayFramebufferScale = ImVec2 ( 1, 1 );

	// All ImGui graphics should be rendered with blending and scissor test on, and depth test and backface culling disabled
	glEnable ( GL_BLEND );
	glBlendEquation ( GL_FUNC_ADD );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable ( GL_CULL_FACE );
	glDisable ( GL_DEPTH_TEST );
	glEnable ( GL_SCISSOR_TEST );
	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );


	while ( !glfwWindowShouldClose ( window ) )
	{
		int width, height;
		glfwGetFramebufferSize ( window, &width, &height );

		glViewport ( 0, 0, width, height );
		glClear ( GL_COLOR_BUFFER_BIT );

		// Tell ImGui the current window dimensions, start a new frame, and render a demo UI window
		ImGuiIO& io = ImGui::GetIO ();
		io.DisplaySize = ImVec2 ( (float)width, (float)height );
		ImGui::NewFrame ();
		ImGui::ShowDemoWindow ();

		// Generate ImGui geometry data
		ImGui::Render ();

		// Retrieve geometry data
		const ImDrawData* draw_data = ImGui::GetDrawData ();

		// Construct orthographic projection matrix using left, right, top, and bottom clipping planes
		const float L = draw_data->DisplayPos.x;
		const float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
		const float T = draw_data->DisplayPos.y;
		const float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
		const mat4 orthoProjection = glm::ortho ( L, R, B, T );

		glNamedBufferSubData ( perFrameDataBuffer, 0, sizeof ( mat4 ), glm::value_ptr ( orthoProjection ) );

		// go through all ImGui command lists, update content of index and vertex buffers, and invoke rendering commands
		for ( int n = 0; n < draw_data->CmdListsCount; n++ )
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];

			// Each ImGui command list has vertex and index data associated with it. Update appropriate OpenGL buffers.
			glNamedBufferSubData ( handleVBO, 0, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof ( ImDrawVert ), cmd_list->VtxBuffer.Data );
			glNamedBufferSubData ( handleElements, 0, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof ( ImDrawIdx ), cmd_list->IdxBuffer.Data );

			// Rendering commands are stored inside the command buffer. Iterate over them and render the actual geometry.
			for ( int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++ )
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				const ImVec4 cr = pcmd->ClipRect;
				// void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) 
				glScissor ( (GLint)cr.x, (GLint)( height - cr.w ), (GLsizei)( cr.z - cr.x ), (GLsizei)( cr.w - cr.y ) );
				glBindTextureUnit ( 0, (GLuint)(intptr_t)pcmd->TextureId );
				glDrawElementsBaseVertex ( GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, (void*)(intptr_t)( pcmd->IdxOffset * sizeof ( ImDrawIdx ) ), (GLint)pcmd->VtxOffset );
			}
		}

		glScissor ( 0, 0, width, height );


		glfwSwapBuffers ( window );
		glfwPollEvents ();
	}

	// Teardown
	ImGui::DestroyContext ();

	glfwDestroyWindow ( window );
	glfwTerminate ();

	exit ( EXIT_SUCCESS );
}