#pragma once

#include <vector>

#include "Render/camera/free/FreeCamera.hpp"
#include "Render/camera/orbit/OrbitCamera.hpp"
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
     * @brief Applies a frame's input events to the orbit camera (3D mode).
     * @param events Events polled this frame.
     * @param camera Orbit camera to drive (drag orbits, scroll dollies).
     * @return False when the user asked to quit, true otherwise.
     */
    bool handleOrbit(const std::vector<Event> &events, OrbitCamera &camera);

    /**
     * @brief Applies a frame's input events to the free-fly camera (3D mode).
     * @param events Events polled this frame.
     * @param camera Free camera to drive (keys move, drag looks).
     * @return False when the user asked to quit, true otherwise.
     */
    bool handleFree(const std::vector<Event> &events, FreeCamera &camera);

    /**
     * @brief Consumes the click recorded this frame, if any.
     * @param x Receives the click X in pixels.
     * @param y Receives the click Y in pixels.
     * @return True if a click was pending (then cleared), false otherwise.
     */
    bool consumeClick(double &x, double &y);

  private:
    /**
     * @brief Records a movement key as held or released.
     * @param key Key code from the event.
     * @param held True on press, false on release.
     */
    void setHeld(int key, bool held);

    static constexpr int LeftButton = 0;             ///< Neutral left mouse button code (GLFW_MOUSE_BUTTON_LEFT).
    static constexpr float ZoomBase = 1.1f;          ///< Per-scroll-step zoom multiplier base.
    static constexpr double DragThreshold = 4.0;     ///< Pixel motion above which a press is a drag, not a click.
    static constexpr float DollyBase = 1.1f;         ///< Per-scroll-step orbit-distance multiplier base.
    static constexpr float OrbitSensitivity = 0.3f;  ///< Degrees of orbit rotation per pixel dragged.
    static constexpr float FreeLookSensitivity = 0.2f; ///< Degrees of free-look per pixel dragged.
    static constexpr float FreeMoveStep = 0.4f;      ///< Free-fly move distance per frame and per held key.
    static constexpr int KeyForward = 'W';           ///< Move-forward key (GLFW key code).
    static constexpr int KeyBack = 'S';              ///< Move-back key.
    static constexpr int KeyLeft = 'A';              ///< Strafe-left key.
    static constexpr int KeyRight = 'D';             ///< Strafe-right key.
    static constexpr int KeyUp = ' ';                ///< Move-up key (space).
    static constexpr int KeyDown = 340;              ///< Move-down key (GLFW_KEY_LEFT_SHIFT).

    bool _holdForward;  ///< Whether the forward key is held.
    bool _holdBack;     ///< Whether the back key is held.
    bool _holdLeft;     ///< Whether the strafe-left key is held.
    bool _holdRight;    ///< Whether the strafe-right key is held.
    bool _holdUp;       ///< Whether the move-up key is held.
    bool _holdDown;     ///< Whether the move-down key is held.
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
