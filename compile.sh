/Users/schoe/goinfre/vk/macOS/bin/glslc ./glsl/particle.comp -o ./spv/particle.comp.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./glsl/particle.frag -o ./spv/particle.frag.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./glsl/particle.vert -o ./spv/particle.vert.spv

/Users/schoe/goinfre/vk/macOS/bin/glslc ./stableFluids/shaderSource/fluid.frag -o ./spv/fluid.frag.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./stableFluids/shaderSource/fluid.vert -o ./spv/fluid.vert.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./stableFluids/shaderSource/sourcing.comp -o ./spv/sourcing.comp.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./stableFluids/shaderSource/advect.comp -o ./spv/advect.comp.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./stableFluids/shaderSource/applyPressure.comp -o ./spv/applyPressure.comp.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./stableFluids/shaderSource/diffuse.comp -o ./spv/diffuse.comp.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./stableFluids/shaderSource/divergence.comp -o ./spv/divergence.comp.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./stableFluids/shaderSource/jacobi.comp -o ./spv/jacobi.comp.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./stableFluids/shaderSource/confineVorticity.comp -o ./spv/confineVorticity.comp.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./stableFluids/shaderSource/computeVorticity.comp -o ./spv/computeVorticity.comp.spv

/Users/schoe/goinfre/vk/macOS/bin/glslc ./cloud/shaderSource/cloudDensity.comp -o ./spv/cloudDensity.comp.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./cloud/shaderSource/cloudLighting.comp -o ./spv/cloudLighting.comp.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./cloud/shaderSource/cloud.vert -o ./spv/cloud.vert.spv
/Users/schoe/goinfre/vk/macOS/bin/glslc ./cloud/shaderSource/cloud.frag -o ./spv/cloud.frag.spv