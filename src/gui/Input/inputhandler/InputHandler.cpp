#include "Input/inputhandler/InputHandler.hpp"

#include <cmath>

namespace Zappy
{

InputHandler::InputHandler() : _dragging(false), _moved(false), _clickPending(false), _lastX(0.0), _lastY(0.0), _pressX(0.0), _pressY(0.0), _clickX(0.0), _clickY(0.0)
{
}

bool InputHandler::handle(const std::vector<Event> &events, TopDownCamera &camera)
{
    bool running = true;

    for (const Event &event : events)
    {
        if (event.type == EventType::Close)
            running = false;
        else if (event.type == EventType::Scroll)
            camera.zoomBy(std::pow(ZoomBase, static_cast<float>(event.scrollDelta)));
        else if (event.type == EventType::MouseButton && event.key == LeftButton)
        {
            if (event.pressed)
            {
                _dragging = true;
                _moved = false;
                _lastX = event.mouseX;
                _lastY = event.mouseY;
                _pressX = event.mouseX;
                _pressY = event.mouseY;
            }
            else
            {
                _dragging = false;
                if (!_moved)
                {
                    _clickPending = true;
                    _clickX = event.mouseX;
                    _clickY = event.mouseY;
                }
            }
        }
        else if (event.type == EventType::MouseMove && _dragging)
        {
            camera.panPixels(static_cast<float>(event.mouseX - _lastX), static_cast<float>(event.mouseY - _lastY));
            _lastX = event.mouseX;
            _lastY = event.mouseY;
            if (std::abs(event.mouseX - _pressX) > DragThreshold || std::abs(event.mouseY - _pressY) > DragThreshold)
                _moved = true;
        }
    }
    return running;
}

bool InputHandler::consumeClick(double &x, double &y)
{
    if (!_clickPending)
        return false;
    x = _clickX;
    y = _clickY;
    _clickPending = false;
    return true;
}

} // namespace Zappy
