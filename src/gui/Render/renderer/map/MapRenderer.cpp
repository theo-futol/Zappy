#include "Render/renderer/map/MapRenderer.hpp"

#include <glm/ext/matrix_transform.hpp>

#include "types/NamedColors.hpp"

namespace Zappy
{

MapRenderer::MapRenderer(AssetCache &assets) : _assets(assets)
{
}

void MapRenderer::render(const RenderContext &context)
{
    Shader *shader = _assets.shader("basic");
    Mesh *mesh = _assets.mesh("quad");
    const Map &map = context.state.map();

    if (shader == nullptr || mesh == nullptr)
        return;
    shader->use();
    shader->setUniform("uView", context.view);
    shader->setUniform("uProjection", context.projection);

    for (int gridY = 0; gridY < map.height(); ++gridY)
    {
        for (int gridX = 0; gridX < map.width(); ++gridX)
        {
            WorldPoint point = context.mapping.toWorld(gridX, gridY);

            shader->setUniform("uModel", glm::translate(Mat4(1.0f), point.position));
            shader->setUniform("uColor", tileColor(gridX, gridY));
            mesh->draw();
            drawResources(*shader, *mesh, map.at(gridX, gridY), point.position);
        }
    }
}

Vec3 MapRenderer::tileColor(int gridX, int gridY) const
{
    if ((gridX + gridY) % 2 == 0)
        return NamedColors::LightGray;
    return NamedColors::DarkGray;
}

void MapRenderer::drawResources(Shader &shader, Mesh &mesh, const Tile &tile, const Vec3 &tilePosition) const
{
    for (int i = 0; i < static_cast<int>(ResourceSet::Count); ++i)
    {
        ResourceType type = static_cast<ResourceType>(i);

        if (tile.resources().get(type) <= 0)
            continue;

        Mat4 model = glm::translate(Mat4(1.0f), tilePosition + resourceOffset(i));

        model = glm::scale(model, Vec3(0.18f));
        shader.setUniform("uModel", model);
        shader.setUniform("uColor", resourceColor(type));
        mesh.draw();
    }
}

Vec3 MapRenderer::resourceColor(ResourceType type) const
{
    switch (type)
    {
    case ResourceType::Food:
        return NamedColors::Green;
    case ResourceType::Linemate:
        return NamedColors::OffWhite;
    case ResourceType::Deraumere:
        return NamedColors::Brown;
    case ResourceType::Sibur:
        return NamedColors::Cyan;
    case ResourceType::Mendiane:
        return NamedColors::Purple;
    case ResourceType::Phiras:
        return NamedColors::Orange;
    case ResourceType::Thystame:
        return NamedColors::Red;
    }
    return NamedColors::White;
}

Vec3 MapRenderer::resourceOffset(int index) const
{
    int column = index % 3;
    int row = index / 3;

    return Vec3(static_cast<float>(column - 1) * 0.3f, static_cast<float>(1 - row) * 0.3f, 0.01f);
}

} // namespace Zappy
