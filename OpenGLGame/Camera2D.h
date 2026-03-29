#pragma once

#ifndef CAMERA2D_H_
#define CAMERA2D_H_

#include "pch.h"

class Camera2D
{
public:
    void SetPosition(const glm::vec2& position)
    {
        mPosition = position;
    }

    void Move(const glm::vec2& offset)
    {
        mPosition += offset;
    }

    void SetViewportSize(float width, float height)
    {
        mViewportSize.x = std::max(width, 1.0f);
        mViewportSize.y = std::max(height, 1.0f);
    }

    void SetZoom(float zoom)
    {
        mZoom = std::max(zoom, 0.01f);
    }

    void SetRotation(float degrees)
    {
        mRotationDegrees = degrees;
    }

    glm::vec2 GetPosition() const
    {
        return mPosition;
    }

    glm::vec2 GetViewportSize() const
    {
        return mViewportSize;
    }

    float GetZoom() const
    {
        return mZoom;
    }

    float GetRotation() const
    {
        return mRotationDegrees;
    }

    glm::mat4 GetViewMatrix() const
    {
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(-mPosition, 0.0f));
        view = glm::translate(view, glm::vec3(mViewportSize * 0.5f, 0.0f));
        view = glm::rotate(view, glm::radians(-mRotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
        view = glm::scale(view, glm::vec3(mZoom, mZoom, 1.0f));
        view = glm::translate(view, glm::vec3(-(mViewportSize * 0.5f), 0.0f));
        return view;
    }

    glm::mat4 GetProjectionMatrix() const
    {
        return glm::ortho(0.0f, mViewportSize.x, 0.0f, mViewportSize.y);
    }

    glm::mat4 GetViewProjectionMatrix() const
    {
        return GetProjectionMatrix() * GetViewMatrix();
    }

private:
    glm::vec2 mPosition = glm::vec2(0.0f);
    glm::vec2 mViewportSize = glm::vec2(800.0f, 600.0f);
    float mZoom = 1.0f;
    float mRotationDegrees = 0.0f;
};

#endif // !CAMERA2D_H_
