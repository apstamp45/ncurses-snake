#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>

// The tail count the snake will start with
#define STARTING_LENGTH 3

// The starting speed (each loop will delay by 1000 / speed millis)
#define STARTING_SPEED 3

enum direction {
	UP,
	DOWN,
	LEFT,
	RIGHT
} typedef direction;

struct head {
	int x;
	int y;
	direction d;
} typedef head;

struct segment {
	int x;
	int y;
} typedef segment;

struct snake {
	head h;
	// The extra space is for the clearing segment
	segment t[STARTING_LENGTH + 1];
	int tc;
} typedef snake;

struct apple {
	int x;
	int y;
} typedef apple;

// For passing args to the threads
struct threadargs {
	bool* isrunning;
	snake* s;
	int* speed;
	int* lastkey;
	apple* a;
	int height;
	int width;
} typedef threadargs;

int randinrange(int min, int max) {
	return rand() % (max + 1 - min) + min;
}

// Returns num if min <= x < max,
// else min if x < min, or max - 1 if x >= max
int constrain(int num, int min, int max) {
	return (!(num < min || num >= max)) ? num : (num < min) ? min : max - 1;
}

void drawsquare(int y, int x, int height, int width, int colorpair) {
	attron(COLOR_PAIR(colorpair));
	mvaddch(constrain(y, 0, height), constrain(x, 0, width) * 2, ' ');
	addch(' ');
	attroff(COLOR_PAIR(colorpair));
}

void movesnake(snake* s, int height, int width) {
	for (int i = s->tc; i > 0; i--) {
		s->t[i].x = s->t[i - 1].x;
		s->t[i].y = s->t[i - 1].y;
	}
	s->t[0].x = s->h.x;
	s->t[0].y = s->h.y;
	switch (s->h.d) {
		case UP:
			s->h.y -= 1;
			break;
		case DOWN:
			s->h.y += 1;
			break;
		case LEFT:
			s->h.x -= 1;
			break;
		case RIGHT:
			s->h.x += 1;
			break;
	}
	if (s->h.y >= height) {
		s->h.y = 0;
	} else if (s->h.y < 0) {
		s->h.y = height - 1;
	}
	if (s->h.x >= width) {
		s->h.x = 0;
	} else if (s->h.x < 0) {
		s->h.x = width - 1;
	}
}

void rendersnake(snake s, int height, int width) {
	drawsquare(s.t[s.tc].y, s.t[s.tc].x, height, width, 0);
	drawsquare(s.h.y, s.h.x, height, width, 1);
}

void moveapple(apple* a, snake s, int height, int width) {
	int x, y;
	x = randinrange(0, width - 1);
	y = randinrange(0, height - 1);
	for (int i = 0; i <= s.tc; i++) {
		if (s.t[i].x != x || s.t[i].y != y) {
			break;
		} else {
			i = 0;
			x = randinrange(0, width - 1);
			y = randinrange(0, height - 1);
		}
	}
	a->x = x;
	a->y = y;
}

void renderapple(apple a, int height, int width) {
	drawsquare(a.x, a.y, height, width, 2);
}

void* handlekeys(void* args) {
	threadargs* arg = (threadargs*) args;
	while (*(arg->isrunning)) {
		*(arg->lastkey) = getch();
		if (*(arg->lastkey) == 27) {
			break;
		}
	}
	pthread_exit(NULL);
}

void* gameloop(void* args) {
	threadargs* arg = (threadargs*) args;
	erase();
	while (*(arg->isrunning)) {
		rendersnake(*(arg->s), arg->height, arg->width);
		renderapple(*(arg->a), arg->height, arg->width);
		movesnake(arg->s, arg->height, arg->width);
		refresh();
		// Delay for 1000 / speed millis
		clock_t start = clock();
		while (((double) (clock() - start) / CLOCKS_PER_SEC) * 1000.0 < (1000 / *(arg->speed)) && *(arg->isrunning));
		switch (*(arg->lastkey)) {
			case KEY_UP:
				if (arg->s->h.d != DOWN) {
					arg->s->h.d = UP;
				}
				break;
			case KEY_DOWN:
				if (arg->s->h.d != UP) {
					arg->s->h.d = DOWN;
				}
				break;
			case KEY_LEFT:
				if (arg->s->h.d != RIGHT) {
					arg->s->h.d = LEFT;
				}
				break;
			case KEY_RIGHT:
				if (arg->s->h.d != LEFT) {
					arg->s->h.d = RIGHT;
				}
				break;
			case 27: // Escape key
				*(arg->isrunning) = 0;
				break;
		}
	}
	pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
	srand(time(NULL));
	WINDOW* w;
	snake s;
	bool isrunning = 1;
	int speed = STARTING_SPEED;
	int lastkey;
	apple a;
	// Init window
	w = initscr();
	noecho();
	int mode = curs_set(0);
	cbreak();
	keypad(w, true);
	start_color();
	init_pair(1, COLOR_GREEN, COLOR_GREEN);
	init_pair(2, COLOR_RED, COLOR_RED);
	int height = getmaxy(w);
	int width = getmaxx(w) / 2;
	// Init snake
	s.h.x = width / 2;
	s.h.y = height / 2;
	s.h.d = RIGHT;
	s.tc = 0;
	for (int i = 0; i < STARTING_LENGTH + 1; i++) {
		s.t[i].x = (s.h.d != LEFT) ? (s.h.x - 1 - i) : (s.h.x + 1 + i);
		s.t[i].y = s.h.y;
		if (i < STARTING_LENGTH) {
			drawsquare(s.t[i].y, s.t[i].x, height, width, 1);
		}
		s.tc++;
	}
	s.tc--;
	// Init apple
	moveapple(&a, s, width, height);
	// Set thread args
	threadargs args;
	threadargs* arg = &args;
	arg->isrunning = &isrunning;
	arg->s = &s;
	arg->speed = &speed;
	arg->lastkey = &lastkey;
	arg->a = &a;
	arg->height = height;
	arg->width = width;
	pthread_t key, game;
	// Run threads
	if (pthread_create(&key, NULL, &handlekeys, arg) != 0) {
		return 1;
	}
	if (pthread_create(&game, NULL, &gameloop, arg) != 0) {
		return 2;
	}
	if (pthread_join(key, NULL) != 0) {
		return 3;
	}
	if (pthread_join(game, NULL) != 0) {
		return 4;
	}
	curs_set(mode);
	endwin();
	return 0;
}
