#include "Render/renderer/sky/SkyRenderer.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Graphics/primitives/Primitives.hpp"
#include "Graphics/shader/Shader.hpp"
#include "types/Mat.hpp"
#include "types/MeshData.hpp"

namespace Zappy
{

SkyRenderer::SkyRenderer(AssetCache &assets, const std::string &cubemapPath) : _assets(assets), _cubemap(std::make_unique<CubemapTexture>(cubemapPath)), _cube(nullptr)
{
    MeshData cube = Primitives::cube();

    _cube = &assets.createMesh("skybox_cube", cube.vertices, cube.indices, {3, 3, 2});
    assets.loadShader("skybox", "shaders/skybox.vert", "shaders/skybox.frag");
}

void SkyRenderer::render(const RenderContext &context)
{
    Shader *shader = _assets.shader("skybox");

    if (shader == nullptr || _cube == nullptr)
        return;

    shader->use();
    shader->setUniform("uView", Mat4(Mat3(context.view))); // rotation seule (translation retirée)
    shader->setUniform("uProjection", context.projection);
    shader->setUniform("uSky", 0);
    _cubemap->bind(0);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    _cube->draw();
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

} // namespace Zappy
