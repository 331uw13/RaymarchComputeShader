#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

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




void cleanup() {
	if(window != NULL) { glfwDestroyWindow(window); }
	glfwTerminate();
	puts("bye.");
}


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
	//glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

	if((window = glfwCreateWindow(1200, 850, "testing ideas.", NULL, NULL)) == NULL) {
		fprintf(stderr, "Failed to create window\n");
		goto finish;
	}

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

	res = 1;

finish:
	return res;
}

void check_groups() {
	int g[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &g[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &g[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &g[2]);

	printf("max work group sizes:  (x=%i, y=%i, z=%i)\n", g[0], g[1], g[2]);

	
	memset(g, 0, sizeof *g * 3);

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &g[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &g[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &g[2]);

	printf("max work group counts: (x=%i, y=%i, z=%i)\n", g[0], g[1], g[2]);

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
		printf("\033[90m%s\033[0m\n", buf);


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
				"out_color = texture(texture0, tex_coord);"
			"}";

		int s[] = {
			compile_shader(vs_src, GL_VERTEX_SHADER),
			compile_shader(fs_src, GL_FRAGMENT_SHADER)
		};

		texture_program = create_program(s, 2);
		
		glDeleteShader(s[0]);
		glDeleteShader(s[1]);
	}


	check_groups();

	u32 texture = 0;

	int window_w = 0;
	int window_h = 0;
	glfwGetWindowSize(window, &window_w, &window_h);

	int texture_w = window_w/3;
	int texture_h = window_h/3;

	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, texture_w, texture_h, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

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

	glViewport(0.0, 0.0, window_w*2, window_h*2);


	glClearColor(0.0, 0.0, 0.0, 1.0);
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		glUseProgram(compute_program);
		glUniform1f(glGetUniformLocation(compute_program, "time"), glfwGetTime());

		glDispatchCompute(texture_w, texture_h, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glClear(GL_COLOR_BUFFER_BIT);
		
		glUseProgram(texture_program);
		glBindVertexArray(vao);
		glBindTexture(GL_TEXTURE_2D, texture);
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









