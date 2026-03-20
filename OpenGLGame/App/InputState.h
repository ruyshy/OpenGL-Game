#pragma once

#ifndef INPUTSTATE_H_
#define INPUTSTATE_H_

#include <array>

class InputState
{
public:
    static constexpr size_t KeyCount = 512;
    static constexpr size_t MouseButtonCount = 8;

    void beginFrame();

    void setKeyState(int keyCode, bool isDown);
    void setMouseButtonState(int button, bool isDown);
    void addScrollDelta(double offsetX, double offsetY);

    bool isKeyDown(int keyCode) const;
    bool wasKeyPressed(int keyCode) const;
    bool wasKeyReleased(int keyCode) const;

    bool isMouseButtonDown(int button) const;
    bool wasMouseButtonPressed(int button) const;
    bool wasMouseButtonReleased(int button) const;

    double getScrollDeltaX() const;
    double getScrollDeltaY() const;

private:
    bool isValidKey(int keyCode) const;
    bool isValidMouseButton(int button) const;

private:
    array<bool, KeyCount> _keysDown{};
    array<bool, KeyCount> _keysPressed{};
    array<bool, KeyCount> _keysReleased{};
    array<bool, MouseButtonCount> _mouseButtonsDown{};
    array<bool, MouseButtonCount> _mouseButtonsPressed{};
    array<bool, MouseButtonCount> _mouseButtonsReleased{};
    double _scrollDeltaX = 0.0;
    double _scrollDeltaY = 0.0;
};

#endif // !INPUTSTATE_H_
