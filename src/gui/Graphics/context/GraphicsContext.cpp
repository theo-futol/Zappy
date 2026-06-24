#include "Graphics/context/GraphicsContext.hpp"
#include "glad/glad.h"

namespace Zappy
{

GraphicsContext::GraphicsContext()
{
}

void GraphicsContext::configureDefaults()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GraphicsContext::setViewport(int width, int height)
{
    glViewport(0, 0, width, height);
}

} // namespace Zappy
