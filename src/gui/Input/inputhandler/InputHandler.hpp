#pragma once

#include <vector>

#include "Render/camera/topdown/TopDownCamera.hpp"
#include "types/Event.hpp"

namespace Zappy
{

/**
 * @class InputHandler
 * @brief Translates window events into camera control, click detection and quit decisions.
 *
 * Drives zoom and pan directly on the camera, and reports a single click per frame
 * (a press/release without a drag) for the caller to route to selection or UI.
 */
class InputHandler
{
  public:
    InputHandler();

    /**
     * @brief Applies a frame's input events to the camera and records a click if any.
     * @param events Events polled this frame.
     * @param camera Camera to drive (zoom, pan).
     * @return False when the user asked to quit, true otherwise.
     */
    bool handle(const std::vector<Event> &events, TopDownCamera &camera);

    /**
     * @brief Consumes the click recorded this frame, if any.
     * @param x Receives the click X in pixels.
     * @param y Receives the click Y in pixels.
     * @return True if a click was pending (then cleared), false otherwise.
     */
    bool consumeClick(double &x, double &y);

  private:
    static constexpr int LeftButton = 0;         ///< Neutral left mouse button code (GLFW_MOUSE_BUTTON_LEFT).
    static constexpr float ZoomBase = 1.1f;      ///< Per-scroll-step zoom multiplier base.
    static constexpr double DragThreshold = 4.0; ///< Pixel motion above which a press is a drag, not a click.

    bool _dragging;     ///< Whether the left button is held (panning).
    bool _moved;        ///< Whether the cursor moved past the drag threshold since the press.
    bool _clickPending; ///< Whether a click was recorded this frame.
    double _lastX;      ///< Last cursor X recorded while dragging.
    double _lastY;      ///< Last cursor Y recorded while dragging.
    double _pressX;     ///< Cursor X at the last button press.
    double _pressY;     ///< Cursor Y at the last button press.
    double _clickX;     ///< Recorded click X.
    double _clickY;     ///< Recorded click Y.
};

} // namespace Zappy
