#pragma once

#include <exception>
#include <memory>
#include <string>
#include <vector>
#include "Graphics/context/GraphicsContext.hpp"
#include "Graphics/text/TextRenderer.hpp"
#include "Graphics/window/Window.hpp"
#include "Input/inputhandler/InputHandler.hpp"
#include "Model/gamestate/GameState.hpp"
#include "Render/asset/AssetCache.hpp"
#include "Render/camera/topdown/TopDownCamera.hpp"
#include "Render/projection/planar/PlanarProjection.hpp"
#include "Render/renderer/hud/HudRenderer.hpp"
#include "interface/IRenderer.hpp"

namespace Zappy
{

/**
 * @class RenderSystem
 * @brief Owns the window, GPU resources and renderers; drives one frame of output.
 */
class RenderSystem
{
  public:
    /**
     * @class RenderSystemException
     * @brief Error raised during render setup or while loading assets.
     */
    class RenderSystemException : public std::exception
    {
      public:
        explicit RenderSystemException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    /**
     * @brief Builds the render system.
     * @param width Window width in pixels.
     * @param height Window height in pixels.
     * @param title Window title.
     */
    RenderSystem(int width, int height, const std::string &title);

    RenderSystem(const RenderSystem &) = delete;
    RenderSystem &operator=(const RenderSystem &) = delete;
    RenderSystem(RenderSystem &&) = delete;
    RenderSystem &operator=(RenderSystem &&) = delete;

    /**
     * @brief Creates the window and context and loads the renderers and assets.
     * @throws RenderSystemException On window, context or asset failure.
     */
    void init();

    /**
     * @brief Draws one frame from the given state.
     * @param state Game state to render.
     */
    void render(const GameState &state);

    /**
     * @brief Polls input and updates the camera and selection.
     * @param state State updated on selection.
     * @return False when the user asked to quit, true otherwise.
     */
    bool processInput(GameState &state);

    /**
     * @brief Takes the commands queued this frame (e.g. from HUD buttons or selection).
     * @return The pending command lines, moved out (the queue is left empty).
     */
    std::vector<std::string> takeOutgoing();

  private:
    /**
     * @brief Picks the entity under a world click and updates the selection.
     * @param px Click X in pixels.
     * @param py Click Y in pixels.
     * @param state State whose selection is set or cleared.
     */
    void selectAt(double px, double py, GameState &state);

    int _width;         ///< Window width in pixels.
    int _height;        ///< Window height in pixels.
    std::string _title; ///< Window title.

    std::unique_ptr<Window> _window;           ///< Owned window and context.
    std::unique_ptr<GraphicsContext> _context; ///< Owned OpenGL state.
    std::unique_ptr<AssetCache> _assets;       ///< Owned GPU resource cache.
    std::unique_ptr<TextRenderer> _text;       ///< Owned text renderer.

    TopDownCamera _camera;        ///< Active camera.
    PlanarProjection _projection; ///< Active grid-to-world mapping.
    InputHandler _input;          ///< Input translation.

    std::vector<std::unique_ptr<IRenderer>> _renderers; ///< Ordered world render stages.
    std::unique_ptr<HudRenderer> _hud;                  ///< HUD overlay (drawn separately, full viewport, depth off).
    bool _framed;                                       ///< Whether the map was auto-framed once (then user controls).
    std::vector<std::string> _outgoing;                 ///< Commands queued this frame, drained by Core.
};

} // namespace Zappy
