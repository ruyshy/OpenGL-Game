#include "pch.h"
#include "Sprite.h"

#include "Animation2D.h"
#include "Shader.h"

#include "VertexBuffer2D.h"
#include "Transform2D.h"
#include "Texture2D.h"

Sprite::Sprite(shared_ptr<Shader> shader, const char* filename)
	: mpShader(shader), mScreenX(0), mScreenY(0), mScreenW(800), mScreenH(600), mLastX(0), mLastY(0)
{
	mpTextured = make_shared<Texture2D>(TextureSystem::Generate(filename));
	mpVertexBufferObject = make_shared<VertexBufferObject2D>(VertexBufferSystem2D::Generate());
	mpTransform = make_shared<Transform2D>();
}

Sprite::Sprite(shared_ptr<Shader> shader, const char* filename, float x, float y, float xx, float yy)
	: mpShader(shader), mScreenX(x), mScreenY(y), mScreenW(xx), mScreenH(yy), mLastX(0), mLastY(0)
{
	mpTextured = make_shared<Texture2D>(TextureSystem::Generate(filename));
	mpVertexBufferObject = make_shared<VertexBufferObject2D>(VertexBufferSystem2D::Generate());
	mpTransform = make_shared<Transform2D>();
}

Sprite::~Sprite()
{
	TextureSystem::Delete(*mpTextured);
	VertexBufferSystem2D::Delete(*mpVertexBufferObject);
}

void Sprite::Draw()
{
	if (!mVisible)return;

	mpShader->use();
	mpShader->setFloat("zDepth", mZDepth);
	mpShader->setMat4("model_matrx", mpTransform->Get());
	mpShader->setVec4("tintColor", mTintColor);
	mpTextured->use();
	mpVertexBufferObject->Draw();
}

void Sprite::ApplyAnimation(Animation2D& animation, double deltaTime)
{
	if (mpTextured == nullptr || mpVertexBufferObject == nullptr)
	{
		return;
	}

	animation.play(*mpTextured, *mpVertexBufferObject, deltaTime);
}

bool Sprite::checkCollision(shared_ptr<Sprite> other, double offset)
{
	return
		(GetPosition().x + offset) < (other->GetPosition().x + other->GetScale().x) &&
		(GetPosition().x + GetScale().x - offset) > other->GetPosition().x &&
		(GetPosition().y + offset) < other->GetPosition().y + other->GetScale().y &&
		(GetPosition().y + GetScale().y - offset) > other->GetPosition().y;
}

bool Sprite::hasMoved()
{
	return GetPosition().x != mLastX || GetPosition().y != mLastY;
}

bool Sprite::hasScreen()
{
	return
		GetPosition().x + GetScale().x >= (mScreenX + GetScale().x) &&
		GetPosition().x <= (mScreenW - GetScale().x) &&
		GetPosition().y + GetScale().y >= (mScreenY + GetScale().y) &&
		GetPosition().y <= (mScreenH - GetScale().y);
}

bool Sprite::hasScreen(float width, float height)
{
	return
		GetPosition().x + GetScale().x >= (width + GetScale().x) &&
		GetPosition().x <= (width - GetScale().x) &&
		GetPosition().y + GetScale().y >= (height + GetScale().y) &&
		GetPosition().y <= (height - GetScale().y);
}

void Sprite::update()
{
	mLastX = GetPosition().x;
	mLastY = GetPosition().y;
}

shared_ptr<Transform2D> Sprite::GetTransform() { return mpTransform; }
shared_ptr<Texture2D> Sprite::GetTexture() { return mpTextured; }
shared_ptr<VertexBufferObject2D> Sprite::GetVertexBuffer() { return mpVertexBufferObject; }
bool Sprite::GetVisible() { return mVisible; }
vec2 Sprite::GetPosition() { return mpTransform->GetPosition(); }
vec2 Sprite::GetCenter() { return mpTransform->GetCenter(); }
vec2 Sprite::GetScale() { return mpTransform->GetScale(); }
mat4 Sprite::GetMatrix() { return mpTransform->Get(); }
float Sprite::GetAngle() { return mpTransform->GetAngle(); }
bool Sprite::GetFlipX() { return mpTransform->GetFlipX(); }
bool Sprite::GetFlipY() { return mpTransform->GetFlipY(); }
vec4 Sprite::GetScreen() { return vec4(mScreenX, mScreenY, mScreenW, mScreenH); }
vec4 Sprite::GetTintColor() { return mTintColor; }


//Set
void Sprite::SetTransform(Transform2D transform)
{
	mpTransform->SetPosition(transform.GetPosition());
	mpTransform->SetScale(transform.GetScale());
	mpTransform->SetAngle(transform.GetAngle());
	mpTransform->SetFlipX(transform.GetFlipX());
	mpTransform->SetFlipY(transform.GetFlipY());
}
void Sprite::SetVisible(bool visible) { mVisible = visible; }
void Sprite::SetPosition(vec2 position) { mpTransform->SetPosition(position); }
void Sprite::SetPosition(float x, float y) { mpTransform->SetPosition(vec2(x, y)); }
void Sprite::SetDepth(float value) { mZDepth = value; }
void Sprite::SetScale(vec2 scale) { mpTransform->SetScale(scale); }
void Sprite::SetScale(float x, float y) { mpTransform->SetScale(vec2(x, y)); }
void Sprite::SetAngle(float angle) { mpTransform->SetAngle(angle); }
void Sprite::SetFlipX(bool flip) { mpTransform->SetFlipX(flip); }
void Sprite::SetFlipY(bool flip) { mpTransform->SetFlipY(flip); }
void Sprite::SetTintColor(const vec4& color) { mTintColor = color; }
void Sprite::SetTintColor(float r, float g, float b, float a) { mTintColor = vec4(r, g, b, a); }
