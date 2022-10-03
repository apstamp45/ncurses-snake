#include <stdio.h>
#include <string.h>
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
#define HIGHSCORE_FILE "/etc/snake.hs"
#define TITLE " ___\n/   \\            |\n|       __  ___  |     __\n \\-\\  |/  \\  __\\ | /  /__\\\n    | |   | /  | |/\\  |\n\\___/ |   | \\__| |  \\ \\__/\n"
#define TITLE_HEIGHT 6
#define TITLE_WIDTH 26
#define EXIT_KEY 113 // Q

struct segment {
	int x;
	int y;
} typedef segment;

struct snake {
	segment h;
	segment mv; // For storing the snake's direction
	segment* t;
	int tc;
} typedef snake;

bool isrunning;
int lastkey;
int speed;
int starty;
int startx;
int mode; // Stores initial cursor mode
pthread_t keyhandler;
snake s;
segment a;

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
	// Set every segment to the one at next index
	for (int i = s.tc; i > 0; i--) {
		s.t[i].x = s.t[i - 1].x;
		s.t[i].y = s.t[i - 1].y;
	}
	// The last segment gets the head's coords
	s.t[0].x = s.h.x;
	s.t[0].y = s.h.y;
	// Shift the head's position
	s.h.x += s.mv.x;
	s.h.y += s.mv.y;
	// Warp the head if out of bounds
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
	s.t = realloc(s.t, sizeof(segment) * (s.tc + 1));
	// Make sure the added segment doesn't affect rendering
	s.t[s.tc].x = s.h.x;
	s.t[s.tc].y = s.h.y;
}

void moveapple() {
	int x, y;
	x = rand() % WINDOW_WIDTH;
	y = rand() % WINDOW_HEIGHT;
	// Check that the apple doesn't collide with snake
	for (int i = 0; i < s.tc; i++) {
		if ((s.t[i].x == x && s.t[i].y == y) ||
				(s.h.x == x && s.h.y == y)) {
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
		if (lastkey == EXIT_KEY) {
			break;
		}
	}
	pthread_exit(NULL);
}

void init() {
	srand(time(NULL));
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
	// Init snake window
	int maxy = getmaxy(w);
	if (maxy >= WINDOW_HEIGHT + TITLE_HEIGHT + 3) {
		starty = (maxy - WINDOW_HEIGHT + TITLE_HEIGHT + 1) / 2;
	} else {
		starty = (maxy - WINDOW_HEIGHT) / 2;
	}
	startx = (getmaxx(w) / 2 - WINDOW_WIDTH) / 2;
	if (getmaxy(w) < WINDOW_HEIGHT + 2 || getmaxx(w) / 2 < WINDOW_WIDTH + 2) {
		curs_set(mode);
		endwin();
		printf("Terminal size is too small.\nResize to at least %d rows by %d columns.\n", WINDOW_HEIGHT + 2, WINDOW_WIDTH + 2);
		exit(0);
	}
	// Init snake and apple
	s.h.x = WINDOW_WIDTH / 2;
	s.h.y = WINDOW_HEIGHT / 2;
	s.mv.x = 1;
	s.mv.y = 0;
	s.tc = 0;
	s.t = malloc(sizeof(segment) * (STARTING_LENGTH + 1));
	for (int i = 0; i < STARTING_LENGTH + 1; i++) {
		s.t[i].x = s.h.x - 1 - i;
		s.t[i].y = s.h.y;
		s.tc++;
	}
	s.tc--;
	moveapple();
	refresh();
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
	if (maxy >= WINDOW_HEIGHT + TITLE_HEIGHT + 3) {
		drawascii(TITLE, starty - TITLE_HEIGHT - 2,
				  startx * 2 + ((WINDOW_WIDTH * 2 - TITLE_WIDTH) / 2));
	}
	// Start key handler
	if (pthread_create(&keyhandler, NULL, &handlekeys, NULL) != 0) {
		curs_set(mode);
		endwin();
		printf("Error creating key handler\n");
		exit(1);
	}
	refresh();
	mvprintw(0, 0, "Score: %d", s.tc - STARTING_LENGTH);
}

void loop() {
	isrunning = 1;
	while (isrunning) {
		int collision = checkforcollision();
		if (collision == 1) {// If the snake crashes into itself
			break;
		} else if (collision == 2) { // If the snake crashes into the apple
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
				if (s.mv.y != 1) {
					s.mv.y = -1;
					s.mv.x = 0;
				}
				break;
			case 106: // J
			case KEY_DOWN:
				if (s.mv.y != -1) {
					s.mv.y = 1;
					s.mv.x = 0;
				}
				break;
			case 104: // H
			case KEY_LEFT:
				if (s.mv.x != 1) {
					s.mv.x = -1;
					s.mv.y = 0;
				}
				break;
			case 108: // L
			case KEY_RIGHT:
				if (s.mv.x != -1) {
					s.mv.x = 1;
					s.mv.y = 0;
				}
				break;
			case EXIT_KEY:
				isrunning = 0;
				break;
			case 0: // Nothing
				break;
		}
		movesnake();
		lastkey = 0;
	}
}

void end() {
	curs_set(mode);
	endwin();
	if (pthread_cancel(keyhandler)) {
		printf("WARNING: Error closing key handler thread\n");
	}
	// Check high score, and update if needed
	int score = s.tc - STARTING_LENGTH;
	FILE* file = fopen(HIGHSCORE_FILE, "r");
	if (file == NULL) {
		printf("Error opening highscore file (should be located at %s)\nYour score was %d\n", HIGHSCORE_FILE, score);
		exit(1);
	}
	int hs = getw(file);
	if (score > hs) {// Write the new score to the file if is new high score
		fclose(file);
		file = fopen(HIGHSCORE_FILE, "w");
		if (file == NULL) {
			printf("Error opening high score file for writing (error with file permissions?).\n");
			exit(1);
		}
		putw(score, file);
		printf("%d! New high score!\n", score);
	} else {
		printf("Score: %d | The high score is %d\n", score, hs);
	}
	fclose(file);
	free(s.t);
}

int main(int argc, char* argv[]) {
	init();
	loop();
	end();
	return 0;
}
