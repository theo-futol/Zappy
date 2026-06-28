#include "Render/renderer/scene/SceneRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include "Graphics/modelloader/ModelLoader.hpp"
#include "Graphics/modelslicer/ModelSlicer.hpp"
#include "Graphics/primitives/Primitives.hpp"
#include "Model/map/Map.hpp"
#include "Model/resourceset/ResourceSet.hpp"
#include "Model/tile/Tile.hpp"
#include "interface/IEntity.hpp"
#include "types/EntityKey.hpp"
#include "types/GridPosition.hpp"
#include "types/ResourceType.hpp"
#include "types/Theme.hpp"
#include "types/WorldPoint.hpp"

namespace Zappy
{

const Vec3 SceneRenderer::LightColor(1.0f, 1.0f, 1.0f);
const Vec3 SceneRenderer::ModelColor(0.70f, 0.80f, 0.90f);
const Vec3 SceneRenderer::HoverColor(1.0f, 0.9f, 0.3f);
const Vec3 SceneRenderer::SelectColor(0.2f, 1.0f, 0.6f);

SceneRenderer::SceneRenderer(AssetCache &assets, const Model &ground, const std::string &groundDirectory)
    : _assets(assets), _ground(std::make_unique<RenderModel>(ground, assets, "ground", groundDirectory)), _registry(), _golems(), _eggs(), _resources(), _highlight(nullptr),
      _torusMajor(12.5f), _torusMinor(3.0f), _torusWorld(true)
{
    MeshData highlightQuad = Primitives::groundQuad();

    loadThemes(assets);
    loadResources(assets);
    _highlight = &assets.createMesh("highlight_quad", highlightQuad.vertices, highlightQuad.indices, {3, 3, 2});
}

void SceneRenderer::setTorusRadii(float major, float minor)
{
    _torusMajor = major;
    _torusMinor = minor;
}

void SceneRenderer::setTorusWorld(bool torusWorld)
{
    _torusWorld = torusWorld;
}

float SceneRenderer::orientationYaw(Orientation orientation)
{
    switch (orientation)
    {
    case Orientation::East:
        return -90.0f;
    case Orientation::South:
        return 180.0f;
    case Orientation::West:
        return 90.0f;
    case Orientation::North:
    default:
        return 0.0f;
    }
}

Mat4 SceneRenderer::tileFrame(const WorldPoint &point)
{
    Vec3 up = glm::normalize(point.normal);
    Vec3 right = glm::normalize(point.tangent);
    Vec3 forward = glm::normalize(glm::cross(right, up));
    Mat4 frame(1.0f);

    frame[0] = Vec4(right, 0.0f);
    frame[1] = Vec4(up, 0.0f);
    frame[2] = Vec4(forward, 0.0f);
    return frame;
}

void SceneRenderer::drawHighlight(Shader &shader, const WorldPoint &point, const Vec3 &color) const
{
    Mat4 model = glm::translate(Mat4(1.0f), point.position + point.normal * HighlightLift) * tileFrame(point) * glm::scale(Mat4(1.0f), Vec3(HighlightSize, 1.0f, HighlightSize));

    shader.setUniform("uNormalMatrix", Mat3(1.0f));
    shader.setUniform("uHasTexture", 0);
    shader.setUniform("uBaseColor", color);
    _highlight->drawInstanced(std::vector<Mat4>{model});
}

void SceneRenderer::loadResources(AssetCache &assets)
{
    static const std::array<const char *, ResourceSet::Count> needles = {
        "",                            // Food: drawn from its own whole model
        "green_cristal_baseColor",     // Linemate
        "green_cristal.001_baseColor", // Deraumere
        "green_cristal_3_baseColor",   // Sibur
        "rock2_baseColor",             // Mendiane
        "rock4_baseColor",             // Phiras
        "rock8_baseColor"              // Thystame
    };
    ModelLoader loader;
    ModelSlicer slicer;
    Model crystal = loader.load(CrystalModelPath);
    std::string crystalDirectory = directoryOf(CrystalModelPath);

    _resources.resize(ResourceSet::Count);
    _resources[static_cast<std::size_t>(ResourceType::Food)] = std::make_unique<RenderModel>(loader.load(FoodModelPath), assets, "res_food", directoryOf(FoodModelPath));
    for (std::size_t i = 0; i < ResourceSet::Count; ++i)
    {
        if (needles[i][0] == '\0')
            continue;
        _resources[i] = std::make_unique<RenderModel>(slicer.byTexture(crystal, needles[i]), assets, "res#" + std::to_string(i), crystalDirectory);
    }
}

void SceneRenderer::loadThemes(AssetCache &assets)
{
    ModelLoader loader;

    for (std::size_t i = 0; i < _registry.count(); ++i)
    {
        const Theme &theme = _registry.forTeam(i);
        std::string suffix = std::to_string(i);

        _golems.push_back(std::make_unique<RenderModel>(loader.load(theme.golem), assets, "golem#" + suffix, directoryOf(theme.golem)));
        _eggs.push_back(std::make_unique<RenderModel>(loader.load(theme.egg), assets, "egg#" + suffix, directoryOf(theme.egg)));
    }
}

Vec3 SceneRenderer::resourceSpot(std::size_t index)
{
    std::size_t column = index % 3;
    std::size_t row = index / 3;
    float offsetX = (static_cast<float>(column) - 1.0f) * ResourceSpacing;
    float offsetZ = (static_cast<float>(row) - 1.0f) * ResourceSpacing;

    return Vec3(offsetX, 0.0f, offsetZ);
}

std::string SceneRenderer::directoryOf(const std::string &path)
{
    std::size_t slash = path.find_last_of('/');

    return (slash == std::string::npos) ? std::string(".") : path.substr(0, slash);
}

bool SceneRenderer::transformerPose(const EntityKey &key, GridPosition position, const RenderModel &model, float time, int &clip, float &poseTime)
{
    int vehicleClip = model.animationIndex(VehicleClipName);
    int robotClip = model.animationIndex(RobotClipName);

    if (vehicleClip < 0 || robotClip < 0)
        return false;

    EntityAnim &state = _entityAnim[key];

    if (!state.initialized)
    {
        state.initialized = true;
        state.lastPos = position;
    }
    if (position.x != state.lastPos.x || position.y != state.lastPos.y)
    {
        state.lastPos = position;
        state.lastMoveTime = time;
    }

    bool wantVehicle = (time - state.lastMoveTime) < IdleBeforeRobot;

    if (state.clip < 0 && wantVehicle != state.vehicle)
    {
        state.vehicle = wantVehicle;
        state.clip = wantVehicle ? vehicleClip : robotClip;
        state.clipStart = time;
    }
    if (state.clip >= 0 && time - state.clipStart >= model.animationDuration(static_cast<std::size_t>(state.clip)))
        state.clip = -1;

    int settledClip = state.vehicle ? vehicleClip : robotClip;

    if (state.clip >= 0)
    {
        clip = state.clip;
        poseTime = time - state.clipStart;
    }
    else
    {
        clip = settledClip;
        poseTime = model.animationDuration(static_cast<std::size_t>(settledClip));
    }
    return true;
}

void SceneRenderer::render(const RenderContext &context)
{
    Shader *shader = _assets.shader("phong");

    if (shader == nullptr)
        return;

    const Map &map = context.state.map();
    Vec3 eye = Vec3(glm::inverse(context.view)[3]);
    float outer = _torusMajor + _torusMinor;
    float span = static_cast<float>(std::max(map.width(), map.height()));
    Vec3 mapCenter(static_cast<float>(map.width()) * 0.5f, 0.0f, static_cast<float>(map.height()) * 0.5f);
    Vec3 lightPos = _torusWorld ? Vec3(outer, 2.0f * outer, outer) : mapCenter + Vec3(span * 0.5f, span + 1.0f, span * 0.5f);

    shader->use();
    shader->setUniform("uView", context.view);
    shader->setUniform("uProjection", context.projection);
    shader->setUniform("uLightPos", lightPos);
    shader->setUniform("uViewPos", eye);
    shader->setUniform("uLightColor", LightColor);
    shader->setUniform("uTexture", 0);
    shader->setUniform("uBaseColor", ModelColor);

    Mat4 groundBase;

    if (_torusWorld)
    {
        Mat4 lay = glm::rotate(Mat4(1.0f), glm::radians(90.0f), Vec3(1.0f, 0.0f, 0.0f));

        groundBase = lay * glm::scale(Mat4(1.0f), Vec3(2.0f * outer)) * glm::translate(Mat4(1.0f), -_ground->center()) * _ground->unitTransform();
    }
    else
    {
        Vec3 groundCenter(static_cast<float>(map.width() - 1) * 0.5f, 0.0f, static_cast<float>(map.height() - 1) * 0.5f);
        Vec3 groundScale(static_cast<float>(map.width()), 1.0f, static_cast<float>(map.height()));

        groundBase = glm::translate(Mat4(1.0f), groundCenter) * glm::scale(Mat4(1.0f), groundScale) * _ground->footprintTransform();
    }
    std::vector<Mat4> groundBases{groundBase};
    std::size_t themeCount = _golems.size();

    _ground->drawInstanced(*shader, groundBases);
    for (const std::pair<const EntityKey, std::unique_ptr<IEntity>> &entry : context.state.entities())
    {
        const IEntity &entity = *entry.second;
        WorldPoint point = context.mapping.toWorld(entity.position().x, entity.position().y);
        std::size_t theme = context.state.teamIndex(entity.team()) % themeCount;
        bool isEgg = entity.getEntityType() == "egg";
        const RenderModel &model = isEgg ? *_eggs[theme] : *_golems[theme];
        Mat4 facing = glm::rotate(Mat4(1.0f), glm::radians(orientationYaw(entity.orientation())), Vec3(0.0f, 1.0f, 0.0f));
        Mat4 base = glm::translate(Mat4(1.0f), point.position) * tileFrame(point) * facing * model.unitTransform();

        if (model.skinned())
        {
            int clip = 0;
            float poseTime = 0.0f;

            if (isEgg || !transformerPose(entry.first, entity.position(), model, context.time, clip, poseTime))
            {
                float duration = model.animationDuration(0);

                clip = 0;
                poseTime = (duration > 0.0f) ? std::fmod(context.time, duration) : 0.0f;
            }
            model.drawSkinned(*shader, base, model.poseJoints(static_cast<std::size_t>(clip), poseTime));
        }
        else
            model.drawInstanced(*shader, std::vector<Mat4>{base});
    }

    std::vector<std::vector<Mat4>> resourceBases(ResourceSet::Count);

    for (int gridY = 0; gridY < map.height(); ++gridY)
        for (int gridX = 0; gridX < map.width(); ++gridX)
        {
            const Tile &tile = map.at(gridX, gridY);
            WorldPoint point = context.mapping.toWorld(gridX, gridY);
            Mat4 frame = tileFrame(point);

            for (std::size_t type = 0; type < ResourceSet::Count; ++type)
            {
                if (_resources[type] == nullptr || tile.resources().get(static_cast<ResourceType>(type)) <= 0)
                    continue;
                Mat4 base = glm::translate(Mat4(1.0f), point.position) * frame * glm::translate(Mat4(1.0f), resourceSpot(type)) * glm::scale(Mat4(1.0f), Vec3(ResourceScale)) *
                            _resources[type]->unitTransform();

                resourceBases[type].push_back(base);
            }
        }
    for (std::size_t type = 0; type < ResourceSet::Count; ++type)
        if (_resources[type] != nullptr)
            _resources[type]->drawInstanced(*shader, resourceBases[type]);

    const IEntity *selected = context.state.selectedEntity();

    if (context.state.hasHoveredTile())
    {
        GridPosition hovered = context.state.hoveredTile();

        drawHighlight(*shader, context.mapping.toWorld(hovered.x, hovered.y), HoverColor);
    }
    if (selected != nullptr)
        drawHighlight(*shader, context.mapping.toWorld(selected->position().x, selected->position().y), SelectColor);
}

} // namespace Zappy
