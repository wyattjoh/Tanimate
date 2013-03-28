#include	<stdio.h>
#include	<curses.h>
#include	<pthread.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>

#define MAX_SAUCERS 8
#define MAX_ROCKETS 10
#define	MAX_THREADS	MAX_SAUCERS + MAX_ROCKETS
#define	TUNIT   20000		/* timeunits in microseconds */

struct	propset {
		int id;
		int	row;	/* the row     */
		int col;    /* the col     */
		int	delay;  /* delay in time units */
		int	dir;	/* +1 or -1	*/
		int state;
		struct propset * glob;
};

struct checkset {
	struct propset * saucers;
	struct propset * rockets;
};

pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER; /* Display MUT */
pthread_mutex_t rl = PTHREAD_MUTEX_INITIALIZER; /* Rocket MUT */
pthread_mutex_t sl = PTHREAD_MUTEX_INITIALIZER; /* Saucer MUT */

pthread_mutex_t score_lock = PTHREAD_MUTEX_INITIALIZER; /* Score MUT */
pthread_mutex_t rcount_lock = PTHREAD_MUTEX_INITIALIZER; /* Rocket Count Mutex */
pthread_mutex_t escaped_lock = PTHREAD_MUTEX_INITIALIZER; /* Escaped Saucer Count Mutex */
pthread_mutex_t gamestate_l = PTHREAD_MUTEX_INITIALIZER; /* Gamestate Mutex */

int escaped_count;
int rocket_count;
int score;
int gamestate;

void animate_launcher(int, struct propset *);
void *manage_threads();
void *animate_saucer();
void *animate_rocket();
void setup(struct propset *, struct propset *, struct propset *);
void fire_rocket(struct propset *, struct propset *);