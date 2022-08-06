#include <jcVkFramework/vkFramework/VulkanApp.h>
#include <jcVkFramework/vkRenderers/VulkanComputeBase.h>

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/load_ktx.hpp>

#include <jcCommonframework/ResourceString.h>

constexpr int brdfW = 256;
constexpr int brdfH = 256;

const uint32_t bufferSize = 2 * sizeof ( float ) * brdfW * brdfH;

float lutData[bufferSize];

VulkanInstance vk;
VulkanRenderDevice vkDev;

void calculateLUT ( float* output )
{
	ComputeBase cb ( vkDev, appendToRoot ( "assets/shaders/VK06_BRDF_LUT.comp" ).c_str (), sizeof ( float ), bufferSize );
	if ( !cb.execute ( brdfW, brdfH, 1 ) )
		exit ( EXIT_FAILURE );

	cb.downloadOutput ( 0, (uint8_t*)lutData, bufferSize );
}

gli::texture convertLUTtoTexture ( const float* data )
{
	gli::texture lutTexture = gli::texture2d ( gli::FORMAT_RG16_SFLOAT_PACK16, gli::extent2d ( brdfW, brdfH ), 1 );

	for ( int y = 0; y < brdfH; y++ )
	{
		for ( int x = 0; x < brdfW; x++ )
		{
			const int ofs = y * brdfW + x;
			const gli::vec2 value ( data[ofs * 2 + 0], data[ofs * 2 + 1] );
			const gli::texture::extent_type uv = { x, y, 0 };
			lutTexture.store<glm::uint32> ( uv, 0, 0, 0, gli::packHalf2x16 ( value ) );
		}
	}

	return lutTexture;
}

int main ()
{
	GLFWwindow* window = initVulkanApp ( brdfW, brdfH );

	// createInstance(&vk.instance);
	createInstanceWithDebugging ( &vk.instance, "jc3DTest Chapter 06 BRDF LUT Exercise" );

	if ( !setupDebugCallbacks ( vk.instance, &vk.messenger ) ||
		glfwCreateWindowSurface ( vk.instance, window, nullptr, &vk.surface ) ||
		!initVulkanRenderDeviceWithCompute ( vk, vkDev, brdfW, brdfH, VkPhysicalDeviceFeatures{} ) )
	{
		exit ( EXIT_FAILURE );
	}

	printf ( "Calculating LUT texture...\n" );
	calculateLUT ( lutData );

	printf ( "Saving LUT texture...\n" );
	gli::texture lutTexture = convertLUTtoTexture ( lutData );

	// use Pico Pixel to view https://pixelandpolygon.com/
	gli::save_ktx ( lutTexture, appendToRoot ( "assets/images/brdfLUT.ktx" ).c_str() );

	destroyVulkanRenderDevice ( vkDev );
	destroyVulkanInstance ( vk );

	glfwTerminate ();
	glslang_finalize_process ();

	return 0;
}