#pragma once

#ifndef ANIMATION2D_H_
#define ANIMATION2D_H_


class Animation2D
{
public:
	Animation2D(const char* filename);
	~Animation2D();


	void play(Texture2D& spritetexture, VertexBufferObject2D& rectangle, double deltatime);

	void set_animation_speed(float newspeed);


protected:
	double anim_cursor;
	int current_frame_indx;
	int frames_count;
	float speed;

	vector<vec4> frames;
};

#endif // !ANIMATION2D_H_


