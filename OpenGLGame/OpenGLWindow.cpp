#include "pch.h"
#include "OpenGLWindow.h"

map<GLFWwindow*, OpenGLWindow*> OpenGLWindow::_windows;

namespace
{
    constexpr double kMaxFrameDeltaSeconds = 0.1;

    void glfwErrorCallback(int errorCode, const char* description)
    {
        cerr << "GLFW error (" << errorCode << "): " << (description != nullptr ? description : "unknown") << endl;
    }
}

OpenGLWindow::OpenGLWindow()
{
    for (auto& kwp : _keyWasPressed)
    {
        kwp = false;
    }
}

bool OpenGLWindow::createOpenGLWindow(const string& windowTitle, int majorVersion, int minorVersion, int width, int height, bool showFullscreen)
{
    OpenGLWindowSettings settings;
    settings.title = windowTitle;
    settings.majorVersion = majorVersion;
    settings.minorVersion = minorVersion;
    settings.width = width;
    settings.height = height;
    settings.showFullscreen = showFullscreen;
    return createOpenGLWindow(settings);
}

bool OpenGLWindow::createOpenGLWindow(const OpenGLWindowSettings& settings)
{
    glfwSetErrorCallback(glfwErrorCallback);
    if (glfwInit() == GLFW_FALSE)
    {
        cerr << "Failed to initialize GLFW." << endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, settings.majorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, settings.minorVersion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    _window = glfwCreateWindow(
        settings.width,
        settings.height,
        settings.title.c_str(),
        settings.showFullscreen ? glfwGetPrimaryMonitor() : nullptr,
        nullptr);
    if (_window == nullptr)
    {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(_window);
    if (!initializeGlad())
    {
        glfwDestroyWindow(_window);
        _window = nullptr;
        glfwTerminate();
        return false;
    }

    glfwSetWindowSizeCallback(_window, onWindowSizeChangedStatic);
    glfwSetDropCallback(_window, drop_callback);
    glfwSetMouseButtonCallback(_window, onMouseButtonPressedStatic);
    glfwSetScrollCallback(_window, onMouseWheelScrollStatic);
    glfwSetKeyCallback(_window, onKeyChangedStatic);
    _windows[_window] = this;

    if (settings.showFullscreen)
    {
        glfwMaximizeWindow(_window);
        // After calling glfwMaximizeWindow, the onWindowSizeChanged somehow does not get called. Therefore I call it manually.
        int wwidth, hheight;
        glfwGetWindowSize(_window, &wwidth, &hheight);
        onWindowSizeChangedInternal(wwidth, hheight);
    }
    else
    {
        onWindowSizeChangedInternal(settings.width, settings.height);
    }

    applyWindowStyle();
    tryLoadWindowIcon();
    setVerticalSynchronization(settings.enableVSync);

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
    if (_window == nullptr)
    {
        cerr << "runApp called without a valid window." << endl;
        _hasErrorOccured = true;
        return;
    }

    recalculateProjectionMatrix();
    initializeScene();
    _sceneInitialized = true;

    // Update time at the beginning, so that calculations are correct
    _lastFrameTime = _lastFrameTimeFPS = glfwGetTime();

    while (glfwWindowShouldClose(_window) == 0)
    {
        updateDeltaTimeAndFPS();
        Render();
        updateScene();
    }

    if (_sceneInitialized)
    {
        releaseScene();
        _sceneInitialized = false;
    }

    auto* window = _window;
    glfwDestroyWindow(window);
    if (const auto iterator = _windows.find(window); iterator != _windows.end())
    {
        _windows.erase(iterator);
    }
    _window = nullptr;

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
    if (_window != nullptr)
    {
        glfwSetWindowShouldClose(_window, true);
    }
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
    if (_window == nullptr)
    {
        return;
    }

    renderScene();

    glfwSwapBuffers(_window);
    glfwPollEvents();
}

void OpenGLWindow::recalculateProjectionMatrix()
{
    if (_window == nullptr)
    {
        return;
    }

    int width, height;
    glfwGetWindowSize(getWindow(), &width, &height);
    if (width <= 0 || height <= 0)  return;
    _projectionMatrix = perspective(radians(57.0f), static_cast<float>(width) / static_cast<float>(height), 0.5f, 1500.0f);
    _orthoMatrix = ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
}

void OpenGLWindow::updateDeltaTimeAndFPS()
{
    const auto currentTime = glfwGetTime();
    _timeDelta = std::min(currentTime - _lastFrameTime, kMaxFrameDeltaSeconds);
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
    if (width <= 0 || height <= 0)
    {
        return;
    }

    screenWidth_ = width;
    screenHeight_ = height;
    recalculateProjectionMatrix();
    glViewport(0, 0, width, height);
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
    (void)mods;

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

void OpenGLWindow::onKeyChangedStatic(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;

    if (_windows.count(window) != 0)
    {
        _windows[window]->onKeyChanged(key, action);
    }
}

void OpenGLWindow::drop_callback(GLFWwindow* window, int count, const char** paths)
{
    if (count <= 0 || paths == nullptr || paths[0] == nullptr)
    {
        return;
    }

    vector<string> string_vector;
    string_vector.push_back(util::encoding::utf8_to_acp(paths[0]));

    cout << "drop count : " << count << endl;
    cout << "paths : " << string_vector[0] << endl;
}

bool OpenGLWindow::initializeGlad() const
{
    const auto loaded = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
    if (loaded == 0)
    {
        cerr << "Failed to initialize GLAD." << endl;
        return false;
    }

    return true;
}

void OpenGLWindow::applyWindowStyle() const
{
    if (_window == nullptr)
    {
        return;
    }

    auto* nativeWindow = glfwGetWin32Window(_window);
    if (nativeWindow == nullptr)
    {
        return;
    }

    LONG_PTR style = GetWindowLongPtr(nativeWindow, GWL_STYLE);
    style &= ~WS_MAXIMIZEBOX;
    SetWindowLongPtr(nativeWindow, GWL_STYLE, style);
}

void OpenGLWindow::tryLoadWindowIcon() const
{
    if (_window == nullptr)
    {
        return;
    }

    GLFWimage image{};
    image.pixels = stbi_load(".\\Image\\icon.png", &image.width, &image.height, 0, 4);
    if (image.pixels == nullptr)
    {
        return;
    }

    glfwSetWindowIcon(_window, 1, &image);
    stbi_image_free(image.pixels);
}
