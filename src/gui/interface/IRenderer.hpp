#pragma once

#include "Render/rendercontext/RenderContext.hpp"

namespace Zappy
{

/**
 * @class IRenderer
 * @brief Contract for one stage of the rendering pipeline.
 */
class IRenderer
{
  public:
    virtual ~IRenderer() = default;

    /**
     * @brief Draws this stage for the current frame.
     * @param context Per-frame state, matrices, projection.
     */
    virtual void render(const RenderContext &context) = 0;
};

} // namespace Zappy
