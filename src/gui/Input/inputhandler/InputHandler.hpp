#pragma once

#include <vector>

#include "Render/camera/topdown/TopDownCamera.hpp"
#include "types/Event.hpp"

namespace Zappy
{

/**
 * @class InputHandler
 * @brief Translates window events into camera control and quit decisions.
 */
class InputHandler
{
  public:
    InputHandler();

    /**
     * @brief Applies a frame's input events to the camera.
     * @param events Events polled this frame.
     * @param camera Camera to drive (zoom, pan).
     * @return False when the user asked to quit, true otherwise.
     */
    bool handle(const std::vector<Event> &events, TopDownCamera &camera);
};

} // namespace Zappy
