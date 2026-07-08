#include "Render/renderer/entity/EntityRenderer.hpp"
#include "Graphics/shader/Shader.hpp"
#include "types/Appearance.hpp"
#include "types/GridPosition.hpp"
#include "types/Mat.hpp"
#include "types/Vec.hpp"
#include "types/WorldPoint.hpp"
#include <glm/ext/matrix_transform.hpp>

namespace Zappy
{

EntityRenderer::EntityRenderer(AssetCache &assets) : _assets(assets)
{
}

void EntityRenderer::render(const RenderContext &context)
{
    Shader *shader = _assets.shader("basic");

    if (shader == nullptr)
        return;
    shader->use();
    shader->setUniform("uView", context.view);
    shader->setUniform("uProjection", context.projection);
    for (const auto &[key, entity] : context.state.entities())
    {
        Appearance appearance = entity->appearance();
        Mesh *mesh = _assets.mesh(meshForVisual(appearance.visualId));
        GridPosition position = entity->position();
        WorldPoint point = context.mapping.toWorld(position.x, position.y);

        if (mesh == nullptr)
            continue;
        Mat4 model = glm::translate(Mat4(1.0f), point.position + Vec3(0.0f, 0.0f, 0.1f));
        model = glm::rotate(model, angleForOrientation(entity->orientation()), Vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, Vec3(appearance.scale * 0.6f));
        shader->setUniform("uModel", model);
        shader->setUniform("uColor", Vec3(appearance.color.r, appearance.color.g, appearance.color.b));
        mesh->draw();
    }
}

std::string EntityRenderer::meshForVisual(const std::string &visualId) const
{
    if (visualId == "trantorian")
        return "triangle";
    return "quad";
}

float EntityRenderer::angleForOrientation(Orientation orientation) const
{
    switch (orientation)
    {
    // Server convention: North = y-1 (up), South = y+1 (down). The triangle's nose points
    // toward +Y at rest, so North must rotate to -Y (180) and South to +Y (0); E/W are on
    // the rotation axis and stay correct.
    case Orientation::North:
        return glm::radians(180.0f);
    case Orientation::East:
        return glm::radians(-90.0f);
    case Orientation::South:
        return 0.0f;
    case Orientation::West:
        return glm::radians(90.0f);
    }
    return 0.0f;
}

} // namespace Zappy
