#include "Render/renderer/hud/HudRenderer.hpp"

namespace Zappy
{

HudRenderer::HudRenderer(TextRenderer &text) : _text(text)
{
}

void HudRenderer::render(const RenderContext &context)
{
    (void)context;
}

} // namespace Zappy
