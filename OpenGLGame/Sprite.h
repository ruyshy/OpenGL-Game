#pragma once

#ifndef SPRITE_H_
#define SPRITE_H_

#include <memory>

#include <glm/glm.hpp>

class Shader;
class VertexBufferObject2D;
class Transform2D;
class Texture2D;
class Animation2D;

class Sprite
{
public:
	Sprite(std::shared_ptr<Shader> shader, const char* filename);
	Sprite(std::shared_ptr<Shader> shader, const char* filename, float x, float y, float xx, float yy);
	~Sprite();


	void Draw();
	void PlayAnimation(Animation2D& animation, double deltaTime);

	bool checkCollision(std::shared_ptr<Sprite> other, double offset = 0.0f);
	bool hasMoved();
	bool hasScreen();
	bool hasScreen(float width, float height);
	void update();

	bool GetVisible();

	std::shared_ptr<Transform2D> GetTransform();
	glm::vec2 GetPosition();
	glm::vec2 GetCenter();
	glm::vec2 GetScale();
	glm::mat4 GetMatrix();
	float GetAngle();
	bool GetFlipX();
	bool GetFlipY();

	glm::vec4 GetScreen();

	void SetTransform(Transform2D transform);
	void SetVisible(bool visible);
	void SetPosition(glm::vec2 position);
	void SetPosition(float x, float y);
	void SetDepth(float value);
	void SetScale(glm::vec2 scale);
	void SetScale(float x, float y);
	void SetAngle(float angle);
	void SetFlipX(bool flip);
	void SetFlipY(bool flip);

private:
	std::shared_ptr<VertexBufferObject2D> mpVertexBufferObject;
	std::shared_ptr<Transform2D> mpTransform;
	float mScreenX, mScreenY, mScreenW, mScreenH;
	float mLastX, mLastY;
	float mZDepth = 0.0f;

	bool mVisible = true;

	std::shared_ptr<Shader> mpShader = nullptr;
	std::shared_ptr<Texture2D> mpTextured = nullptr;
};

#endif // !SPRITE_H_
