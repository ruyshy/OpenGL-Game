#pragma once

#ifndef VERTEXBUFFER2D_H_
#define VERTEXBUFFER2D_H_

class VertexBufferObject2D
{
public:
	unsigned int
		VAO,		// vertex array object
		VBO,		// vertex buffer object
		EBO,		// element buffer object
		CBO,		// color buffer object
		UVBO;	// uv buffer object

	void Draw();
};

class VertexBufferSystem2D
{
public:
	static const unsigned int indices[];
	static VertexBufferObject2D Generate();
	static void Delete(VertexBufferObject2D& obj);
};

#endif // !VERTEXBUFFER2D_H_
