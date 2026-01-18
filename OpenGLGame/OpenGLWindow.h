#pragma once

#ifndef OPENGOWINDOW_H_
#define OPENGOWINDOW_H_

class OpenGLWindow
{
public:
    OpenGLWindow();
    virtual ~OpenGLWindow() {}

    bool createOpenGLWindow(const std::string& windowTitle, int majorVersion, int minorVersion, int width =  800, int height = 600, bool showFullscreen = false);
    GLFWwindow* getWindow() const;
    void runApp();
    virtual void initializeScene() {}
    virtual void renderScene() {}
    virtual void updateScene() {}
    virtual void releaseScene() {}
    bool keyPressed(int keyCode) const;
    bool keyPressedOnce(int keyCode);
    void closeWindow(bool errorHasOccured = false);
    bool hasErrorOccured() const;
    mat4 getProjectionMatrix() const;
    mat4 getOrthoProjectionMatrix() const;
    float sof(float value) const;
    double sof(double value) const;
    double getTimeDelta() const;
    int getFPS() const;
    void setVerticalSynchronization(bool enable);
    bool isVerticalSynchronizationEnabled() const;
    int getScreenWidth() const;
    int getScreenHeight() const;
    ivec2 getOpenGLCursorPosition() const;
    static OpenGLWindow* getDefaultWindow();

public:
    void Render();

protected:
    virtual void onWindowSizeChanged(int width, int height) {}
    virtual void onMouseButtonPressed(int button, int action) {}
    virtual void onMouseWheelScroll(double scrollOffsetX, double scrollOffsetY) {}

private:
    GLFWwindow* _window = nullptr;
    bool _keyWasPressed[512] = { 0, };
    bool _hasErrorOccured = false;

    mat4 _projectionMatrix = mat4(1.0f);
    mat4 _orthoMatrix = mat4(1.0f);
    void recalculateProjectionMatrix();

    double _lastFrameTime = 0.0;
    double _lastFrameTimeFPS = 0.0;
    double _timeDelta = 0.0;
    int _FPS = 0;
    int _nextFPS = 0;
    bool _isVerticalSynchronizationEnabled = false;

    bool _hasMouseBeenScrolled = false;
    double _scrollOffsetX = 0.0;
    double _scrollOffsetY = 0.0;

    int screenWidth_{ 0 };
    int screenHeight_{ 0 };
    void updateDeltaTimeAndFPS();
    void onWindowSizeChangedInternal(int width, int height);
    static void onWindowSizeChangedStatic(GLFWwindow* window, int width, int height);
    static void onMouseButtonPressedStatic(GLFWwindow* window, int button, int action, int mods);
    static void onMouseWheelScrollStatic(GLFWwindow* window, double scrollOffsetX, double scrollOffsetY);
    static void drop_callback(GLFWwindow* window, int count, const char** paths);

    static map<GLFWwindow*, OpenGLWindow*> _windows;
};


#endif // !OPENGOWINDOW_H_