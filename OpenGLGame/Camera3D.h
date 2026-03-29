#pragma once

#ifndef CAMERA3D_H_
#define CAMERA3D_H_

#include "pch.h"

class Camera3D
{
public:
    glm::vec3 Position = glm::vec3(0.0f, 180.0f, 420.0f);
    float Yaw = -90.0f;
    float Pitch = -18.0f;
    float MovementSpeed = 260.0f;
    float MouseSensitivity = 0.12f;
    float Zoom = 45.0f;

public:
    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(Position, Position + GetFrontVector(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 GetFrontVector() const
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        return glm::normalize(front);
    }

    glm::vec3 GetRightVector() const
    {
        return glm::normalize(glm::cross(GetFrontVector(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    void MoveForward(float amount)
    {
        Position += GetFrontVector() * amount;
    }

    void MoveRight(float amount)
    {
        Position += GetRightVector() * amount;
    }

    void MoveUp(float amount)
    {
        Position.y += amount;
    }

    void Rotate(float yawOffset, float pitchOffset)
    {
        Yaw += yawOffset * MouseSensitivity;
        Pitch += pitchOffset * MouseSensitivity;
        Pitch = glm::clamp(Pitch, -89.0f, 89.0f);
    }
};

#endif
