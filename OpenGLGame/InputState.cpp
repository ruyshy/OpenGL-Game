#include "pch.h"
#include "InputState.h"

void InputState::beginFrame()
{
    _keysPressed.fill(false);
    _keysReleased.fill(false);
    _mouseButtonsPressed.fill(false);
    _mouseButtonsReleased.fill(false);
    _scrollDeltaX = 0.0;
    _scrollDeltaY = 0.0;
}

void InputState::setKeyState(int keyCode, bool isDown)
{
    if (!isValidKey(keyCode))
    {
        return;
    }

    if (_keysDown[keyCode] != isDown)
    {
        _keysDown[keyCode] = isDown;
        if (isDown)
        {
            _keysPressed[keyCode] = true;
        }
        else
        {
            _keysReleased[keyCode] = true;
        }
    }
}

void InputState::setMouseButtonState(int button, bool isDown)
{
    if (!isValidMouseButton(button))
    {
        return;
    }

    if (_mouseButtonsDown[button] != isDown)
    {
        _mouseButtonsDown[button] = isDown;
        if (isDown)
        {
            _mouseButtonsPressed[button] = true;
        }
        else
        {
            _mouseButtonsReleased[button] = true;
        }
    }
}

void InputState::addScrollDelta(double offsetX, double offsetY)
{
    _scrollDeltaX += offsetX;
    _scrollDeltaY += offsetY;
}

bool InputState::isKeyDown(int keyCode) const
{
    return isValidKey(keyCode) ? _keysDown[keyCode] : false;
}

bool InputState::wasKeyPressed(int keyCode) const
{
    return isValidKey(keyCode) ? _keysPressed[keyCode] : false;
}

bool InputState::wasKeyReleased(int keyCode) const
{
    return isValidKey(keyCode) ? _keysReleased[keyCode] : false;
}

bool InputState::isMouseButtonDown(int button) const
{
    return isValidMouseButton(button) ? _mouseButtonsDown[button] : false;
}

bool InputState::wasMouseButtonPressed(int button) const
{
    return isValidMouseButton(button) ? _mouseButtonsPressed[button] : false;
}

bool InputState::wasMouseButtonReleased(int button) const
{
    return isValidMouseButton(button) ? _mouseButtonsReleased[button] : false;
}

double InputState::getScrollDeltaX() const
{
    return _scrollDeltaX;
}

double InputState::getScrollDeltaY() const
{
    return _scrollDeltaY;
}

bool InputState::isValidKey(int keyCode) const
{
    return keyCode >= 0 && keyCode < static_cast<int>(KeyCount);
}

bool InputState::isValidMouseButton(int button) const
{
    return button >= 0 && button < static_cast<int>(MouseButtonCount);
}
