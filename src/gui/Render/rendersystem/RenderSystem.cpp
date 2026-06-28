#include "Render/rendersystem/RenderSystem.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

#include <glm/ext/matrix_transform.hpp>
#include <glm/matrix.hpp>

#include "Command/commandbuilder/CommandBuilder.hpp"
#include "Graphics/cubemap/CubemapTexture.hpp"
#include "Graphics/modelloader/ModelLoader.hpp"
#include "Graphics/primitives/Primitives.hpp"
#include "Graphics/window/Window.hpp"
#include "Render/camera/orbit/OrbitCamera.hpp"
#include "Render/projection/ground/GroundProjection.hpp"
#include "Render/projection/planar/PlanarProjection.hpp"
#include "Render/projection/torus/TorusProjection.hpp"
#include "Render/renderer/entity/EntityRenderer.hpp"
#include "Render/renderer/map/MapRenderer.hpp"
#include "Render/renderer/scene/SceneRenderer.hpp"
#include "Render/renderer/sky/SkyRenderer.hpp"
#include "Render/rendermodel/RenderModel.hpp"
#include "interface/IEntity.hpp"
#include "types/EntityKey.hpp"
#include "types/GridPosition.hpp"
#include "types/WorldPoint.hpp"

namespace Zappy
{

RenderSystem::RenderSystem(Window &window, GraphicsContext &context, RenderMode mode)
    : _width(window.width()), _height(window.height()), _window(window), _context(context), _assets(nullptr), _text(nullptr), _camera(nullptr), _projection(nullptr),
      _topDown(nullptr), _orbit(nullptr), _torus(nullptr), _input(), _renderers(), _hud(nullptr), _sceneHud(nullptr), _framed(false), _outgoing(), _mode(mode), _mouseX(0.0),
      _mouseY(0.0), _following(false), _pov(false), _free(), _freeFly(false), _returnToMenu(false)
{
}

bool RenderSystem::wantsMenu() const
{
    return _returnToMenu;
}

float RenderSystem::orientationYaw(Orientation orientation)
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

const IEntity *RenderSystem::resolveFollowed(const GameState &state)
{
    const IEntity *followed = _following ? state.selectedEntity() : nullptr;

    if (followed != nullptr && followed->getEntityType() != "player")
        followed = nullptr;
    if (_following && followed == nullptr)
        detachFollow(state);
    _sceneHud->setFollowing(_following);
    _sceneHud->setPov(_pov);
    return followed;
}

void RenderSystem::detachFollow(const GameState &state)
{
    _following = false;
    _pov = false;
    if (_torus != nullptr)
        _orbit->setTarget(Vec3(0.0f, 0.0f, 0.0f));
    else if (state.map().width() > 0 && state.map().height() > 0)
        _orbit->setTarget(Vec3(static_cast<float>(state.map().width() - 1) * 0.5f, 0.0f, static_cast<float>(state.map().height() - 1) * 0.5f));
}

void RenderSystem::init()
{
    _context.configureDefaults();
    _context.setViewport(_width, _height);
    if (_mode == RenderMode::TwoD)
        initTwoD();
    else
        initThreeD();
}

void RenderSystem::initThreeD()
{
    std::unique_ptr<OrbitCamera> camera = std::make_unique<OrbitCamera>();
    bool torusWorld = (_mode == RenderMode::ThreeDTorus);

    _orbit = camera.get();
    camera->setViewport(_width, _height);
    _camera = std::move(camera);
    _free.setViewport(_width, _height);
    if (torusWorld)
    {
        std::unique_ptr<TorusProjection> torus = std::make_unique<TorusProjection>();

        _torus = torus.get();
        _torus->setRadii(TorusMajor, TorusMinor);
        _projection = std::move(torus);
    }
    else
    {
        _projection = std::make_unique<GroundProjection>();
    }
    try
    {
        ModelLoader loader;
        std::string groundPath = torusWorld ? TorusModelPath : FlatGroundModelPath;
        std::string groundDir = groundPath.substr(0, groundPath.find_last_of('/'));
        Model ground = loader.load(groundPath);

        MeshData quad = Primitives::quad();

        _assets = std::make_unique<AssetCache>();
        _renderers.push_back(std::make_unique<SkyRenderer>(*_assets, SkyCrossPath));
        _assets->loadShader("phong", "shaders/phong.vert", "shaders/phong.frag");
        std::unique_ptr<SceneRenderer> scene = std::make_unique<SceneRenderer>(*_assets, ground, groundDir);

        scene->setTorusWorld(torusWorld);
        scene->setTorusRadii(TorusMajor, TorusMinor);
        _renderers.push_back(std::move(scene));

        _assets->loadShader("basic", "shaders/basic.vert", "shaders/basic.frag");
        _assets->createMesh("quad", quad.vertices, quad.indices, {3});
        _text = std::make_unique<TextRenderer>();
        _text->loadFont("fonts/BlackOpsOne-Regular.ttf", 22);
        _text->setScreenSize(_width, _height);
        _sceneHud = std::make_unique<SceneHudRenderer>(*_assets, *_text);
        _sceneHud->setScreenSize(_width, _height);
    }
    catch (const CubemapTexture::CubemapException &e)
    {
        throw RenderSystemException(std::string("sky: ") + e.what());
    }
    catch (const TextRenderer::TextRendererException &e)
    {
        throw RenderSystemException(std::string("text: ") + e.what());
    }
    catch (const ModelLoader::ModelLoaderException &e)
    {
        throw RenderSystemException(std::string("model: ") + e.what());
    }
    catch (const RenderModel::RenderModelException &e)
    {
        throw RenderSystemException(std::string("model: ") + e.what());
    }
    catch (const AssetCache::AssetCacheException &e)
    {
        throw RenderSystemException(std::string("asset: ") + e.what());
    }
}

void RenderSystem::initTwoD()
{
    MeshData quad = Primitives::quad();
    MeshData triangle = Primitives::triangle();
    std::unique_ptr<TopDownCamera> camera = std::make_unique<TopDownCamera>();

    _topDown = camera.get();
    camera->setViewport(_width - HudRenderer::panelWidth(_width), _height);
    _camera = std::move(camera);
    _projection = std::make_unique<PlanarProjection>();
    try
    {
        _assets = std::make_unique<AssetCache>();
        _assets->loadShader("basic", "shaders/basic.vert", "shaders/basic.frag");
        _assets->createMesh("quad", quad.vertices, quad.indices, {3});
        _assets->createMesh("triangle", triangle.vertices, triangle.indices, {3});
        _renderers.push_back(std::make_unique<MapRenderer>(*_assets));
        _renderers.push_back(std::make_unique<EntityRenderer>(*_assets));

        _text = std::make_unique<TextRenderer>();
        _text->loadFont("fonts/BlackOpsOne-Regular.ttf", 22);
        _text->setScreenSize(_width, _height);
        _hud = std::make_unique<HudRenderer>(*_assets, *_text);
        _hud->setScreenSize(_width, _height);
    }
    catch (const AssetCache::AssetCacheException &e)
    {
        throw RenderSystemException(std::string("asset: ") + e.what());
    }
    catch (const TextRenderer::TextRendererException &e)
    {
        throw RenderSystemException(std::string("text: ") + e.what());
    }
}

void RenderSystem::render(const GameState &state)
{
    if (_mode != RenderMode::TwoD)
    {
        if (!_framed && state.map().width() > 0 && state.map().height() > 0)
        {
            if (_torus != nullptr)
            {
                _torus->setGridSize(state.map().width(), state.map().height());
                _orbit->frame(Vec3(0.0f, 0.0f, 0.0f), (TorusMajor + TorusMinor) * TorusFramePad);
            }
            else
            {
                Vec3 center(static_cast<float>(state.map().width() - 1) * 0.5f, 0.0f, static_cast<float>(state.map().height() - 1) * 0.5f);

                _orbit->frame(center, 0.6f * static_cast<float>(std::max(state.map().width(), state.map().height())) + 2.0f);
            }
            _framed = true;
        }
        const IEntity *followed = resolveFollowed(state);

        _sceneHud->setCameraFree(_freeFly);

        Mat4 view = (_freeFly ? _free.view() : _orbit->view());
        Mat4 projection = (_freeFly ? _free.projection() : _orbit->projection());

        if (followed != nullptr)
        {
            WorldPoint anchor = _projection->toWorld(followed->position().x, followed->position().y);
            Vec3 eye = anchor.position + Vec3(0.0f, FollowEye, 0.0f);

            projection = _orbit->projection();
            if (_pov)
            {
                _orbit->setYaw(orientationYaw(followed->orientation()));
                view = glm::lookAt(eye, eye + _orbit->forward(), Vec3(0.0f, 1.0f, 0.0f));
            }
            else
            {
                _orbit->setTarget(eye);
                view = _orbit->view();
            }
        }
        _window.clear(Color(0.1f, 0.1f, 0.12f, 1.0f));
        _context.setViewport(_width, _height);
        _context.setDepthTest(true);
        RenderContext context{state, view, projection, *_projection};
        for (const std::unique_ptr<IRenderer> &renderer : _renderers)
            renderer->render(context);
        _context.setDepthTest(false);
        _sceneHud->render(context);
        _context.setDepthTest(true);
        _window.swapBuffers();
        return;
    }
    if (!_framed && state.map().width() > 0 && state.map().height() > 0)
    {
        _topDown->fitToMap(state.map().width(), state.map().height());
        _framed = true;
    }
    RenderContext context{state, _camera->view(), _camera->projection(), *_projection};
    _window.clear(Color(0.1f, 0.1f, 0.12f, 1.0f));
    _context.setViewport(_width - HudRenderer::panelWidth(_width), _height);
    _context.setDepthTest(true);
    for (const std::unique_ptr<IRenderer> &renderer : _renderers)
        renderer->render(context);
    _context.setViewport(_width, _height);
    _context.setDepthTest(false);
    _hud->render(context);
    _context.setDepthTest(true);
    _window.swapBuffers();
}

bool RenderSystem::processInput(GameState &state)
{
    if (!_window.isOpen())
        return false;

    std::vector<Event> events = _window.pollEvents();

    for (const Event &event : events)
    {
        if (event.type == EventType::MouseMove)
        {
            _mouseX = event.mouseX;
            _mouseY = event.mouseY;
        }
        if (event.type == EventType::Resize)
        {
            _width = event.width;
            _height = event.height;
            if (_mode == RenderMode::TwoD)
            {
                _camera->setViewport(_width - HudRenderer::panelWidth(_width), _height);
                _hud->setScreenSize(_width, _height);
                _text->setScreenSize(_width, _height);
            }
            else
            {
                _camera->setViewport(_width, _height);
                _free.setViewport(_width, _height);
                _sceneHud->setScreenSize(_width, _height);
                _text->setScreenSize(_width, _height);
            }
        }
    }
    if (_mode != RenderMode::TwoD)
    {
        bool running3D = (_freeFly && !_following) ? _input.handleFree(events, _free) : _input.handleOrbit(events, *_orbit);
        double clickX = 0.0;
        double clickY = 0.0;

        updateHover(state);
        if (_input.consumeClick(clickX, clickY))
        {
            if (_sceneHud->menuButtonHit(clickX, clickY))
            {
                _returnToMenu = true;
                return false;
            }
            if (_sceneHud->cameraButtonHit(clickX, clickY))
            {
                _freeFly = !_freeFly;
                if (_freeFly)
                    _free.setPose(Vec3(glm::inverse(_orbit->view())[3]), _orbit->forward());
            }
            else if (_following && _sceneHud->modeButtonHit(clickX, clickY, state))
            {
                _pov = !_pov;
                const IEntity *target = state.selectedEntity();

                if (_pov && target != nullptr)
                    _orbit->setAngles(orientationYaw(target->orientation()), 0.0f);
            }
            else if (_sceneHud->followButtonHit(clickX, clickY, state))
            {
                if (_following)
                    detachFollow(state);
                else
                {
                    const IEntity *target = state.selectedEntity();

                    if (target != nullptr)
                    {
                        WorldPoint anchor = _projection->toWorld(target->position().x, target->position().y);

                        _following = true;
                        _orbit->frame(anchor.position + Vec3(0.0f, FollowEye, 0.0f), FollowRadius);
                    }
                }
            }
            else
            {
                std::string command = _sceneHud->handleClick(clickX, clickY, state);

                if (!command.empty())
                    _outgoing.push_back(command);
                else if (!_following)
                    selectAt3D(clickX, clickY, state);
            }
        }
        return running3D;
    }

    bool running = _input.handle(events, *_topDown);
    double clickX = 0.0;
    double clickY = 0.0;

    if (_input.consumeClick(clickX, clickY))
    {
        if (_hud->menuButtonHit(clickX, clickY))
        {
            _returnToMenu = true;
            return false;
        }

        std::string command = _hud->handleClick(clickX, clickY, state);

        if (!command.empty())
            _outgoing.push_back(command);
        else
            selectAt(clickX, clickY, state);
    }
    return running;
}

void RenderSystem::selectAt(double px, double py, GameState &state)
{
    if (px >= _topDown->viewportWidth())
        return;

    float worldX = 0.0f;
    float worldY = 0.0f;

    _topDown->worldFromScreen(static_cast<float>(px), static_cast<float>(py), worldX, worldY);
    selectEntityAt(static_cast<int>(std::lround(worldX)), static_cast<int>(std::lround(worldY)), state);
}

bool RenderSystem::groundTile(double px, double py, const GameState &state, int &tileX, int &tileY) const
{
    Mat4 view = (_freeFly && !_following) ? _free.view() : _orbit->view();
    Mat4 projection = (_freeFly && !_following) ? _free.projection() : _orbit->projection();
    Mat4 viewProj = projection * view;
    float width = static_cast<float>(_width);
    float height = static_cast<float>(_height);
    float bestDepth = std::numeric_limits<float>::max();
    bool found = false;

    for (int gridY = 0; gridY < state.map().height(); ++gridY)
        for (int gridX = 0; gridX < state.map().width(); ++gridX)
        {
            WorldPoint point = _projection->toWorld(gridX, gridY);
            Vec4 clip = viewProj * Vec4(point.position, 1.0f);

            if (clip.w <= 0.0f)
                continue;

            float screenX = (clip.x / clip.w * 0.5f + 0.5f) * width;
            float screenY = (1.0f - (clip.y / clip.w * 0.5f + 0.5f)) * height;
            float dx = screenX - static_cast<float>(px);
            float dy = screenY - static_cast<float>(py);

            if (dx * dx + dy * dy > PickRadiusPx * PickRadiusPx)
                continue;
            if (clip.w < bestDepth)
            {
                bestDepth = clip.w;
                tileX = gridX;
                tileY = gridY;
                found = true;
            }
        }
    return found;
}

void RenderSystem::selectAt3D(double px, double py, GameState &state)
{
    int tileX = 0;
    int tileY = 0;

    if (groundTile(px, py, state, tileX, tileY))
        selectEntityAt(tileX, tileY, state);
}

void RenderSystem::updateHover(GameState &state)
{
    int tileX = 0;
    int tileY = 0;

    if (groundTile(_mouseX, _mouseY, state, tileX, tileY) && tileX >= 0 && tileX < state.map().width() && tileY >= 0 && tileY < state.map().height())
        state.setHoveredTile(GridPosition{tileX, tileY});
    else
        state.clearHoveredTile();
}

void RenderSystem::selectEntityAt(int tileX, int tileY, GameState &state)
{
    const IEntity *picked = nullptr;

    for (const std::pair<const EntityKey, std::unique_ptr<IEntity>> &entry : state.entities())
    {
        GridPosition position = entry.second->position();

        if (position.x != tileX || position.y != tileY)
            continue;
        picked = entry.second.get();
        if (entry.second->getEntityType() == "player")
            break;
    }
    if (picked == nullptr)
    {
        state.clearSelection();
        return;
    }
    state.selectEntity(EntityKey{picked->getEntityType(), picked->number()});
    if (picked->getEntityType() == "player")
    {
        _outgoing.push_back(CommandBuilder::requestPlayerPosition(picked->number()));
        _outgoing.push_back(CommandBuilder::requestPlayerLevel(picked->number()));
        _outgoing.push_back(CommandBuilder::requestPlayerInventory(picked->number()));
    }
}

std::vector<std::string> RenderSystem::takeOutgoing()
{
    std::vector<std::string> out = std::move(_outgoing);

    _outgoing.clear();
    return out;
}

} // namespace Zappy
