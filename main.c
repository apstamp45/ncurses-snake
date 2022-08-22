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

bool isrunning;
int lastkey;
int speed;
int width;
int height;
snake s;
apple a;

int randinrange(int min, int max) {
	return rand() % (max + 1 - min) + min;
}

// Returns num if min <= x < max,
// else min if x < min, or max - 1 if x >= max
int constrain(int num, int min, int max) {
	return (!(num < min || num >= max)) ? num : (num < min) ? min : max - 1;
}

void drawsquare(int y, int x, int colorpair) {
	attron(COLOR_PAIR(colorpair));
	mvaddch(constrain(y, 0, height), constrain(x, 0, width) * 2, ' ');
	addch(' ');
	attroff(COLOR_PAIR(colorpair));
}

void clearwindow() {
	for (int i = 0; i < width * height; i++) {
		addch(' ');
	}
}

void movesnake() {
	for (int i = s.tc; i > 0; i--) {
		s.t[i].x = s.t[i - 1].x;
		s.t[i].y = s.t[i - 1].y;
	}
	s.t[0].x = s.h.x;
	s.t[0].y = s.h.y;
	switch (s.h.d) {
		case UP:
			s.h.y -= 1;
			break;
		case DOWN:
			s.h.y += 1;
			break;
		case LEFT:
			s.h.x -= 1;
			break;
		case RIGHT:
			s.h.x += 1;
			break;
	}
	if (s.h.y >= height) {
		s.h.y = 0;
	} else if (s.h.y < 0) {
		s.h.y = height - 1;
	}
	if (s.h.x >= width) {
		s.h.x = 0;
	} else if (s.h.x < 0) {
		s.h.x = width - 1;
	}
}

void rendersnake() {
	drawsquare(s.t[s.tc].y, s.t[s.tc].x, 0);
	drawsquare(s.h.y, s.h.x, 1);
}

void moveapple() {
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
	a.x = x;
	a.y = y;
}

void renderapple() {
	drawsquare(a.y, a.x, 2);
}

void* handlekeys() {
	while (isrunning) {
		lastkey = getch();
		if (lastkey == 27) {
			break;
		}
	}
	pthread_exit(NULL);
}

void* gameloop() {
	clearwindow();
	for (int i = 0; i < s.tc; i++) {
		drawsquare(s.t[i].y, s.t[i].x, 1);
	}
	while (isrunning) {
		rendersnake();
		renderapple();
		movesnake();
		refresh();
		// Delay for 1000 / speed millis
		clock_t start = clock();
		while (((double) (clock() - start) / CLOCKS_PER_SEC) * 1000.0 < (1000 / speed) && isrunning);
		switch (lastkey) {
			case KEY_UP:
				if (s.h.d != DOWN) {
					s.h.d = UP;
				}
				break;
			case KEY_DOWN:
				if (s.h.d != UP) {
					s.h.d = DOWN;
				}
				break;
			case KEY_LEFT:
				if (s.h.d != RIGHT) {
					s.h.d = LEFT;
				}
				break;
			case KEY_RIGHT:
				if (s.h.d != LEFT) {
					s.h.d = RIGHT;
				}
				break;
			case 27: // Escape key
				isrunning = 0;
				break;
		}
		lastkey = 0;
	}
	pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
	srand(time(NULL));
	WINDOW* w;
	isrunning = 1;
	speed = STARTING_SPEED;
	// Init window
	w = initscr();
	noecho();
	int mode = curs_set(0);
	cbreak();
	keypad(w, true);
	start_color();
	init_pair(1, COLOR_GREEN, COLOR_GREEN);
	init_pair(2, COLOR_RED, COLOR_RED);
	height = getmaxy(w);
	width = getmaxx(w) / 2;
	// Init snake and apple
	s.h.x = width / 2;
	s.h.y = height / 2;
	s.h.d = RIGHT;
	s.tc = 0;
	for (int i = 0; i < STARTING_LENGTH + 1; i++) {
		s.t[i].x = (s.h.d != LEFT) ? (s.h.x - 1 - i) : (s.h.x + 1 + i);
		s.t[i].y = s.h.y;
		s.tc++;
	}
	s.tc--;
	moveapple();
	// Run threads
	pthread_t key, loop;
	if (pthread_create(&key, NULL, &handlekeys, NULL) != 0) {
		return 1;
	}
	if (pthread_create(&loop, NULL, &gameloop, NULL) != 0) {
		return 2;
	}
	if (pthread_join(key, NULL)) {
		return 3;
	}
	if (pthread_join(loop, NULL)) {
		return 4;
	}
	curs_set(mode);
	endwin();
	return 0;
}
