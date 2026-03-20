#pragma once

#ifndef TEXTURE2D_H_
#define TEXTURE2D_H_

#include <string>

class Texture2D
{
public:
	int width;
	int height;
	unsigned int ID;

	void use();
};

class TextureSystem
{
public:
	static Texture2D Generate(const char* filename);
	static Texture2D Generate(std::string image);
	static void Delete(Texture2D& obj);
};

#endif // !TEXTURE2D_H_