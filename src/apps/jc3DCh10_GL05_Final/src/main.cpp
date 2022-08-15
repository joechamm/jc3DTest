#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <jcGLframework/GLFWApp.h>
#include <jcGLframework/GLShader.h>
#include <jcGLframework/GLSceneDataLazy.h>
#include <jcGLframework/GLFramebuffer.h>
#include <jcGLframework/LineCanvasGL.h>
#include <jcGLframework/UtilsGLImGui.h>
#include <jcGLframework/GLMeshIndirect.h>
#include <jcGLframework/GLSkyboxRenderer.h>
#include <jcCommonFramework/UtilsMath.h>
#include <jcCommonFramework/UtilsFPS.h>
#include <jcCommonFramework/Camera.h>
#include <jcCommonFramework/scene/VtxData.h>
#include <jcCommonFramework/ResourceString.h>

using glm::mat4;
using glm::vec4;
using glm::vec3;
using glm::vec2;

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 light;
	vec4 cameraPos;
	vec4 frustumPlanes [ 6 ];
	vec4 frustumCorners [ 8 ];
	uint32_t numShapesToCull;
};

struct SSAOParams
{
	float scale_ = 1.5f;
	float bias_ = 0.15f;
	float zNear_ = 0.1f;
	float zFar_ = 1000.0f;
	float radius_ = 0.05f;
	float attScale_ = 1.0f;
	float distScale_ = 0.6f;
} g_SSAOParams;

static_assert( sizeof ( SSAOParams ) <= sizeof ( PerFrameData ) );

struct HDRParams
{
	float exposure_ = 0.9f;
	float maxWhite_ = 1.17f;
	float bloomStrength_ = 1.1f;
	float adaptationSpeed_ = 0.1f;
} g_HDRParams;

static_assert( sizeof ( HDRParams ) <= sizeof ( PerFrameData ) );

struct MouseState
{
	vec2 pos = vec2 ( 0.0f );
	bool pressedLeft = false;
} mouseState;

CameraPositioner_FirstPerson positioner ( vec3 ( -10.0f, 3.0f, 3.0f ), vec3 ( 0.0f, 0.0f, -1.0f ), vec3 ( 0.0f, 1.0f, 0.0f ) );
Camera camera ( positioner );

mat4 g_CullingView = camera.getViewMatrix ();

bool g_EnableGPUCulling = true;
bool g_FreezeCullingView = false;
bool g_DrawOpaque = true;
bool g_DrawTransparent = true;
bool g_DrawGrid = true;
bool g_EnableSSAO = true;
bool g_EnableBlur = true;
bool g_EnableHDR = true;
bool g_DrawBoxes = false;
bool g_EnableShadows = true;
bool g_ShowLightFrustum = false;
float g_LightTheta = 0.0f;
float g_LightPhi = 0.0f;

int main ()
{
	GLFWApp app;

	// grid
	GLShader shdGridVert ( appendToRoot ( "assets/shaders/GL10_grid.vert" ).c_str () );
	GLShader shdGridFrag ( appendToRoot ( "assets/shaders/GL10_grid.frag" ).c_str () );
	GLProgram progGrid( shdGridVert, shdGridFrag );

	// scene
	GLShader shdSceneVert ( appendToRoot ( "assets/shaders/GL10_scene_IBL.vert" ).c_str () );
	GLShader shdSceneFrag ( appendToRoot ( "assets/shaders/GL10_scene_IBL.frag" ).c_str () );
	GLProgram progScene ( shdSceneVert, shdSceneFrag );

	// generic postprocessing
	GLShader shdFullScreenQuadVert ( appendToRoot ( "assets/shaders/GL10_FullScreenQuad.vert" ).c_str () );
	
	// OIT
	GLShader shdFragOIT ( appendToRoot ( "assets/shaders/GL10_mesh_oit.frag" ).c_str () );
	GLProgram progOIT ( shdSceneVert, shdFragOIT );

	GLShader shdFragCombineOIT ( appendToRoot ( "assets/shaders/GL10_OIT.frag" ).c_str () );
	GLProgram progCombineOIT ( shdFullScreenQuadVert, shdFragCombineOIT );

	// GPU culling
	GLShader shdCompCulling ( appendToRoot ( "assets/shaders/GL10_FrustumCulling.comp" ).c_str () );
	GLProgram progCulling ( shdCompCulling );

	// SSAO
	GLShader shdSSAOFrag ( appendToRoot ( "assets/shaders/GL10_SSAO.frag" ).c_str () );
	GLShader shdCombineSSAOFrag ( appendToRoot ( "assets/shaders/GL10_SSAO_combine.frag" ).c_str () );
	GLProgram progSSAO ( shdFullScreenQuadVert, shdSSAOFrag );
	GLProgram progCombineSSAO ( shdFullScreenQuadVert, shdCombineSSAOFrag );

	// blur
	GLShader shdBlurXFrag ( appendToRoot ( "assets/shaders/GL10_BlurX.frag" ).c_str () );
	GLShader shdBlurYFrag ( appendToRoot ( "assets/shaders/GL10_BlurY.frag" ).c_str () );
	GLProgram progBlurX ( shdFullScreenQuadVert, shdBlurXFrag );
	GLProgram progBlurY ( shdFullScreenQuadVert, shdBlurYFrag );
	
	// HDR
	GLShader shdCombineHDR ( appendToRoot ( "assets/shaders/GL10_HDR.frag" ).c_str () );
	GLProgram progCombineHDR ( shdFullScreenQuadVert, shdCombineHDR );

	GLShader shdToLuminance ( appendToRoot ( "assets/shaders/GL10_ToLuminance.frag" ).c_str () );
	GLProgram progToLuminance ( shdFullScreenQuadVert, shdToLuminance );
	
	GLShader shdBrightPass ( appendToRoot ( "assets/shaders/GL10_BrightPass.frag" ).c_str () );
	GLProgram progBrightPass ( shdFullScreenQuadVert, shdBrightPass );

	GLShader shdAdaptation ( appendToRoot ( "assets/shaders/GL10_Adaptation.comp" ).c_str () );
	GLProgram progAdaptation ( shdAdaptation );

	// shadows
	GLShader shdShadowVert ( appendToRoot ( "assets/shaders/GL10_Shadow.vert" ).c_str () );
	GLShader shdShadowFrag ( appendToRoot ( "assets/shaders/GL10_Shadow.frag" ).c_str () );
	GLProgram progShadow ( shdShadowVert, shdShadowFrag );

	const GLuint kMaxNumObjects = 128 * 1024;
	const GLsizeiptr kUniformBufferSize = sizeof ( PerFrameData );
	const GLsizeiptr kBoundingBoxesBufferSize = sizeof ( BoundingBox ) * kMaxNumObjects;
	const GLuint kBufferIndex_BoundingBoxes = kBufferIndex_PerFrameUniforms + 1;
	const GLuint kBufferIndex_DrawCommands = kBufferIndex_PerFrameUniforms + 2;
	const GLuint kBufferIndex_NumVisibleMeshes = kBufferIndex_PerFrameUniforms + 3;

	GLBuffer perFrameDataBuffer ( kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	glBindBufferRange ( GL_UNIFORM_BUFFER, kBufferIndex_PerFrameUniforms, perFrameDataBuffer.getHandle (), 0, kUniformBufferSize );
	
	GLBuffer boundingBoxesBuffer ( kBoundingBoxesBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT );
	GLBuffer numVisibleMeshesBuffer ( sizeof ( uint32_t ), nullptr, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );
	volatile uint32_t* numVisibleMeshesPtr = ( uint32_t* ) glMapNamedBuffer ( numVisibleMeshesBuffer.getHandle (), GL_READ_WRITE );
	assert ( numVisibleMeshesPtr );
	if ( !numVisibleMeshesPtr )
	{
		printf ( "numVisibleMeshesPtr == nullptr\n" );
		exit ( 255 );
	}

	glClearColor ( 1.0f, 1.0f, 1.0f, 1.0f );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable ( GL_DEPTH_TEST );

	GLSceneDataLazy sceneData ( appendToRoot ( "assets/meshes/bistro_all.meshes" ).c_str (), appendToRoot ( "assets/meshes/bistro_all.scene" ).c_str (), appendToRoot ( "assets/meshes/bistro_all.materials" ).c_str () );
	GLMeshIndirect mesh ( sceneData );

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
			if ( key == GLFW_KEY_W )
				positioner.movement_.forward_ = pressed;
			if ( key == GLFW_KEY_S )
				positioner.movement_.backward_ = pressed;
			if ( key == GLFW_KEY_A )
				positioner.movement_.left_ = pressed;
			if ( key == GLFW_KEY_D )
				positioner.movement_.right_ = pressed;
			if ( key == GLFW_KEY_1 )
				positioner.movement_.up_ = pressed;
			if ( key == GLFW_KEY_2 )
				positioner.movement_.down_ = pressed;
			if ( key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT )
				positioner.movement_.fastSpeed_ = pressed;
			if ( key == GLFW_KEY_SPACE )
				positioner.setUpVector ( vec3 ( 0.0f, 1.0f, 0.0f ) );
			if ( key == GLFW_KEY_P && pressed )
				g_FreezeCullingView = !g_FreezeCullingView;
		}
	);

	positioner.maxSpeed_ = 1.0f;

	GLSkyboxRenderer skybox;
	ImGuiGLRenderer rendererUI;
	CanvasGL canvas;
	
	GLTexture rotationPattern ( GL_TEXTURE_2D, appendToRoot ( "assets/images/rot_texture.bmp" ).c_str () );

	GLIndirectBuffer meshesOpaque ( sceneData.shapes_.size () );
	GLIndirectBuffer meshesTransparent ( sceneData.shapes_.size () );
	
	auto isTransparent = [&sceneData] ( const DrawElementsIndirectCommand& c )
	{
		const auto mtlIndex = c.baseInstance_ & 0xffff;
		const auto& mtl = sceneData.materials_ [ mtlIndex ];
		return ( mtl.flags_ & sMaterialFlags_Transparent ) > 0;
	};

	mesh.bufferIndirect_.selectTo ( meshesOpaque, [&isTransparent] ( const DrawElementsIndirectCommand& c ) -> bool
		{
			return !isTransparent ( c );
		} );

	mesh.bufferIndirect_.selectTo ( meshesTransparent, [&isTransparent] ( const DrawElementsIndirectCommand& c ) -> bool
		{
			return isTransparent ( c );
		} );

	struct TransparentFragment
	{
		float R, G, B, A;
		float Depth;
		uint32_t Next;
	};

	int width, height;
	glfwGetFramebufferSize ( app.getWindow (), &width, &height );
	GLFramebuffer opaqueFramebuffer ( width, height, GL_RGBA16F, GL_DEPTH_COMPONENT24 );
	GLFramebuffer framebuffer ( width, height, GL_RGBA16F, GL_DEPTH_COMPONENT24 );
	GLFramebuffer luminance ( 64, 64, GL_RGBA16F, 0 );
	GLFramebuffer brightPass ( 256, 256, GL_RGBA16F, 0 );
	GLFramebuffer bloom1 ( 256, 256, GL_RGBA16F, 0 );
	GLFramebuffer bloom2 ( 256, 256, GL_RGBA16F, 0 );
	GLFramebuffer ssao ( 1024, 1024, GL_RGBA8, 0 );
	GLFramebuffer blur ( 1024, 1024, GL_RGBA8, 0 );
	
	GLuint luminance1x1;
	glGenTextures ( 1, &luminance1x1 );
	glTextureView ( luminance1x1, GL_TEXTURE_2D, luminance.getTextureColor ().getHandle (), GL_RGBA16F, 6, 1, 0, 1 );
	// ping-pong textures for light adaptation
	GLTexture luminance1 ( GL_TEXTURE_2D, 1, 1, GL_RGBA16F );
	GLTexture luminance2 ( GL_TEXTURE_2D, 1, 1, GL_RGBA16F );
	const GLTexture* luminances [] = { &luminance1, &luminance2 };
	const vec4 brightPixel ( vec3 ( 50.0f ), 1.0f );
	glTextureSubImage2D ( luminance1.getHandle (), 0, 0, 0, 1, 1, GL_RGBA, GL_FLOAT, glm::value_ptr ( brightPixel ) );
	
	const uint32_t kMaxOITFragments = 16 * 1024 * 1024;
	const GLuint kBufferIndex_TransparencyLists = kBufferIndex_Materials + 1;
	GLBuffer oitAtomicCounter ( sizeof ( uint32_t ), nullptr, GL_DYNAMIC_STORAGE_BIT );
	GLBuffer oitTransparencyLists ( sizeof ( TransparentFragment )* kMaxOITFragments, nullptr, GL_DYNAMIC_STORAGE_BIT );

	GLTexture oitHeads ( GL_TEXTURE_2D, width, height, GL_R32UI );
	glBindImageTexture ( 0, oitHeads.getHandle (), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI );
	glBindBufferBase ( GL_ATOMIC_COUNTER_BUFFER, 0, oitAtomicCounter.getHandle () );

	auto clearTransparencyBuffers = [&oitAtomicCounter, &oitHeads] ()
	{
		const uint32_t minusOne = 0xFFFFFFFF;
		const uint32_t zero = 0;
		glClearTexImage ( oitHeads.getHandle (), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &minusOne );
		glNamedBufferSubData ( oitAtomicCounter.getHandle (), 0, sizeof ( uint32_t ), &zero );
	};

	std::vector<BoundingBox> reorderedBoxes;
	reorderedBoxes.reserve ( sceneData.shapes_.size () );

	// pretransform bounding boxes to world space
	for ( const auto& c : sceneData.shapes_ )
	{
		const mat4 model = sceneData.scene_.globalTransforms_ [ c.transformIndex ];
		reorderedBoxes.push_back ( sceneData.meshData_.boxes_ [ c.meshIndex ] );
		reorderedBoxes.back ().transform ( model );
	};

	glNamedBufferSubData ( boundingBoxesBuffer.getHandle (), 0, reorderedBoxes.size () * sizeof ( BoundingBox ), reorderedBoxes.data () );

	BoundingBox bigBox = reorderedBoxes.front ();
	for ( const auto& b : reorderedBoxes )
	{
		bigBox.combinePoint ( b.min_ );
		bigBox.combinePoint ( b.max_ );
	}

	FramesPerSecondCounter fpsCounter ( 0.5f );

	auto imGuiPushFlagsAndStyles = [] ( bool value )
	{
		ImGui::PushItemFlag ( ImGuiItemFlags_Disabled, !value );
		ImGui::PushStyleVar ( ImGuiStyleVar_Alpha, ImGui::GetStyle ().Alpha * value ? 1.0f : 0.2f );
	};

	auto imGuiPopFlagsAndStyles = [] ()
	{
		ImGui::PopItemFlag ();
		ImGui::PopStyleVar ();
	};

	GLFramebuffer shadowMap ( 8192, 8192, GL_R8, GL_DEPTH_COMPONENT24 );
	const GLint swizzleMask [] = { GL_RED, GL_RED, GL_RED, GL_ONE };
	glTextureParameteriv ( shadowMap.getTextureColor ().getHandle (), GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );
	glTextureParameteriv ( shadowMap.getTextureColor ().getHandle (), GL_TEXTURE_SWIZZLE_RGBA, swizzleMask );

	GLsync fenceCulling = nullptr;

	while ( !glfwWindowShouldClose ( app.getWindow () ) )
	{
		fpsCounter.tick ( app.getDeltaSeconds () );

		if ( sceneData.uploadLoadedTextures () )
		{
			mesh.updateMaterialsBuffer ( sceneData );
		}

		positioner.update ( app.getDeltaSeconds (), mouseState.pos, mouseState.pressedLeft );

		int width, height;
		glfwGetFramebufferSize ( app.getWindow (), &width, &height );
		const float ratio = width / ( float ) height;

		glViewport ( 0, 0, width, height );
		glClearNamedFramebufferfv ( opaqueFramebuffer.getHandle (), GL_COLOR, 0, glm::value_ptr ( vec4 ( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
		glClearNamedFramebufferfi ( opaqueFramebuffer.getHandle (), GL_DEPTH_STENCIL, 0, 1.0f, 0 );

		if ( !g_FreezeCullingView )
		{
			g_CullingView = camera.getViewMatrix ();
		}

		const mat4 proj = glm::perspective ( 45.0f, ratio, 0.1f, 1000.0f );
		const mat4 view = camera.getViewMatrix ();

		// calculate light parameters for shadow mapping
		const mat4 rot1 = glm::rotate ( mat4 ( 1.0f ), glm::radians ( g_LightTheta ), vec3 ( 0.0f, 0.0f, 1.0f ) );
		const mat4 rot2 = glm::rotate(rot1, glm::radians(g_LightPhi), vec3(1.0f, 0.0f, 0.0f));
		const vec3 lightDir = glm::normalize ( vec3 ( rot2 * vec4 ( 0.0f, -1.0f, 0.0f, 1.0f ) ) );
		const mat4 lightView = glm::lookAt(vec3(0.0f), lightDir, vec3(0.0f, 0.0f, 1.0f));
		const BoundingBox box = bigBox.getTransformed ( lightView );
		const mat4 lightProj = glm::ortho ( box.min_.x, box.max_.x, box.min_.y, box.max_.y, -box.max_.z, -box.min_.z );

		PerFrameData perFrameData = {
			.view = view,
			.proj = proj,
			.light = lightProj * lightView,
			.cameraPos = vec4 ( camera.getPosition (), 1.0f )
		};

		getFrustumPlanes ( proj * g_CullingView, perFrameData.frustumPlanes );
		getFrustumCorners ( proj * g_CullingView, perFrameData.frustumCorners );

		clearTransparencyBuffers ();

		// cull
		{
			*numVisibleMeshesPtr = 0;
			progCulling.useProgram ();
			glMemoryBarrier ( GL_BUFFER_UPDATE_BARRIER_BIT );
			glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kBufferIndex_BoundingBoxes, boundingBoxesBuffer.getHandle () );
			glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kBufferIndex_NumVisibleMeshes, numVisibleMeshesBuffer.getHandle () );
			
			perFrameData.numShapesToCull = g_EnableGPUCulling ? ( uint32_t ) meshesOpaque.drawCommands_.size () : 0u;
			glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, kUniformBufferSize, &perFrameData );
			glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kBufferIndex_DrawCommands, meshesOpaque.getHandle () );
			glDispatchCompute ( 1 + ( GLuint ) meshesOpaque.drawCommands_.size () / 64, 1, 1 );

			perFrameData.numShapesToCull = g_EnableGPUCulling ? ( uint32_t ) meshesTransparent.drawCommands_.size () : 0u;
			glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, kUniformBufferSize, &perFrameData );
			glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kBufferIndex_DrawCommands, meshesTransparent.getHandle () );
			glDispatchCompute ( 1 + ( GLuint ) meshesTransparent.drawCommands_.size () / 64, 1, 1 );

			glMemoryBarrier ( GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT );
			fenceCulling = glFenceSync ( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );			
		}

		glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, kBufferIndex_TransparencyLists, oitTransparencyLists.getHandle () );

		// 1. Render shadow map
		if ( g_EnableShadows )
		{
			glDisable ( GL_BLEND );
			glEnable ( GL_DEPTH_TEST );
			// Calculate light parameters
			const PerFrameData perFrameDataShadows = { .view = lightView, .proj = lightProj };
			glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, kUniformBufferSize, &perFrameDataShadows );
			glClearNamedFramebufferfv ( shadowMap.getHandle (), GL_COLOR, 0, glm::value_ptr ( vec4 ( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
			glClearNamedFramebufferfi ( shadowMap.getHandle (), GL_DEPTH_STENCIL, 0, 1.0f, 0 );
			shadowMap.bind ();
			progShadow.useProgram ();
			mesh.draw ( mesh.bufferIndirect_.drawCommands_.size () );
			shadowMap.unbind ();
			perFrameData.light = lightProj * lightView;
			glBindTextureUnit ( 4, shadowMap.getTextureDepth ().getHandle () );
		}
		else
		{
			// disable shadows
			perFrameData.light = mat4 ( 0.0f );
		}

		glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, kUniformBufferSize, &perFrameData );

		// 1. Render scene
		opaqueFramebuffer.bind ();
		glDisable ( GL_BLEND );
		glEnable ( GL_DEPTH_TEST );
		// 1.0 Cube map
		skybox.draw ();
		// 1.1 Bistro
		if ( g_DrawOpaque )
		{
			progScene.useProgram ();
			mesh.draw ( meshesOpaque.drawCommands_.size (), &meshesOpaque );
		}
		if ( g_DrawGrid )
		{
			glEnable ( GL_BLEND );
			progGrid.useProgram ();
			glDrawArraysInstancedBaseInstance ( GL_TRIANGLES, 0, 6, 1, 0 );
			glDisable ( GL_BLEND );
		}
		if ( g_DrawBoxes )
		{
			DrawElementsIndirectCommand* cmd = mesh.bufferIndirect_.drawCommands_.data ();
			for ( const auto& c : sceneData.shapes_ )
			{
				drawBox3dGL ( canvas, mat4 ( 1.0f ), sceneData.meshData_.boxes_ [ c.meshIndex ], vec4 ( 0.0f, 1.0f, 0.0f, 1.0f ) );
			}
		}
		drawBox3dGL ( canvas, mat4 ( 1.0f ), bigBox, vec4 ( 1.0f, 1.0f, 1.0f, 1.0f ) );
		if ( g_DrawTransparent )
		{
			glBindImageTexture ( 0, oitHeads.getHandle (), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI );
			glDepthMask ( GL_FALSE );
			glColorMask ( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
			progOIT.useProgram ();
			mesh.draw ( meshesTransparent.drawCommands_.size (), &meshesTransparent );
			glFlush ();
			glMemoryBarrier ( GL_SHADER_STORAGE_BARRIER_BIT );
			glDepthMask ( GL_TRUE );
			glColorMask ( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
		}
		if ( g_FreezeCullingView )
		{
			renderCameraFrustumGL ( canvas, g_CullingView, proj, vec4 ( 1.0f, 1.0f, 0.0f, 1.0f ), 100 );
		}
		if ( g_EnableShadows && g_ShowLightFrustum )
		{
			renderCameraFrustumGL ( canvas, lightView, lightProj, vec4 ( 1.0f, 0.0f, 0.0f, 1.0f ), 100 );
			canvas.line ( vec3 ( 0.0f ), lightDir * 100.0f, vec4 ( 0.0f, 0.0f, 1.0f, 1.0f ) );
		}

		canvas.flush ();
		opaqueFramebuffer.unbind ();

		// SSAO 
		if ( g_EnableSSAO )
		{
			glDisable ( GL_DEPTH_TEST );
			glClearNamedFramebufferfv ( ssao.getHandle (), GL_COLOR, 0, glm::value_ptr ( vec4 ( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
			glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, sizeof ( g_SSAOParams ), &g_SSAOParams );
			ssao.bind ();
			progSSAO.useProgram ();
			glBindTextureUnit ( 0, opaqueFramebuffer.getTextureDepth ().getHandle () );
			glBindTextureUnit ( 1, rotationPattern.getHandle () );
			glDrawArrays ( GL_TRIANGLES, 0, 6 );
			ssao.unbind ();
			if ( g_EnableBlur )
			{
				// Blur X
				blur.bind ();
				progBlurX.useProgram ();
				glBindTextureUnit ( 0, ssao.getTextureColor ().getHandle () );
				glDrawArrays ( GL_TRIANGLES, 0, 6 );
				blur.unbind ();
				// Blur Y
				ssao.bind ();
				progBlurY.useProgram ();
				glBindTextureUnit ( 0, blur.getTextureColor ().getHandle () );
				glDrawArrays ( GL_TRIANGLES, 0, 6 );
				ssao.unbind ();
			}

			glClearNamedFramebufferfv ( framebuffer.getHandle (), GL_COLOR, 0, glm::value_ptr ( vec4 ( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
			framebuffer.bind ();
			progCombineSSAO.useProgram ();
			glBindTextureUnit ( 0, opaqueFramebuffer.getTextureColor ().getHandle () );
			glBindTextureUnit ( 1, ssao.getTextureColor ().getHandle () );
			glDrawArrays ( GL_TRIANGLES, 0, 6 );
			framebuffer.unbind ();
		}
		else
		{
			glBlitNamedFramebuffer ( opaqueFramebuffer.getHandle (), framebuffer.getHandle (), 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR );
		}

		// combine OIT
		opaqueFramebuffer.bind ();
		glDisable ( GL_DEPTH_TEST );
		glDisable ( GL_BLEND );
		progCombineOIT.useProgram ();
		glBindTextureUnit ( 0, framebuffer.getTextureColor ().getHandle () );
		glDrawArrays ( GL_TRIANGLES, 0, 6 );
		opaqueFramebuffer.unbind ();
		glBlitNamedFramebuffer ( opaqueFramebuffer.getHandle (), framebuffer.getHandle (), 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR );

		// HDR pipeline
		// pas HDR params to shaders
		if ( g_EnableHDR )
		{
			glNamedBufferSubData ( perFrameDataBuffer.getHandle (), 0, sizeof ( g_HDRParams ), &g_HDRParams );
			// 2.1 Downscale and convert to luminance
			luminance.bind ();
			progToLuminance.useProgram ();
			glBindTextureUnit ( 0, framebuffer.getTextureColor ().getHandle () );
			glDrawArrays ( GL_TRIANGLES, 0, 6 );
			luminance.unbind ();
			glGenerateTextureMipmap ( luminance.getTextureColor ().getHandle () );
			// 2.2 Light adaptation
			glMemoryBarrier ( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );
			progAdaptation.useProgram ();
			glBindImageTexture ( 0, luminances [ 0 ]->getHandle (), 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F );
			glBindImageTexture ( 1, luminance1x1, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F );
			glBindImageTexture ( 2, luminances [ 1 ]->getHandle (), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F );
			glDispatchCompute ( 1, 1, 1 );
			glMemoryBarrier ( GL_TEXTURE_FETCH_BARRIER_BIT );
			// 2.3 Extract bright areas
			brightPass.bind ();
			progBrightPass.useProgram ();
			glBindTextureUnit ( 0, framebuffer.getTextureColor ().getHandle () );
			glDrawArrays ( GL_TRIANGLES, 0, 6 );
			brightPass.unbind ();
			glBlitNamedFramebuffer ( brightPass.getHandle (), bloom2.getHandle (), 0, 0, 256, 256, 0, 0, 256, 256, GL_COLOR_BUFFER_BIT, GL_LINEAR );
			for ( int i = 0; i != 4; i++ )
			{
				// 2.4 Blur X
				bloom1.bind ();
				progBlurX.useProgram ();
				glBindTextureUnit ( 0, bloom2.getTextureColor ().getHandle () );
				glDrawArrays ( GL_TRIANGLES, 0, 6 );
				bloom1.unbind ();
				// 2.5 Blur Y
				bloom2.bind ();
				progBlurY.useProgram ();
				glBindTextureUnit ( 0, bloom1.getTextureColor ().getHandle () );
				glDrawArrays ( GL_TRIANGLES, 0, 6 );
				bloom2.unbind ();
			}

			// 3. Apply tone mapping
			glViewport ( 0, 0, width, height );
			progCombineHDR.useProgram ();
			glBindTextureUnit ( 0, framebuffer.getTextureColor ().getHandle () );
			glBindTextureUnit ( 1, luminances [ 1 ]->getHandle () );
			glBindTextureUnit ( 2, bloom2.getTextureColor ().getHandle () );
			glDrawArrays ( GL_TRIANGLES, 0, 6 );
		}
		else
		{
			glBlitNamedFramebuffer ( framebuffer.getHandle (), 0, 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR );
		}

		// wait for comptue shader results to become visible
		if ( g_EnableGPUCulling && fenceCulling )
		{
			for ( ;;)
			{
				const GLenum res = glClientWaitSync ( fenceCulling, GL_SYNC_FLUSH_COMMANDS_BIT, 1000 );
				if ( res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED ) break;				
			}
			glDeleteSync ( fenceCulling );
		}

		glViewport ( 0, 0, width, height );

		const float indentSize = 16.0f;
		/*ImGuiIO& io = ImGui::GetIO ();
		io.DisplaySize = ImVec2 ( ( float ) width, ( float ) height );
		ImGui::NewFrame ();
		ImGui::Begin ( "Control", nullptr );
		ImGui::Text ( "Transparency:" );
		ImGui::Indent ( indentSize );
		ImGui::Checkbox ( "Opaque meshes", &g_DrawOpaque );
		ImGui::Checkbox ( "Transparent meshes", &g_DrawTransparent );
		ImGui::Unindent ( indentSize );
		ImGui::Separator ();
		ImGui::Text ( "GPU culling:" );
		ImGui::Indent ( indentSize );
		ImGui::Checkbox ( "Enable GPU culling", &g_EnableGPUCulling );
		imGuiPushFlagsAndStyles ( g_EnableGPUCulling );
		ImGui::Checkbox ( "Freeze culling frustum (P)", &g_FreezeCullingView );
		ImGui::Text ( "Visible meshes: %i", *numVisibleMeshesPtr );
		imGuiPopFlagsAndStyles ();
		ImGui::Unindent ( indentSize );
		ImGui::Separator ();
		ImGui::Text ( "SSAO:" );
		ImGui::Indent ( indentSize );
		ImGui::Checkbox ( "Enable SSAO blur", &g_EnableBlur );
		ImGui::SliderFloat ( "SSAO scale", &g_SSAOParams.scale_, 0.0f, 2.0f );
		ImGui::SliderFloat ( "SSAO bias", &g_SSAOParams.bias_, 0.0f, 0.3f );
		ImGui::SliderFloat ( "SSAO radius", &g_SSAOParams.radius_, 0.02f, 0.2f );
		ImGui::SliderFloat ( "SSAO attenuation scale", &g_SSAOParams.attScale_, 0.5f, 1.5f );
		ImGui::SliderFloat ( "SSAO distance scale", &g_SSAOParams.distScale_, 0.0f, 1.0f );
		imGuiPopFlagsAndStyles ();
		ImGui::Unindent ( indentSize );
		ImGui::Separator ();
		ImGui::Text ( "HDR:" );
		ImGui::Indent ( indentSize );
		ImGui::Checkbox ( "Enable HDR", &g_EnableHDR );
		imGuiPushFlagsAndStyles ( g_EnableHDR );
		ImGui::SliderFloat ( "Exposure", &g_HDRParams.exposure_, 0.1f, 2.0f );
		ImGui::SliderFloat ( "Max white", &g_HDRParams.maxWhite_, 0.5f, 2.0f );
		ImGui::SliderFloat ( "Bloom strength", &g_HDRParams.bloomStrength_, 0.0f, 2.0f );
		ImGui::SliderFloat ( "Adaptation speed", &g_HDRParams.adaptationSpeed_, 0.01f, 0.5f );
		imGuiPopFlagsAndStyles ();
		ImGui::Unindent ( indentSize );
		ImGui::Separator ();
		ImGui::Text ( "Shadows:" );
		ImGui::Indent ( indentSize );
		ImGui::Checkbox ( "Enable shadows", &g_EnableShadows );
		imGuiPushFlagsAndStyles ( g_EnableShadows );
		ImGui::Checkbox ( "Show light's frustum (red) and scene AABB (white)", &g_ShowLightFrustum );
		ImGui::SliderFloat ( "Light Theta", &g_LightTheta, -85.0f, +85.0f );
		ImGui::SliderFloat ( "Light Phi", &g_LightPhi, -85.0f, +85.0f );
		imGuiPopFlagsAndStyles ();
		ImGui::Unindent ( indentSize );
		ImGui::Separator ();
		ImGui::Checkbox ( "Grid", &g_DrawGrid );
		ImGui::Checkbox ( "Bounding boxes (all)", &g_DrawBoxes );
		ImGui::End ();*/
		ImGuiIO& io = ImGui::GetIO ();
		io.DisplaySize = ImVec2 ( ( float ) width, ( float ) height );
		ImGui::NewFrame ();
		ImGui::Begin ( "Control", nullptr );
		ImGui::Text ( "Transparency:" );
		ImGui::Indent ( indentSize );
		ImGui::Checkbox ( "Opaque meshes", &g_DrawOpaque );
		ImGui::Checkbox ( "Transparent meshes", &g_DrawTransparent );
		ImGui::Unindent ( indentSize );
		ImGui::Separator ();
		ImGui::Text ( "GPU culling:" );
		ImGui::Indent ( indentSize );
		ImGui::Checkbox ( "Enable GPU culling", &g_EnableGPUCulling );
		imGuiPushFlagsAndStyles ( g_EnableGPUCulling );
		ImGui::Checkbox ( "Freeze culling frustum (P)", &g_FreezeCullingView );
		ImGui::Text ( "Visible meshes: %i", *numVisibleMeshesPtr );
		imGuiPopFlagsAndStyles ();
		ImGui::Unindent ( indentSize );
		ImGui::Separator ();
		ImGui::Text ( "SSAO:" );
		ImGui::Indent ( indentSize );
		ImGui::Checkbox ( "Enable SSAO", &g_EnableSSAO );
		imGuiPushFlagsAndStyles ( g_EnableSSAO );
		ImGui::Checkbox ( "Enable SSAO blur", &g_EnableBlur );
		ImGui::SliderFloat ( "SSAO scale", &g_SSAOParams.scale_, 0.0f, 2.0f );
		ImGui::SliderFloat ( "SSAO bias", &g_SSAOParams.bias_, 0.0f, 0.3f );
		ImGui::SliderFloat ( "SSAO radius", &g_SSAOParams.radius_, 0.02f, 0.2f );
		ImGui::SliderFloat ( "SSAO attenuation scale", &g_SSAOParams.attScale_, 0.5f, 1.5f );
		ImGui::SliderFloat ( "SSAO distance scale", &g_SSAOParams.distScale_, 0.0f, 1.0f );
		imGuiPopFlagsAndStyles ();
		ImGui::Unindent ( indentSize );
		ImGui::Separator ();
		ImGui::Text ( "HDR:" );
		ImGui::Indent ( indentSize );
		ImGui::Checkbox ( "Enable HDR", &g_EnableHDR );
		imGuiPushFlagsAndStyles ( g_EnableHDR );
		ImGui::SliderFloat ( "Exposure", &g_HDRParams.exposure_, 0.1f, 2.0f );
		ImGui::SliderFloat ( "Max white", &g_HDRParams.maxWhite_, 0.5f, 2.0f );
		ImGui::SliderFloat ( "Bloom strength", &g_HDRParams.bloomStrength_, 0.0f, 2.0f );
		ImGui::SliderFloat ( "Adaptation speed", &g_HDRParams.adaptationSpeed_, 0.01f, 0.5f );
		imGuiPopFlagsAndStyles ();
		ImGui::Unindent ( indentSize );
		ImGui::Separator ();
		ImGui::Text ( "Shadows:" );
		ImGui::Indent ( indentSize );
		ImGui::Checkbox ( "Enable shadows", &g_EnableShadows );
		imGuiPushFlagsAndStyles ( g_EnableShadows );
		ImGui::Checkbox ( "Show light's frustum (red) and scene AABB (white)", &g_ShowLightFrustum );
		ImGui::SliderFloat ( "Light Theta", &g_LightTheta, -85.0f, +85.0f );
		ImGui::SliderFloat ( "Light Phi", &g_LightPhi, -85.0f, +85.0f );
		imGuiPopFlagsAndStyles ();
		ImGui::Unindent ( indentSize );
		ImGui::Separator ();
		ImGui::Checkbox ( "Grid", &g_DrawGrid );
		ImGui::Checkbox ( "Bounding boxes (all)", &g_DrawBoxes );
		ImGui::End ();
		if ( g_EnableSSAO )
		{
			imguiTextureWindowGL ( "SSAO", ssao.getTextureColor ().getHandle () );
		}
		if ( g_EnableShadows )
		{
			imguiTextureWindowGL ( "Shadow Map", shadowMap.getTextureDepth ().getHandle ());
		}
		ImGui::Render ();
		rendererUI.render ( width, height, ImGui::GetDrawData () );

		// swap current and adapter luminances
		std::swap ( luminances [ 0 ], luminances [ 1 ] );

		app.swapBuffers ();
	}

	glUnmapNamedBuffer ( numVisibleMeshesBuffer.getHandle () );
	glDeleteTextures ( 1, &luminance1x1 );
	
	return 0;
}