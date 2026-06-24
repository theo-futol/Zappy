#include "Render/rendersystem/RenderSystem.hpp"

#include <memory>

#include "Graphics/primitives/Primitives.hpp"
#include "Graphics/window/Window.hpp"
#include "Render/renderer/entity/EntityRenderer.hpp"
#include "Render/renderer/map/MapRenderer.hpp"

namespace Zappy
{

RenderSystem::RenderSystem(int width, int height, const std::string &title)
    : _width(width), _height(height), _title(title), _window(nullptr), _context(nullptr), _assets(nullptr), _text(nullptr), _camera(), _projection(), _input(), _renderers()
{
}

void RenderSystem::init()
{
    MeshData quad = Primitives::quad();
    MeshData triangle = Primitives::triangle();

    _camera.setViewport(_width, _height);
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
    }
    catch (const AssetCache::AssetCacheException &e)
    {
        throw RenderSystemException(std::string("asset: ") + e.what());
    }
}

void RenderSystem::render(const GameState &state)
{
    _camera.fitToMap(state.map().width(), state.map().height());

    RenderContext context{state, _camera.view(), _camera.projection(), _projection};

    _window->clear(Color(0.1f, 0.1f, 0.12f, 1.0f));
    for (const std::unique_ptr<IRenderer> &renderer : _renderers)
        renderer->render(context);
    _window->swapBuffers();
}

bool RenderSystem::processInput()
{
    if (!_window->isOpen())
        return false;
    for (const Event &event : _window->pollEvents())
    {
        if (event.type == EventType::Close)
            return false;
        if (event.type == EventType::Resize)
        {
            _width = event.width;
            _height = event.height;
            _context->setViewport(_width, _height);
            _camera.setViewport(_width, _height);
        }
    }
    return true;
}

} // namespace Zappy
