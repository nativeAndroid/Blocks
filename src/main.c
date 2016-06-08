#include <stdio.h>

#include <SDL.h>
#include <GL/glew.h>

#include "state.h"

#define STACK_MAX 255

//FUNCTION DEFS****

static void cleanup();//shuts everything down for a reboot
static void init_chunk();//contains the startup code
static void runevent(enum states s, enum events e);
static void push(enum states newstate);
static void pop();

static int isrunning = 1;

struct statestack_s {
	enum states states[STACK_MAX];
	int instances[MAX_STATES];
	int top;
};

struct statestack_s stack = { {0} };
#define CURRENTSTATE (stack.states[stack.top])

int queueforpop = 0;
enum states queueforpush = MAX_STATES;

//the main window
SDL_Window *win;
SDL_GLContext glcontext;

static int windoww, windowh;

char *basepath;

static SDL_GameController *controller = NULL;

int
main(int argc, char *argv[])
{
	init_chunk();
	while(isrunning)
	{
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT)
			{
				state_exit();
			} else if(e.type == SDL_WINDOWEVENT)
			{
				if(e.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					state_window_update(e.window.data1, e.window.data2);
					windoww = e.window.data1;
					windowh = e.window.data2;
					state_mouse_center();
				}
			}
			(*statetable[CURRENTSTATE][SDLEVENT]) (&e);
		}
		runevent(CURRENTSTATE, RUN);

		if(queueforpop)
		{
			queueforpop = 0;
			pop();
		}
		if(queueforpush != MAX_STATES)
		{
			enum states state = queueforpush;
			queueforpush = MAX_STATES;
			push(state);
		}
	}

	cleanup();

	return 0;
}

void
state_queue_push(enum states state)
{
	if(queueforpush == MAX_STATES)
		queueforpush = state;
}

void
state_queue_pop()
{
	if(!queueforpop)
		queueforpop = 1;
}

static void
runevent(enum states s, enum events e)
{
	if(statetable[s][e])
		(*statetable[s][e]) (0);
}

static void
fail(const char *msg)
{
	printf("FAILED: %s\n", msg);
	exit(-2);
}

static void
push(enum states newstate)
{
	printf("STATUS: push state %i onto %i\n", newstate, CURRENTSTATE);

	runevent(CURRENTSTATE, PAUSE);

	if(stack.instances[newstate] == 0)
		runevent(newstate, INITALIZE);
	else
		runevent(newstate, RESUME);

	stack.instances[newstate]++;
	stack.top++;
	if(stack.top < STACK_MAX)
		stack.states[stack.top] = newstate;
	else
		exit(-1);
}

static void
pop()
{
	printf("STATUS: pop state %i\n", CURRENTSTATE);

	stack.instances[CURRENTSTATE]--;

	if(stack.instances[CURRENTSTATE] == 0)
		runevent(CURRENTSTATE, CLOSE);
	else
		runevent(CURRENTSTATE, PAUSE);

	stack.top--;
	if(stack.top >= 0)
		stack.states[stack.top + 1] = 0;
	else
		isrunning = 0;

	runevent(CURRENTSTATE, RESUME);
}

static void
cleanup()
{
	printf("STATUS: cleaning up for either an exit or a reboot\n");

	free(basepath);
	SDL_GameControllerClose(controller);
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(win);
	SDL_Quit();
}

static void init_chunk()
{
	printf("STATUS: initalizing the program\n");

	if(SDL_Init(SDL_INIT_EVERYTHING))
		//SDL2 init_chunk failed
		fail("SDL_Init(SDL_INIT_EVERYTHING)");

	win = SDL_CreateWindow("Blocks", 100, 100, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if(!win)
		//SDL2 didn't create a window
		fail("SDL_CreateWindow");

	for (int i = 0; i < SDL_NumJoysticks(); ++i) {
		if(SDL_IsGameController(i))
		{
			controller = SDL_GameControllerOpen(i);
			if(controller)
			{
				printf("Game controller found\n");
				break;
			} else {
				fprintf(stderr, "Could not open gamecontroller %i: %s\n", i, SDL_GetError());
			}
		}
	}

	glcontext = SDL_GL_CreateContext(win);
	if(!glcontext)
		//SDL2 didint create the required link with opengl
		fail("SDL_GL_CreateContext()");

	if(glewInit())
		//glew couldn't do to wrangling
		fail("glewInit()");

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	SDL_GetWindowSize(win, &windoww, &windowh);
	SDL_GL_SetSwapInterval(0);

	glClearColor(0,0,0,1);
	glFrontFace(GL_CCW);
//	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(win);

	basepath = SDL_GetBasePath();

	stack.instances[MENUMAIN] = 1;
	stack.top = 0;
	runevent(MENUMAIN, INITALIZE);
}

void
state_exit()
{
	//cleanup before closing.
	printf("STATUS: exiting the program.\n");
	isrunning = 0;
}

void
state_window_update(int w, int h)
{
	glViewport(0, 0, w, h);
	windoww = w;
	windowh = h;
}

void
state_window_get_size(int *w, int*h)
{
	*w = windoww;
	*h = windowh;
}

void
state_window_swap()
{
	SDL_GL_SwapWindow(win);
}

char *
state_basepath_get()
{
	return basepath;
}

void
state_mouse_center()
{
	SDL_WarpMouseInWindow(win, windoww/2, windowh/2);
}

int
state_has_controller()
{
	return controller ? 1 : 0;
}
