import android.dawn.*
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class AsyncHelperTest {
    @Test
    fun asyncMethodTest() {
        dawnTestLauncher() { device ->
            /* Set up a shader module to support the async call. */
            val shaderModule = device.createShaderModule(
                ShaderModuleDescriptor(shaderSourceWGSL = ShaderSourceWGSL(""))
            )

            /* Call an asynchronous method, converted from a callback pattern by a helper. */
            val result = device.createRenderPipelineAsync(
                RenderPipelineDescriptor(vertex = VertexState(module = shaderModule))
            )

            assert(result.status == CreatePipelineAsyncStatus.ValidationError) {
                """Create render pipeline (async) should fail when no shader entry point exists.
                   The result was: ${result.status}"""
            }
        }
    }

    @Test
    fun asyncMethodTestValidationPasses() {
        dawnTestLauncher() { device ->
            /* Set up a shader module to support the async call. */
            val shaderModule = device.createShaderModule(
                ShaderModuleDescriptor(
                    shaderSourceWGSL = ShaderSourceWGSL(
                        """
        @binding(0) @group(0) var<uniform> frame : u32;
        @vertex
        fn vtx_main(@builtin(vertex_index) vertex_index : u32) -> @builtin(position) vec4f {
          const pos = array(
            vec2( 0.0,  0.5),
            vec2(-0.5, -0.5),
            vec2( 0.5, -0.5)
          );
        
          return vec4f(pos[vertex_index], 0, 1);
        }
                    """
                    )
                )
            )

            /* Call an asynchronous method, converted from a callback pattern by a helper. */
            val result = device.createRenderPipelineAsync(
                RenderPipelineDescriptor(
                    vertex = VertexState(module = shaderModule, entryPoint = "vtx_main")
                )
            )

            assert(result.status == CreatePipelineAsyncStatus.Success) {
                """Create render pipeline (async) should pass with a simple shader.
                   The result was: ${result.status}
                   The message was: ${result.message}"""
            }
        }
    }
}
