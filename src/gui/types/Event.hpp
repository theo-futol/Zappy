#pragma once

namespace Zappy
{

/**
 * @enum EventType
 * @brief Category of a window input event.
 */
enum class EventType
{
    None,
    Close,
    Resize,
    KeyPress,
    KeyRelease,
    Char,
    MouseMove,
    MouseButton,
    Scroll
};

/**
 * @struct Event
 * @brief Library-agnostic input event translated from the windowing backend.
 */
struct Event
{
    EventType type;     ///< Event category.
    int key;            ///< Key or mouse button code (backend-neutral); Unicode codepoint for Char events.
    bool pressed;       ///< True on press, false on release (keys, mouse buttons).
    double mouseX;      ///< Cursor X position.
    double mouseY;      ///< Cursor Y position.
    double scrollDelta; ///< Scroll amount along the Y axis.
    int width;          ///< New width on a resize event.
    int height;         ///< New height on a resize event.
};

} // namespace Zappy
