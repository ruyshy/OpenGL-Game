#include "pch.h"
#include "Animation2D.h"

Animation2D::Animation2D(const char* filename)
	: anim_cursor(0),
	current_frame_indx(0),
	frames_count(0),
	speed(0.05f)
{
	FILE* fp = nullptr;

	const int bufferlen = 255;
	char line[bufferlen];

	fopen_s(&fp, filename, "r");

	if (fp == nullptr)
	{
		printf("error in reading animation file\n");
		return;
	}

	while (fgets(line, bufferlen, fp))
	{
		vector<int> result;

		stringstream ss(line);
		string token;
		while (getline(ss, token, ','))
		{
			result.push_back(stoi(token));
		}

		if (result.size() < 4)
		{
			continue;
		}

		vec4 frame = vec4(result[0], result[1], result[2], result[3]);
		frames.push_back(frame);
	}

	frames_count = static_cast<int>(frames.size());

	if (fp != nullptr)
	{
		fclose(fp);
	}
}

Animation2D::~Animation2D()
{

}

void Animation2D::play(Texture2D& spritetexture, VertexBufferObject2D& rectangle, double deltatime)
{
	if (frames_count == 0 || spritetexture.width <= 0 || spritetexture.height <= 0)
	{
		return;
	}

	anim_cursor += deltatime;

	if (anim_cursor > speed)
	{
		current_frame_indx = (current_frame_indx + 1) % frames_count;
		anim_cursor = 0;
	}

	vec4 frame = frames[current_frame_indx];

	// normalization
	frame.x /= spritetexture.width;
	frame.y /= spritetexture.height;
	frame.z /= spritetexture.width;
	frame.w /= spritetexture.height;

	vector<vec2> uv;

	uv = {
		vec2(frame.x,frame.y),
		vec2(frame.x, frame.y + frame.w),
		vec2(frame.x + frame.z, frame.y),
		vec2(frame.x + frame.z, frame.y + frame.w)
	};

	glBindVertexArray(rectangle.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, rectangle.UVBO);

	/*{ //realocation for memory
		glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vec2), &uv[0], GL_DYNAMIC_DRAW);
	}*/


	// best practice to send data to gpu memory..
	void* gpubuffer = nullptr;
	gpubuffer = glMapBufferRange(GL_ARRAY_BUFFER, 0, 4 * sizeof(vec2), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	if (gpubuffer == nullptr)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		return;
	}
	memcpy(gpubuffer, uv.data(), 4 * sizeof(vec2));
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Animation2D::set_animation_speed(float newspeed)
{
	speed = newspeed;
}
