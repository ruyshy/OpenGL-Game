#include "pch.h"
#include "OpenGLWindow.h"

map<GLFWwindow*, OpenGLWindow*> OpenGLWindow::_windows;

OpenGLWindow::OpenGLWindow()
{
    for (auto& kwp : _keyWasPressed)
    {
        kwp = false;
    }
}

bool OpenGLWindow::createOpenGLWindow(const string& windowTitle, int majorVersion, int minorVersion, int width, int height, bool showFullscreen)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, majorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minorVersion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    const auto primaryMonitor = glfwGetPrimaryMonitor();
    const auto videoMode = glfwGetVideoMode(primaryMonitor);

    _window = glfwCreateWindow(width, height, windowTitle.c_str(), showFullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
    if (_window == nullptr)
    {
        return false;
    }

    glfwMakeContextCurrent(_window);
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
    glfwSetWindowSizeCallback(_window, onWindowSizeChangedStatic);
    glfwSetDropCallback(_window, drop_callback);

    if (showFullscreen)
    {
        glfwMaximizeWindow(_window);
        // After calling glfwMaximizeWindow, the onWindowSizeChanged somehow does not get called. Therefore I call it manually.
        int wwidth, hheight;
        glfwGetWindowSize(_window, &wwidth, &hheight);
        onWindowSizeChangedInternal(wwidth, hheight);
    }
    else
    {
        onWindowSizeChangedInternal(width, height);
    }

    auto windowHandle = GetForegroundWindow();
    long Style = GetWindowLong(windowHandle, GWL_STYLE);
    Style &= ~WS_MAXIMIZEBOX; //this makes it still work when WS_MAXIMIZEBOX is actually already toggled off
    SetWindowLong(windowHandle, GWL_STYLE, Style);

    glfwSetMouseButtonCallback(_window, onMouseButtonPressedStatic);
    glfwSetScrollCallback(_window, onMouseWheelScrollStatic);
    _windows[_window] = this;

    GLFWimage images[1];
    images[0].pixels = stbi_load(".\\Image\\icon.png", &images[0].width, &images[0].height, 0, 4); //rgba channels 
    glfwSetWindowIcon(this->getWindow(), 1, images);
    stbi_image_free(images[0].pixels);

    return true;
}

bool OpenGLWindow::keyPressed(int keyCode) const
{
    return glfwGetKey(_window, keyCode) == GLFW_PRESS;
}

bool OpenGLWindow::keyPressedOnce(int keyCode)
{
    bool result = false;
    if (keyPressed(keyCode))
    {
        if (!_keyWasPressed[keyCode])
        {
            result = true;
        }

        _keyWasPressed[keyCode] = true;
    }
    else
    {
        _keyWasPressed[keyCode] = false;
    }

    return result;
}

void OpenGLWindow::runApp()
{
    setVerticalSynchronization(true);
    recalculateProjectionMatrix();
    initializeScene();

    // Update time at the beginning, so that calculations are correct
    _lastFrameTime = _lastFrameTimeFPS = glfwGetTime();

    while (glfwWindowShouldClose(_window) == 0)
    {
        updateDeltaTimeAndFPS();
        Render();
        updateScene();
    }

    releaseScene();

    glfwDestroyWindow(_window);
    _windows.erase(_windows.find(_window));

    if (_windows.empty())
    {
        glfwTerminate();
    }
}

GLFWwindow* OpenGLWindow::getWindow() const
{
    return _window;
}

void OpenGLWindow::closeWindow(bool hasErrorOccured)
{
    glfwSetWindowShouldClose(_window, true);
    _hasErrorOccured = hasErrorOccured;
}

bool OpenGLWindow::hasErrorOccured() const
{
    return _hasErrorOccured;
}

mat4 OpenGLWindow::getProjectionMatrix() const
{
    return _projectionMatrix;
}

mat4 OpenGLWindow::getOrthoProjectionMatrix() const
{
    return _orthoMatrix;
}

float OpenGLWindow::sof(float value) const
{
    return value * static_cast<float>(_timeDelta);
}

double OpenGLWindow::sof(double value) const
{
    return value * _timeDelta;
}

double OpenGLWindow::getTimeDelta() const
{
    return _timeDelta;
}

int OpenGLWindow::getFPS() const
{
    return _FPS;
}

void OpenGLWindow::setVerticalSynchronization(bool enable)
{
    glfwSwapInterval(enable ? 1 : 0);
    _isVerticalSynchronizationEnabled = enable;
}

bool OpenGLWindow::isVerticalSynchronizationEnabled() const
{
    return _isVerticalSynchronizationEnabled;
}

int OpenGLWindow::getScreenWidth() const
{
    return screenWidth_;
}

int OpenGLWindow::getScreenHeight() const
{
    return screenHeight_;
}

ivec2 OpenGLWindow::getOpenGLCursorPosition() const
{
    double posX, posY;
    glfwGetCursorPos(_window, &posX, &posY);
    return ivec2(static_cast<int>(posX), screenHeight_ - static_cast<int>(posY));
}

OpenGLWindow* OpenGLWindow::getDefaultWindow()
{
    return _windows.size() == 0 ? nullptr : (*_windows.begin()).second;
}

void OpenGLWindow::Render()
{
    renderScene();

    glfwSwapBuffers(_window);
    glfwPollEvents();
}

void OpenGLWindow::recalculateProjectionMatrix()
{
    int width, height;
    glfwGetWindowSize(getWindow(), &width, &height);
    if (width == 0 && height == 0)  return;
    _projectionMatrix = perspective(radians(57.0f), static_cast<float>(width) / static_cast<float>(height), 0.5f, 1500.0f);
    _orthoMatrix = ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
}

void OpenGLWindow::updateDeltaTimeAndFPS()
{
    const auto currentTime = glfwGetTime();
    _timeDelta = currentTime - _lastFrameTime;
    _lastFrameTime = currentTime;
    _nextFPS++;

    if (currentTime - _lastFrameTimeFPS > 1.0)
    {
        _lastFrameTimeFPS = currentTime;
        _FPS = _nextFPS;
        _nextFPS = 0;
    }
}

void OpenGLWindow::onWindowSizeChangedInternal(int width, int height)
{
    recalculateProjectionMatrix();
    glViewport(0, 0, width, height);
    screenWidth_ = width;
    screenHeight_ = height;
    onWindowSizeChanged(width, height);
}

void OpenGLWindow::onWindowSizeChangedStatic(GLFWwindow* window, int width, int height)
{
    if (_windows.count(window) != 0)
    {
        _windows[window]->onWindowSizeChangedInternal(width, height);
    }
}

void OpenGLWindow::onMouseButtonPressedStatic(GLFWwindow* window, int button, int action, int mods)
{
    if (_windows.count(window) != 0)
    {
        _windows[window]->onMouseButtonPressed(button, action);
    }
}

void OpenGLWindow::onMouseWheelScrollStatic(GLFWwindow* window, double scrollOffsetX, double scrollOffsetY)
{
    if (_windows.count(window) != 0)
    {
        _windows[window]->onMouseWheelScroll(scrollOffsetX, scrollOffsetY);
    }
}

void OpenGLWindow::drop_callback(GLFWwindow* window, int count, const char** paths)
{
    vector<string> string_vector;
    string_vector.push_back(util::encoding::utf8_to_acp(paths[0]));

    cout << "drop count : " << count << endl;
    cout << "paths : " << string_vector[0] << endl;
}
