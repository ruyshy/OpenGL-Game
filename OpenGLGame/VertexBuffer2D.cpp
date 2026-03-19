#include "pch.h"
#include "VertexBuffer2D.h"

const unsigned int VertexBufferSystem2D::indices[6] = {
	0, 1, 2, // first triangle
	2, 1, 3  // second triangle
};

void VertexBufferObject2D::Draw()
{
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

VertexBufferObject2D VertexBufferSystem2D::Generate()
{
	VertexBufferObject2D result;

	vec2 arr[4] = {
		vec2(0,0),
		vec2(0,1),
		vec2(1,0),
		vec2(1,1)
	};

	vec4 colors[4] = {
		vec4(1,1,1,1),
		vec4(1,1,1,1),
		vec4(1,1,1,1),
		vec4(1,1,1,1)
	};

	vec2 uv[4] = {
		vec2(0,0),
		vec2(0,1),
		vec2(1,0),
		vec2(1,1)
	};

	glGenVertexArrays(1, &result.VAO);
	glGenBuffers(1, &result.VBO);
	glGenBuffers(1, &result.EBO);
	glGenBuffers(1, &result.CBO);
	glGenBuffers(1, &result.UVBO);


	glBindVertexArray(result.VAO);

	// for vertex buffer ..
	glBindBuffer(GL_ARRAY_BUFFER, result.VBO);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vec2), arr, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, NULL);

	// for color buffer ..
	glBindBuffer(GL_ARRAY_BUFFER, result.CBO);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vec4), colors, GL_STATIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, false, 0, NULL);

	// for uv buffer ..
	glBindBuffer(GL_ARRAY_BUFFER, result.UVBO);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vec2), uv, GL_STATIC_DRAW);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, false, 0, NULL);


	glBindBuffer(GL_ARRAY_BUFFER, 0);


	return result;
}

void VertexBufferSystem2D::Delete(VertexBufferObject2D& obj)
{
	glDeleteVertexArrays(1, &obj.VAO);
	glDeleteBuffers(1, &obj.VBO);
	glDeleteBuffers(1, &obj.EBO);
	glDeleteBuffers(1, &obj.CBO);
	glDeleteBuffers(1, &obj.UVBO);
}
