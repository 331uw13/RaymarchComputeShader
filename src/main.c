#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <math.h>
#include "gl3w.h"
#include <GLFW/glfw3.h>

#include "shader.h"


typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef long unsigned int  u64;

static GLFWwindow* window;

#define OPENGL_VERSION_MA 4
#define OPENGL_VERSION_MI 3

static float cursor_x = 0.0;
static float cursor_y = 0.0;
static float p_cursor_x = 0.0;
static float p_cursor_y = 0.0;

#define PI_R (M_PI/180.0)


void cleanup() {
	if(window != NULL) { glfwDestroyWindow(window); }
	glfwTerminate();
	puts("bye.");
}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {
	
	p_cursor_x = cursor_x;
	p_cursor_y = cursor_y;
	
	cursor_x += x-p_cursor_x;
	cursor_y += y-p_cursor_y;
}

#define RES_X 1200
#define RES_Y 800

u8 init() {
	u8 res = 0;

	if(!glfwInit()) {
		fprintf(stderr, "Failed to initialize glfw!\n");
		goto finish;
	}

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_VERSION_MA);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_VERSION_MI);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_TRUE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
	
	glfwWindowHint(GLFW_SAMPLES, 0);


	if((window = glfwCreateWindow(RES_X, RES_Y, "testing ideas.", NULL, NULL)) == NULL) {
		fprintf(stderr, "Failed to create window\n");
		goto finish;
	}

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwMakeContextCurrent(window);

	if(gl3wInit() != GL3W_OK) {
		fprintf(stderr, "Failed to initialize OpenGL\n");
		goto finish;
	}

	if(!gl3wIsSupported(OPENGL_VERSION_MA, OPENGL_VERSION_MI)) {
		fprintf(stderr, "OpenGl version %i.%i doesnt seem to be supported on this machine...\n",
				OPENGL_VERSION_MA, OPENGL_VERSION_MI);
		goto finish;
	}


	cursor_x = p_cursor_x = 0.0;
	cursor_y = p_cursor_y = 0.0;
	
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetCursorPos(window, cursor_x, cursor_y);


	res = 1;

finish:
	return res;
}


void main_loop() {


	int texture_program = 0;
	int compute_program = 0;
	


	{

		int fd = open("raymarch.glsl", O_RDONLY);	
		struct stat sb;

		if(fd < 0) {
			fprintf(stderr, "Failed to open shader file!\n");
			perror("open");
			return;
		}

		if(fstat(fd, &sb) < 0) {
			fprintf(stderr, "Failed to get file status!\n");
			perror("fstat");
			close(fd);
			return;
		}

		printf("st_size: %li\n", sb.st_size);


		char* buf = malloc(sb.st_size+1);

		if(read(fd, buf, sb.st_size) < 0) {
			fprintf(stderr, "Failed to read file content!\n");
			perror("read");
			free(buf);
			close(fd);
			return;
		}

		buf[sb.st_size] = '\0';

		int shader = compile_shader((const char*)buf, GL_COMPUTE_SHADER);
		compute_program = create_program(&shader, 1);

		free(buf);
		close(fd);
		
	}


	{
		const char* vs_src = 
			"#version 430 core\n"
			"in vec2 pos;"
			"out vec2 tex_coord;"

			"void main() {"
				"gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);"
				"tex_coord = pos+1.0;"
			"}";

		const char* fs_src = 
			"#version 430 core\n"
			"uniform sampler2D texture0;"
			"out vec4 out_color;"
			"in vec2 tex_coord;"
			
			"void main() {"
				"out_color = texture(texture0, tex_coord*0.25);"
			"}";

		int s[] = {
			compile_shader(vs_src, GL_VERTEX_SHADER),
			compile_shader(fs_src, GL_FRAGMENT_SHADER)
		};

		texture_program = create_program(s, 2);
		
		glDeleteShader(s[0]);
		glDeleteShader(s[1]);
	}



	u32 texture = 0;

	int texture_w = 640/2;
	int texture_h = 480/2;

	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	

	int format = GL_RGBA16F;
	
	glTexImage2D(GL_TEXTURE_2D, 0, format, texture_w, texture_h, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, format);


	float points[] = {
		-1.0, -1.0,
		-1.0,  1.0,
		 1.0, -1.0,
		 1.0,  1.0
	};

	u32 vao = 0;
	u32 vbo = 0;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof points, points, GL_STATIC_DRAW);
	
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0);
	glEnableVertexAttribArray(0);
	
	glfwSwapInterval(1);

	int detail = 2;

	int test_w = texture_w/detail;
	int test_h = texture_h/detail;
	

	glBindTexture(GL_TEXTURE_2D, texture);
	glBindVertexArray(vao);

	const int time_uniform = glGetUniformLocation(compute_program, "time");
	const int camera_uniform = glGetUniformLocation(compute_program, "camera");
	const int player_uniform = glGetUniformLocation(compute_program, "player");

	float player_x = 5.0;
	float player_y = 0.0;
	float player_z = 0.0;


	glClearColor(0.0, 0.0, 0.0, 1.0);

	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, 1);
		}


		float look_x = -(0.2*cursor_x)*PI_R;
		float look_y = -(0.2*cursor_y)*PI_R;
		
		float dir_x = cos(look_x)*cos(look_y);
		float dir_z = sin(look_x)*cos(look_y);


		float speed = 0.25;
		if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			player_z += speed*dir_x;
			player_x -= speed*dir_z;
		}

		if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			player_z -= speed*dir_x;
			player_x += speed*dir_z;
		}

		glUseProgram(compute_program);
		glUniform1f(time_uniform, glfwGetTime());
		glUniform2f(camera_uniform, look_x, look_y);
		glUniform3f(player_uniform, player_x, player_y, player_z);


		glDispatchCompute(test_w, test_h, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glClear(GL_COLOR_BUFFER_BIT);
		

		glUseProgram(texture_program);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		glfwSwapBuffers(window);
	}

	glDeleteProgram(texture_program);
	glDeleteProgram(compute_program);
	glDeleteTextures(1, &texture);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}



int main() {
	window = NULL;


	if(init()) {
		puts("initialized!");
		main_loop();
	}

	cleanup();
	return 0;
}









