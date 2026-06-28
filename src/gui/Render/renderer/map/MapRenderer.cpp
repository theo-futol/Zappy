#include "Render/renderer/map/MapRenderer.hpp"

#include <algorithm>
#include <cmath>

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include "Graphics/primitives/Primitives.hpp"
#include "Model/gamestate/GameState.hpp"
#include "types/MeshData.hpp"
#include "types/NamedColors.hpp"

namespace Zappy
{

namespace
{
const Vec3 IncantColor(0.6f, 0.2f, 1.0f);       ///< Pulsing tint of an incanting tile (2D).
const Vec3 BroadcastColor(0.45f, 0.80f, 0.95f); ///< Color of a broadcast ripple (2D).
constexpr float BroadcastDuration = 1.1f;       ///< Seconds a broadcast ripple expands.
constexpr float BroadcastMaxTiles = 9.0f;       ///< Final size of a broadcast ripple, in tiles.
} // namespace

MapRenderer::MapRenderer(AssetCache &assets) : _assets(assets)
{
    MeshData ring = Primitives::ring(48, 0.22f);

    _ring = &assets.createMesh("broadcast_ring2d", ring.vertices, ring.indices, {3});
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
            const Tile &tile = map.at(gridX, gridY);
            Vec3 color = tileColor(gridX, gridY);

            if (tile.incanting())
            {
                float pulse = 0.45f + 0.55f * std::sin(context.time * 6.0f);

                color = glm::mix(color, IncantColor, pulse);
            }
            shader->setUniform("uModel", glm::translate(Mat4(1.0f), point.position));
            shader->setUniform("uColor", color);
            mesh->draw();
            drawResources(*shader, *mesh, tile, point.position);
        }
    }
    drawBroadcasts(*shader, *mesh, context);
}

void MapRenderer::drawBroadcasts(Shader &shader, Mesh &mesh, const RenderContext &context)
{
    (void)mesh;
    for (const GameState::Broadcast &broadcast : context.state.broadcasts())
        if (broadcast.sequence > _seenBroadcastSeq)
        {
            _seenBroadcastSeq = broadcast.sequence;
            _pings.push_back(BroadcastPing{broadcast.origin, context.time});
        }
    if (_ring == nullptr)
        return;
    for (const BroadcastPing &ping : _pings)
    {
        float progress = (context.time - ping.start) / BroadcastDuration;

        if (progress < 0.0f || progress > 1.0f)
            continue;

        WorldPoint point = context.mapping.toWorld(ping.origin.x, ping.origin.y);
        float diameter = glm::mix(0.4f, BroadcastMaxTiles, progress);
        Mat4 model = glm::scale(glm::translate(Mat4(1.0f), point.position), Vec3(diameter, diameter, 1.0f));

        shader.setUniform("uModel", model);
        shader.setUniform("uColor", BroadcastColor);
        _ring->draw();
    }
    _pings.erase(std::remove_if(_pings.begin(), _pings.end(), [&](const BroadcastPing &ping) { return context.time - ping.start > BroadcastDuration; }), _pings.end());
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
