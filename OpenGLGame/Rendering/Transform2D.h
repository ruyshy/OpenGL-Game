#pragma once

#ifndef TRANSFORM2D_H_
#define TRANSFORM2D_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Transform2D
{
public:
	void* operator new(size_t sz) { return malloc(sz); }
	void operator delete(void* p) noexcept { free(p); }

private:
	glm::vec2 mPosition = glm::vec2(0);
	glm::vec2 mScale = glm::vec2(1);

	float mAngle = 0;
	bool mFlipX = false, mFlipY = false;

public:
	glm::mat4 Get()
	{
		glm::vec2 position = mPosition;
		glm::vec2 scale = mScale;

		if (mFlipX)
		{
			position = glm::vec2(position.x + scale.x, position.y);
			scale = glm::vec2(-(fabs(scale.x)), scale.y);
		}
		if (mFlipY)
		{
			position = glm::vec2(position.x, position.y + scale.y);
			scale = glm::vec2(scale.x, -(fabs(scale.y)));
		}

		glm::mat4 model = glm::mat4(1.0f);

		model = glm::translate(model, glm::vec3(position, 0.0f));
		model = glm::translate(model, glm::vec3(0.5f * scale.x, 0.5f * scale.y, 0.0f));
		model = glm::rotate(model, glm::radians(mAngle), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::translate(model, glm::vec3(-0.5f * scale.x, -0.5f * scale.y, 0.0f));

		model = glm::scale(model, glm::vec3(scale, 1.0f));

		return model;
	}

	glm::vec2 GetPosition() { return mPosition; }
	glm::vec2 GetCenter() { return glm::vec2((mPosition.x - (mScale.x / 2)), (mPosition.y - (mScale.y / 2))); }
	glm::vec2 GetScale() { return mScale; }
	float GetAngle() { return mAngle; }
	bool GetFlipX() { return mFlipX; }
	bool GetFlipY() { return mFlipY; }

	void SetPosition(glm::vec2 position) { mPosition = position; }
	void SetScale(glm::vec2 scale) { mScale = scale; }
	void SetAngle(float angle) { mAngle = angle; }
	void SetFlipX(bool flip) { mFlipX = flip; }
	void SetFlipY(bool flip) { mFlipY = flip; }

};

#endif // !TRANSFORM2D_H_
