#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <tchar.h>
#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <array>
#include <list>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>
#include <stddef.h>
#include <random>
#include <thread>

#include <Windows.h>
#include <cstring>
#include <mmsystem.h>
#include <time.h>

#pragma comment(lib,"winmm.lib")

using namespace std;

// opengl
#pragma comment(lib, "OpenGL32.lib")

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

//#include <ft2build.h>
//#include FT_FREETYPE_H  
//#pragma comment(lib,"freetype.lib")

#include "stb_image.h"

#include "Encoding.h"

class OpenGLWindow;

#endif //PCH_H
