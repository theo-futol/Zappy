#include "Input/inputhandler/InputHandler.hpp"

#include <cmath>

namespace Zappy
{

InputHandler::InputHandler()
    : _holdForward(false), _holdBack(false), _holdLeft(false), _holdRight(false), _holdUp(false), _holdDown(false), _dragging(false), _moved(false), _clickPending(false),
      _lastX(0.0), _lastY(0.0), _pressX(0.0), _pressY(0.0), _clickX(0.0), _clickY(0.0)
{
}

void InputHandler::setHeld(int key, bool held)
{
    if (key == KeyForward)
        _holdForward = held;
    else if (key == KeyBack)
        _holdBack = held;
    else if (key == KeyLeft)
        _holdLeft = held;
    else if (key == KeyRight)
        _holdRight = held;
    else if (key == KeyUp)
        _holdUp = held;
    else if (key == KeyDown)
        _holdDown = held;
}

bool InputHandler::handleFree(const std::vector<Event> &events, FreeCamera &camera)
{
    bool running = true;

    for (const Event &event : events)
    {
        if (event.type == EventType::Close)
            running = false;
        else if (event.type == EventType::KeyPress)
            setHeld(event.key, true);
        else if (event.type == EventType::KeyRelease)
            setHeld(event.key, false);
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
            camera.look(static_cast<float>(event.mouseX - _lastX) * FreeLookSensitivity, static_cast<float>(_lastY - event.mouseY) * FreeLookSensitivity);
            _lastX = event.mouseX;
            _lastY = event.mouseY;
            if (std::abs(event.mouseX - _pressX) > DragThreshold || std::abs(event.mouseY - _pressY) > DragThreshold)
                _moved = true;
        }
    }

    float forward = static_cast<float>(_holdForward) - static_cast<float>(_holdBack);
    float right = static_cast<float>(_holdRight) - static_cast<float>(_holdLeft);
    float up = static_cast<float>(_holdUp) - static_cast<float>(_holdDown);

    if (forward != 0.0f || right != 0.0f || up != 0.0f)
        camera.move(forward * FreeMoveStep, right * FreeMoveStep, up * FreeMoveStep);
    return running;
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

bool InputHandler::handleOrbit(const std::vector<Event> &events, OrbitCamera &camera)
{
    bool running = true;

    for (const Event &event : events)
    {
        if (event.type == EventType::Close)
            running = false;
        else if (event.type == EventType::Scroll)
            camera.dolly(std::pow(DollyBase, -static_cast<float>(event.scrollDelta)));
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
            camera.orbit(static_cast<float>(_lastX - event.mouseX) * OrbitSensitivity, static_cast<float>(event.mouseY - _lastY) * OrbitSensitivity);
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
