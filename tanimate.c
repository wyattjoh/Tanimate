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

#include "tanimate.h"

int main(int argc, char *argv[])
{
	int i, r; /* For loop variables */
	int c; /* user input */
	pthread_t thrds[MAX_THREADS]; /* the threads */
	struct propset saucers[MAX_SAUCERS]; /* properties of string */
	struct propset rockets[MAX_ROCKETS]; /* properties of the rocket */
	struct propset launcher;

	/* giva all rockets the saucer array to play with */
	for (i = 0; i < MAX_ROCKETS; i++)
		rockets[i].glob = saucers;

	/* Run a setup on all the structures. */
	setup(saucers, rockets, &launcher);

	/* create all the saucers */
	for (i = 0; i < MAX_SAUCERS; i++)
		if (pthread_create(&thrds[i], NULL, animate_saucer,
		    &saucers[i])){
			fprintf(stderr,"error creating thread/saucer");
			endwin();
			exit(1);
		}

	/* create all the rockets */
	for (r = 0; r < MAX_ROCKETS; r++ && i++)
		if (pthread_create(&thrds[i], NULL, animate_rocket,
		    &rockets[r])){
			fprintf(stderr, "error creating thread/rocket");
			endwin();
			exit(1);
		}

	/* process user input */
	while (1) {
		c = getch();
		if (c == 'q')
			break;
		if (c == 'a' || c == 'd')
			animate_launcher(c, &launcher);
		if (c == ' ')
			fire_rocket(rockets, &launcher);
	}

	/* exit and clean the screen */
	pthread_mutex_lock(&mx);
	endwin();
	return 0;
}

void setup_saucer(int id, struct propset *saucer, int multiplier, int init,
    int speed_mod)
{
	if (init) {
		saucer->row = id; /* the row */
		saucer->deaths = 0; /* number of deaths */

		/* this is apart of the second wave */
		if (multiplier == SECOND_WAVE_DELAY) {
			saucer->id = id + MAX_SAUCERS/2;
		}
		else
			saucer->id = id;
	}
	
	saucer->col = 0;

	/* Based on the mod given as the amount of deaths, delay or speed up */
	if (SAUCER_DELAY_MAX - speed_mod < SAUCER_DELAY_MIN)
		speed_mod = SAUCER_DELAY_MIN;
	else
		speed_mod = SAUCER_DELAY_MAX - speed_mod;

	saucer->delay = 1 + rand() % speed_mod; /* a speed */
	saucer->state = 1;
	saucer->ini_delay = 1 + rand() % multiplier;	
}

/* Prints to curses a value with more than one digit*/
int  print_more_than_nine(int value)
{
	if (value > 9)
		 print_more_than_nine(value/10);
	
	value = value % 10;

	addch(value + 48);
	return value;
}

void print_score()
{
	pthread_mutex_lock(&mx); /* only one thread */

	/* Print out the score after the controls have been printed */
	move(LINES-1, 29);

	addstr("== SCORE == rockets: ");

	/* print out the rocket count */
	pthread_mutex_lock(&rcount_lock);
	print_more_than_nine(rocket_count);
	pthread_mutex_unlock(&rcount_lock);

	addstr(" score: ");
	

	/* print out the score */
	pthread_mutex_lock(&score_lock);
	print_more_than_nine(score);
	pthread_mutex_unlock(&score_lock);

	addstr(" escaped: ");

	/* print out the escaped count */
	pthread_mutex_lock(&escaped_lock);
	print_more_than_nine(escaped_count);
	pthread_mutex_unlock(&escaped_lock);

	addstr("  ");

	move(LINES-1,COLS-1); /* park cursor */
	refresh(); /* and show it */

	pthread_mutex_unlock(&mx); /* done with curses */
}

void game_over(int reason)
{
	pthread_mutex_lock(&gamestate_l);
	if (gamestate != -1) {
		gamestate = -1;
		/* Lock all mutex (Freeze gameplay) */
		pthread_mutex_lock(&rl);
		pthread_mutex_lock(&sl);
		pthread_mutex_lock(&mx);
		move(LINES/2 - 3, COLS/2 - 8);
		addstr( "================" );
		move(LINES/2 - 2, COLS/2 - 8);
		addstr( "= GAME OVER :P =" );
		move(LINES/2 - 1, COLS/2 - 8);
		addstr( "================" );

		move(LINES/2, COLS/2 - 8);

		if (reason) {
			addstr( "= ");
			if (ESCAPED_SAUCER_LIMIT < 10)
				addstr(" ");
			print_more_than_nine(ESCAPED_SAUCER_LIMIT);
			addstr(" escaped!! =");
		}
		else
			addstr( "= No Rockets!! =");

		move(LINES/2 + 1, COLS/2 - 8);
		addstr( "----------------" );

		move(LINES/2 + 2, COLS/2 - 8);
		addstr( "== 'q' 2 quit ==" );
		move(LINES/2 + 3, COLS/2 - 8);
		addstr( "----------------" );
		move(LINES-1,COLS-1);
		refresh();
		pthread_mutex_unlock(&mx);
	}
	pthread_mutex_unlock(&gamestate_l);

}

void setup(struct propset saucers[], struct propset rockets[],
    struct propset * launcher)
{
	int i;

	srand(getpid());
	for (i=0 ; i<MAX_SAUCERS/2; i++)
		setup_saucer(i, &saucers[i], FIRST_WAVE_DELAY, 1, 0);

	for (i=0 ; i<MAX_SAUCERS/2; i++)
		setup_saucer(i, &saucers[i + MAX_SAUCERS/2], SECOND_WAVE_DELAY,
		    1, 0);

	for (i = 0; i<MAX_ROCKETS; i++){
		rockets[i].id = i;
		rockets[i].row = 0;
		rockets[i].col = 0;
		rockets[i].delay = 1;
		rockets[i].state = 0;
	}

	/* set up curses */
	initscr();
	crmode();
	noecho();
	clear();

	/* Print some initial menu stuff */
	mvprintw(LINES-1,0,"'q' to quit, 'SPACE' to fire");

	/* Print the score */
	print_score();

	/* Establish the middle of the screen */
	launcher->col = COLS/2;

	/* Place the launcher at its position */
	animate_launcher(0, launcher);
}

void animate_launcher(int dir, struct propset * launcher)
{
	int mod;

	if (dir == 'a')
		mod = -1;
	else
		mod = 1;

	if (launcher->col + mod >= COLS-2 || launcher->col + mod < 0)
		return;

	pthread_mutex_lock(&mx);
	move(LINES-2, launcher->col);
	addstr("  ");
	move(LINES-2, launcher->col + mod );
	addstr(" |");
	move(LINES-1, COLS-1);
	refresh();
	pthread_mutex_unlock(&mx);

	launcher->col += mod;
}

/* the code that runs in each thread */
void *animate_saucer(void *arg)
{
	struct propset *info = arg; /* point to info block */
	int len = 5+2; /* length of the curses object */
	int col = info->col; 
	int row;
	int c_v = 0;
	int id = info->id;

	if (id > MAX_SAUCERS/2)
		usleep(info->ini_delay*TUNIT*10);
	else
		usleep(info->ini_delay*TUNIT);

	while (1) {	
		pthread_mutex_lock(&sl);
			if (info->state) {
				c_v = 1;

				row = info->row;
				col = info->col;
			}
			else {
				c_v = 0;

				info->col = rand()%(COLS-len-3);
				info->state = 1;
			}
		pthread_mutex_unlock(&sl);

		if (c_v) {
			usleep(info->delay*TUNIT);

			pthread_mutex_lock(&sl);
			if(info->state == 0)
			{
				pthread_mutex_unlock(&sl);
				continue;
			}
			pthread_mutex_lock(&mx); /* only one thread */
			move( row, col ); /* can call curses */

			/* if they have reached borg levels...*/
			if (info->deaths > BORG_TURN_DEATHS)
				addstr(" [---]");
			else
				addstr(" <--->");

			move(LINES-1,COLS-1); /* park cursor */
			refresh(); /* and show it */
			pthread_mutex_unlock(&mx); /* done with curses */
			pthread_mutex_unlock(&sl);

			pthread_mutex_lock(&sl);
			info->col += 1;

			if (col+len >= COLS) {
				/* saucer escaped! */
				info->state = 0;

				pthread_mutex_lock(&escaped_lock);
				escaped_count += 1;
				info->deaths++;
				pthread_mutex_unlock(&escaped_lock);
				print_score();
			}

			pthread_mutex_unlock(&sl);
		}
		else {
			/* saucer was hit :( */

			pthread_mutex_lock(&mx); /* only one thread */
			move(row, col); /* can call curses */
			addstr( "      " ); /* Since I doubt it is */
			move(LINES-1,COLS-1);	/* park cursor */
			refresh(); /* and show it */
			pthread_mutex_unlock(&mx); /* done with curses */

			pthread_mutex_lock(&sl);
			info->deaths++;
			setup_saucer(id, info, 1 + rand()%20, 0, info->deaths);
			pthread_mutex_unlock(&sl);

			usleep(info->ini_delay*TUNIT);
		}

		pthread_mutex_lock(&escaped_lock);
		if (escaped_count >= ESCAPED_SAUCER_LIMIT)
			game_over(1);
		pthread_mutex_unlock(&escaped_lock);
	}
}

/* fires a rocket from the launcher */
void fire_rocket(struct propset rockets[], struct propset * launcher) {
	int ret = 0;	 /* triggers exiting a rocket launch loop */
	int i = 0;	 /* looper var */
	int enough = 0;  /* enough rockets flag */

	/* check if there is enough rockets */
	pthread_mutex_lock(&rcount_lock);
	if (rocket_count > 0) {
		enough = 1; /* enough rockets */
		rocket_count -= 1; /* decrease rocket count */
	}
	else
		enough = -1;
	pthread_mutex_unlock(&rcount_lock);

	print_score(); /* print out the score */

	/* if there is not enough rockets, then exit */
	if (enough < 0)
		game_over(0);

	/* if the gamestate is already killed, then it is not enough */
	pthread_mutex_lock(&gamestate_l);
	if (gamestate == -1)
		enough = 0;
	pthread_mutex_unlock(&gamestate_l);

	/* if there is enough rockets, fire one */
	if (enough)
		/* loop over all rockets until a deactivate one is found */
		for (i = 0; i<MAX_ROCKETS; i++){
			pthread_mutex_lock(&rl);
				if (rockets[i].state == 0) {
					/* fire the rocket at launcher pos */
					rockets[i].state = 1;
					rockets[i].col = launcher->col + 1;

					ret = 1;
				}
			pthread_mutex_unlock(&rl);

			if (ret)
				return;
		}
	
}

/* called by a rocket, checks if it exists in the same place as a saucer */
int check_collision(struct propset saucers[], int row, int col)
{
	int i;		/* looping var */
	int ret = 0;	/* return flag */

	/* loop over all saucers (yes, there is splash damage) and kill em */

	pthread_mutex_lock(&sl);

	for (i = 0; i<MAX_SAUCERS; i++)
		if (saucers[i].row == row && col >= saucers[i].col &&
		    col <= saucers[i].col + 4)
		{
			saucers[i].state = 0;

			/* Check if Borg, if so add multiplier */
			if (saucers[i].deaths > BORG_TURN_DEATHS) {
				pthread_mutex_lock(&score_lock);
				score += SCORE_PER_HIT * BORG_SCORE_MODIFIER
				    - SCORE_PER_HIT;
				pthread_mutex_unlock(&score_lock);
			}

			ret = 1;
		}
	
	pthread_mutex_unlock(&sl);

	return ret;
}

/* animates a given rocket */
void *animate_rocket(void *arg)
{
	struct propset *info = arg; /* point to info block */
	int col;		    /* the column that the rocket is startin */
	int c_v = 0;		    /* local activation state */
	int row = LINES-3;	    /* row it should start on */
	int offset = 0;		    /* offset of rocket erasing */
	int hit = 0;		    /* hit flag */

	while (1) {
		pthread_mutex_lock(&rl);
			if (info->state == 1) {
				c_v = 1;
				col = info->col;
				offset = 0;
			}
			else
				c_v = 0;
		pthread_mutex_unlock(&rl);

		if (c_v == 1) {
			usleep(info->delay*TUNIT);

			if (row != LINES - 3)
				offset = 1;

			/* check for collisions */
			pthread_mutex_lock(&rl);
			if (!check_collision(info->glob, row + offset, col)) {
				pthread_mutex_lock(&mx);
				move(row + offset, col);
				addstr( " " );
				move(LINES-1,COLS-1);
				refresh();
				pthread_mutex_unlock(&mx);
			}
			pthread_mutex_unlock(&rl);

			pthread_mutex_lock(&mx);
			move( row, col );
			addstr( "^" );
			move(LINES-1, COLS-1);
			refresh();
			pthread_mutex_unlock(&mx);

			/* move item to next row */
			row -= 1;

			pthread_mutex_lock(&rl);
			hit = check_collision(info->glob, row, col);
			pthread_mutex_unlock(&rl);

			if (hit) {
				pthread_mutex_lock(&score_lock);
				score += SCORE_PER_HIT;
				pthread_mutex_unlock(&score_lock);
				pthread_mutex_lock(&rcount_lock);
				rocket_count += ROCKET_HIT_BONUS;
				pthread_mutex_unlock(&rcount_lock);
				print_score();
			}

			if ( row < 0 || hit) {
				usleep(info->delay*TUNIT);

				pthread_mutex_lock(&rl);
				info->state = 0;
				pthread_mutex_unlock(&rl);

				pthread_mutex_lock(&mx);
				move(row + 1, col);
				addstr(" ");
				move(LINES-1,COLS-1);
				refresh();
				pthread_mutex_unlock(&mx);
				row = LINES-3;
			}
			
		}
	}
}
