#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>

// The length of tail the snake will start with
#define STARTING_LENGTH 3
#define STARTING_SPEED 4
#define WINDOW_HEIGHT 20
#define WINDOW_WIDTH 20
#define HIGHSCORE_FILE "./ncsnake_highscore"
#define TITLE_FILE "./title.ascic"
#define TITLE_HEIGHT 6
#define TITLE_WIDTH 26

struct segment {
	int x;
	int y;
} typedef segment;

struct head {
	int x;
	int y;
	segment movementvector;
} typedef head;

struct snake {
	head h;
	segment* t;
	int tc;
} typedef snake;

struct apple {
	int x;
	int y;
} typedef apple;

bool isrunning;
int lastkey;
int speed;
int starty;
int startx;
snake s;
apple a;

// Returns num if min <= x < max,
// else min if x < min, or max - 1 if x >= max
int constrain(int num, int min, int max) {
	return (!(num < min || num >= max)) ? num : (num < min) ? min : max - 1;
}

void drawsquare(int y, int x, int colorpair) {
	mvaddch(constrain(y, -starty, WINDOW_HEIGHT + starty) + starty,
			constrain(x, -startx, WINDOW_WIDTH + startx) * 2 + startx * 2,
			' ' | COLOR_PAIR(colorpair));
	addch(' ' | COLOR_PAIR(colorpair));
}

void drawascii(char* image, int y, int x) {
	int j = 0;
	mvaddch(y, x, image[0]);
	for (int i = 1; image[i] != '\0'; i++) {
		if (image[i] == '\n') {
			i++;
			if (image[i] == '\0') {
				break;
			}
			j++;
			mvaddch(y + j, x, image[i]);
		} else {
			addch(image[i]);
		}
	}
}

void movesnake() {
	for (int i = s.tc; i > 0; i--) {
		s.t[i].x = s.t[i - 1].x;
		s.t[i].y = s.t[i - 1].y;
	}
	s.t[0].x = s.h.x;
	s.t[0].y = s.h.y;
	s.h.x += s.h.movementvector.x;
	s.h.y += s.h.movementvector.y;
	if (s.h.y >= WINDOW_HEIGHT) {
		s.h.y = 0;
	} else if (s.h.y < 0) {
		s.h.y = WINDOW_HEIGHT - 1;
	}
	if (s.h.x >= WINDOW_WIDTH) {
		s.h.x = 0;
	} else if (s.h.x < 0) {
		s.h.x = WINDOW_WIDTH - 1;
	}
}

void rendersnake() {
	drawsquare(s.t[s.tc].y, s.t[s.tc].x, 0);
	drawsquare(s.h.y, s.h.x, 1);
}

// Checks if the snake
// collided into itself (returns 1), the
// apple (returns 2), or nothing (returns 0).
int checkforcollision() {
	if (a.x == s.h.x && a.y == s.h.y) {
		return 2;
	}
	for (int i = 0; i < s.tc; i++) {
		if (s.t[i].x == s.h.x && s.t[i].y == s.h.y) {
			return 1;
		}
	}
	return 0;
}

// Extends the snake's tail by one segment
void addtailsegment() {
	s.tc++;
	s.t = realloc(s.t, sizeof(segment) * s.tc + 1);
}

void moveapple() {
	int x, y;
	x = rand() % WINDOW_WIDTH;
	y = rand() % WINDOW_HEIGHT;
	for (int i = 0; i < s.tc; i++) {
		if (s.t[i].x == x || s.t[i].y == y) {
			i = 0;
			x = rand() % WINDOW_WIDTH;
			y = rand() % WINDOW_HEIGHT;
		}
	}
	a.x = x;
	a.y = y;
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

int main(int argc, char* argv[]) {
	srand(time(NULL));
	isrunning = 1;
	speed = STARTING_SPEED;
	// Init window
	WINDOW* w = initscr();
	noecho();
	int mode = curs_set(0);
	cbreak();
	keypad(w, true);
	start_color();
	init_pair(1, COLOR_GREEN, COLOR_GREEN);
	init_pair(2, COLOR_RED, COLOR_RED);
	init_pair(3, COLOR_WHITE, COLOR_WHITE);
	int maxy = getmaxy(w);
	if (maxy >= WINDOW_HEIGHT + TITLE_HEIGHT + 3) {
		starty = (maxy - WINDOW_HEIGHT + TITLE_HEIGHT + 1) / 2;
	} else {
		starty = (maxy - WINDOW_HEIGHT) / 2;
	}
	startx = (getmaxx(w) / 2 - WINDOW_WIDTH) / 2;
	if (getmaxy(w) < WINDOW_HEIGHT || getmaxx(w) / 2 < WINDOW_WIDTH) {
		curs_set(mode);
		endwin();
		printf("Terminal size is too small.\nResize to at least %d rows by %d columns.\n", WINDOW_HEIGHT, WINDOW_WIDTH);
		return 0;
	}
	// Init snake and apple
	s.h.x = WINDOW_WIDTH / 2;
	s.h.y = WINDOW_HEIGHT / 2;
	s.h.movementvector.x = 1;
	s.h.movementvector.y = 0;
	s.tc = 0;
	s.t = malloc(sizeof(segment) * STARTING_LENGTH + 1);
	for (int i = 0; i < STARTING_LENGTH + 1; i++) {
		s.t[i].x = s.h.x - 1 - i;
		s.t[i].y = s.h.y;
		s.tc++;
	}
	s.tc--;
	moveapple();
	for (int i = 0; i < s.tc; i++) {
		drawsquare(s.t[i].y, s.t[i].x, 1);
	}
	// Draw border
	int i;
	for (i = -1; i < WINDOW_WIDTH + 1; i++) {
		drawsquare(-1, i, 3);
		drawsquare(WINDOW_HEIGHT, i, 3);
	}
	for (i = 0; i < WINDOW_HEIGHT; i++) {
		drawsquare(i, -1, 3);
		drawsquare(i, WINDOW_WIDTH, 3);
	}
	// Draw title
	char* title = 0;
	long length;
	FILE* titlefile = fopen(TITLE_FILE, "rb");
	if (titlefile) {
		fseek(titlefile, 0, SEEK_END);
		length = ftell(titlefile);
		fseek(titlefile, 0, SEEK_SET);
		title = malloc(length);
		if (title) {
			fread(title, 1, length, titlefile);
		}
		fclose(titlefile);
	}
	if (title && maxy >= WINDOW_HEIGHT + TITLE_HEIGHT + 3) {
		drawascii(title, starty - TITLE_HEIGHT - 2,
				  startx * 2 + ((WINDOW_WIDTH * 2 - TITLE_WIDTH) / 2));
	}
	// Start key handler
	pthread_t key;
	if (pthread_create(&key, NULL, &handlekeys, NULL) != 0) {
		return 1;
	}
	mvprintw(0, 0, "Score: %d", s.tc - STARTING_LENGTH);
	while (isrunning) {
		int collision = checkforcollision();
		if (collision == 1) {
			break;
		} else if (collision == 2) {
			addtailsegment();
			moveapple();
			speed++;
			mvprintw(0, 0, "Score: %d", s.tc - STARTING_LENGTH);
		}
		rendersnake();
		refresh();
		// Delay for 3000 / (speed + 3) millis
		clock_t start = clock();
		int delay = 3000 / (speed + 3);
		while (((double) (clock() - start) / CLOCKS_PER_SEC) * 1000.0 < delay && isrunning);
		switch (lastkey) {
			case 107: // K
			case KEY_UP:
				if (s.h.movementvector.y != 1) {
					s.h.movementvector.y = -1;
					s.h.movementvector.x = 0;
				}
				break;
			case 106: // J
			case KEY_DOWN:
				if (s.h.movementvector.y != -1) {
					s.h.movementvector.y = 1;
					s.h.movementvector.x = 0;
				}
				break;
			case 104: // H
			case KEY_LEFT:
				if (s.h.movementvector.x != 1) {
					s.h.movementvector.x = -1;
					s.h.movementvector.y = 0;
				}
				break;
			case 108: // L
			case KEY_RIGHT:
				if (s.h.movementvector.x != -1) {
					s.h.movementvector.x = 1;
					s.h.movementvector.y = 0;
				}
				break;
			case 27: // Escape key
				isrunning = 0;
				break;
			/*case 32: // Space key*/
				/*addtailsegment();*/
				/*moveapple();*/
				/*speed++;*/
				/*mvprintw(0, 0, "Score: %d", s.tc - STARTING_LENGTH);*/
			case 0: // Nothing
				break;
			/*default:*/
				/*mvprintw(starty + WINDOW_HEIGHT, 0, "%d", lastkey);*/
				/*break;*/
		}
		movesnake();
		lastkey = 0;
	}
	if (pthread_cancel(key)) {
		return 2;
	}
	curs_set(mode);
	endwin();
	FILE* file = fopen(HIGHSCORE_FILE, "r");
	if (file == NULL) {
		printf("Error opening highscore file\n");
	}
	int hs = getw(file);
	int score = s.tc - STARTING_LENGTH;
	if (score > hs) {
		fclose(file);
		file = fopen(HIGHSCORE_FILE, "w");
		if (file == NULL) {
			printf("Error opening high score file\n");
		}
		putw(score, file);
		printf("High score! The score is %d\n", score);
	} else {
		printf("Score: %d | High score is %d\n", score, hs);
	}
	fclose(file);
	return 0;
}
