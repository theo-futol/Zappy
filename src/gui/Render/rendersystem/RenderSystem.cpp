#include "Render/rendersystem/RenderSystem.hpp"

#include <cmath>
#include <memory>
#include <utility>

#include "Command/commandbuilder/CommandBuilder.hpp"
#include "Graphics/primitives/Primitives.hpp"
#include "Graphics/window/Window.hpp"
#include "Render/renderer/entity/EntityRenderer.hpp"
#include "Render/renderer/map/MapRenderer.hpp"
#include "interface/IEntity.hpp"
#include "types/EntityKey.hpp"
#include "types/GridPosition.hpp"

namespace Zappy
{

RenderSystem::RenderSystem(int width, int height, const std::string &title)
    : _width(width), _height(height), _title(title), _window(nullptr), _context(nullptr), _assets(nullptr), _text(nullptr), _camera(), _projection(), _input(), _renderers(),
      _hud(nullptr), _framed(false), _outgoing()
{
}

void RenderSystem::init()
{
    MeshData quad = Primitives::quad();
    MeshData triangle = Primitives::triangle();

    _camera.setViewport(_width - HudRenderer::panelWidth(_width), _height);
    try
    {
        _window = std::make_unique<Window>(_width, _height, _title);
        _context = std::make_unique<GraphicsContext>();
        _context->configureDefaults();
        _context->setViewport(_width, _height);
    }
    catch (const Window::WindowException &e)
    {
        throw RenderSystemException("Window error: " + std::string(e.what()));
    }
    catch (const GraphicsContext::GraphicsContextException &e)
    {
        throw RenderSystemException("Graphics context error: " + std::string(e.what()));
    }
    try
    {
        _assets = std::make_unique<AssetCache>();
        _assets->loadShader("basic", "shaders/basic.vert", "shaders/basic.frag");
        _assets->createMesh("quad", quad.vertices, quad.indices);
        _assets->createMesh("triangle", triangle.vertices, triangle.indices);
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
    if (!_framed && state.map().width() > 0 && state.map().height() > 0)
    {
        _camera.fitToMap(state.map().width(), state.map().height());
        _framed = true;
    }
    RenderContext context{state, _camera.view(), _camera.projection(), _projection};
    _window->clear(Color(0.1f, 0.1f, 0.12f, 1.0f));
    _context->setViewport(_width - HudRenderer::panelWidth(_width), _height);
    _context->setDepthTest(true);
    for (const std::unique_ptr<IRenderer> &renderer : _renderers)
        renderer->render(context);
    _context->setViewport(_width, _height);
    _context->setDepthTest(false);
    _hud->render(context);
    _context->setDepthTest(true);
    _window->swapBuffers();
}

bool RenderSystem::processInput(GameState &state)
{
    if (!_window->isOpen())
        return false;

    std::vector<Event> events = _window->pollEvents();

    for (const Event &event : events)
    {
        if (event.type == EventType::Resize)
        {
            _width = event.width;
            _height = event.height;
            _camera.setViewport(_width - HudRenderer::panelWidth(_width), _height);
            _hud->setScreenSize(_width, _height);
            _text->setScreenSize(_width, _height);
        }
    }
    bool running = _input.handle(events, _camera);
    double clickX = 0.0;
    double clickY = 0.0;

    if (_input.consumeClick(clickX, clickY))
    {
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
    if (px >= _camera.viewportWidth())
        return;

    float worldX = 0.0f;
    float worldY = 0.0f;

    _camera.worldFromScreen(static_cast<float>(px), static_cast<float>(py), worldX, worldY);

    int tileX = static_cast<int>(std::lround(worldX));
    int tileY = static_cast<int>(std::lround(worldY));
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
