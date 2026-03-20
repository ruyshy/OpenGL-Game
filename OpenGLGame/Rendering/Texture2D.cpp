#include "pch.h"
#include "Texture2D.h"

void Texture2D::use()
{
	if (this == nullptr)
		return;

	glBindTexture(GL_TEXTURE_2D, ID);
}

Texture2D TextureSystem::Generate(const char* filename)
{
	Texture2D result{};
	result.width = 1;
	result.height = 1;

	int width, height, numcomponents;
	unsigned char* data;
	unsigned int textureID;

	glGenTextures(1, &textureID);

	data = stbi_load(filename, &width, &height, &numcomponents, STBI_rgb_alpha);

	result.width = width;
	result.height = height;
	result.ID = textureID;

	if (data)
	{
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// ********* texture minimize & maxmize operation
		// to solve pixlation problem
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //GL_LINEAR_MIPMAP_NEAREST
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		cout << "failed to load image : " << filename << endl;
		cout << "============================================" << endl;

		const unsigned char fallbackPixel[4] = { 255, 0, 255, 255 };
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallbackPixel);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	stbi_image_free(data);

	return result;
}

Texture2D TextureSystem::Generate(std::string image)
{
	Texture2D result{};
	result.width = 1;
	result.height = 1;

	string copy = image;
	int width, height, numcomponents;
	unsigned char* content = ((unsigned char*)copy.c_str());
	int len = copy.length();
	unsigned char* data;
	unsigned int textureID;

	glGenTextures(1, &textureID);

	data = stbi_load_from_memory(content, len, &width, &height, &numcomponents, STBI_rgb_alpha);

	result.width = width;
	result.height = height;
	result.ID = textureID;

	if (data)
	{
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// ********* texture minimize & maxmize operation
		// to solve pixlation problem
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //GL_LINEAR_MIPMAP_NEAREST
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		cout << "failed to load image " << endl;
		cout << "============================================" << endl;

		const unsigned char fallbackPixel[4] = { 255, 0, 255, 255 };
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallbackPixel);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	stbi_image_free(data);

	return result;
}

void TextureSystem::Delete(Texture2D& obj)
{
	glDeleteTextures(1, &obj.ID);
}
