/*
 * Copyright (c) 2008 Bob Beck <beck@obtuse.com>
 * Copyright (c) 2013 Wyatt Johnson <wyatt@ualberta.ca>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include	<stdio.h>
#include	<curses.h>
#include	<pthread.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>

/* game defaults configuration */
#define SCORE_PER_HIT 10
#define START_ROCKET_COUNT 10
#define ESCAPED_SAUCER_LIMIT 10
#define ROCKET_HIT_BONUS 2
#define SAUCER_DELAY_MAX 20
#define SAUCER_DELAY_MIN 10
#define BORG_TURN_DEATHS 4
#define BORG_SCORE_MODIFIER 2

/* thread configuration */
#define MAX_SAUCERS 14
#define MAX_ROCKETS 10
#define	MAX_THREADS	MAX_SAUCERS + MAX_ROCKETS

/* default timing */
#define	TUNIT 20000 /* timeunits in microseconds */
#define FIRST_WAVE_DELAY 500
#define SECOND_WAVE_DELAY 2000

struct	propset {
	int id;			/* id of the object */
	int row;		/* current row */
	int col;		/* current column */
	int delay;		/* speed in timeunits */
	int ini_delay;		/* delay from start */
	int state;		/* enabled or not flag */
	int deaths;		/* number of deaths */
	struct propset * glob;  /* containing the global arrays */
};

struct checkset {
	struct propset * saucers;
	struct propset * rockets;
};

pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER; /* Display MUT */
pthread_mutex_t rl = PTHREAD_MUTEX_INITIALIZER; /* Rocket MUT */
pthread_mutex_t sl = PTHREAD_MUTEX_INITIALIZER; /* Saucer MUT */

pthread_mutex_t score_lock = PTHREAD_MUTEX_INITIALIZER; /* Score MX */
pthread_mutex_t rcount_lock = PTHREAD_MUTEX_INITIALIZER; /* Rocket Count MX */
pthread_mutex_t escaped_lock = PTHREAD_MUTEX_INITIALIZER; /* Saucer Count MX */
pthread_mutex_t gamestate_l = PTHREAD_MUTEX_INITIALIZER; /* Gamestate MX */

int escaped_count; /* number of escaped saucers */
int rocket_count = START_ROCKET_COUNT; /* count of rockets */
int score; /* player score */
int gamestate; /* state of the game */

pthread_t thrds[MAX_THREADS]; /* the threads */
struct propset saucers[MAX_SAUCERS]; /* properties of string */
struct propset rockets[MAX_ROCKETS]; /* properties of the rocket */
struct propset launcher; /* properties of the launcher */


/* Function prototypes */
void setup_saucer(int, struct propset *, int, int, int);
int print_more_than_nine(int);
void print_score();
void game_over(int);
void setup(struct propset *, struct propset *, struct propset *);
void animate_launcher(int, struct propset *);
void *animate_saucer(void *);
void fire_rocket(struct propset *, struct propset *);
int check_collision(struct propset *, int, int);
void *animate_rocket(void *);

