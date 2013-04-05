/*
 * tanimate.c: animate several strings using threads, curses, usleep()
 *
 *	bigidea one thread for each animated string
 *		one thread for keyboard control
 *		shared variables for communication
 *	compile	cc tanimate.c -lcurses -lpthread -o tanimate
 *	to do   needs locks for shared variables
 *	        nice to put screen handling in its own thread
 */

#include "tanimate.h"

int main(int argc, char *argv[])
{
	int c;		/* user input		*/
	pthread_t thrds[MAX_THREADS];	/* the threads		*/
	struct propset saucers[MAX_SAUCERS];	/* properties of string	*/
	struct propset rockets[MAX_ROCKETS]; /* properties of the rocket */
	struct propset launcher;

	rocket_count = 10;

	/*struct checkset checker;*/

	int i, r;

	for (i = 0; i < MAX_ROCKETS; i++) {
		rockets[i].glob = saucers;
	}

	setup(saucers, rockets, &launcher);

	/* create all the threads */
	for (i = 0; i < MAX_SAUCERS; i++)
		if (pthread_create(&thrds[i], NULL, animate_saucer, &saucers[i])){
			fprintf(stderr,"error creating thread/saucer");
			endwin();
			exit(1);
		}

	for (r = 0; r < MAX_ROCKETS; r++ && i++)
		if (pthread_create(&thrds[i], NULL, animate_rocket, &rockets[r])){
			fprintf(stderr, "error creating thread/rocket");
			endwin();
			exit(1);
		}

	/* process user input */
	while(1) {
		c = getch();
		if ( c == 'q' ) break;
		if ( c == 'a' || c == 'd' )
			animate_launcher(c, &launcher);
		if ( c == ' ' )
			fire_rocket(rockets, &launcher);
	}

	/* cancel all the threads */
	pthread_mutex_lock(&mx);
	for (i=0; i<MAX_THREADS; i++ )
		pthread_cancel(thrds[i]);
	endwin();
	return 0;
}

void setup_saucer(int id, struct propset *saucer)
{
	saucer->id = id;
	saucer->row = id;		/* the row	*/
	saucer->col = 0;
	saucer->delay = 1 +(rand()%15);	/* a speed	*/
	saucer->dir = ((rand()%2)?1:-1);	/* +1 or -1	*/
	saucer->state = 1;
}

int print_more_than_ten(int value)
{
	if (value > 9)
		print_more_than_ten(value/10);
	
	value = value % 10;

	addch(value + 48);
	return value;
}

void print_score()
{
	pthread_mutex_lock(&mx);	/* only one thread	*/

	move(LINES-1, 29);

	addstr("== SCORE == rockets: ");

	pthread_mutex_lock(&rcount_lock);
	print_more_than_ten(rocket_count);
	pthread_mutex_unlock(&rcount_lock);

	addstr(" score: ");
	
	pthread_mutex_lock(&score_lock);
	print_more_than_ten(score);
	pthread_mutex_unlock(&score_lock);

	addstr(" escaped: ");

	pthread_mutex_lock(&escaped_lock);
	print_more_than_ten(escaped_count);
	pthread_mutex_unlock(&escaped_lock);

	addstr("  ");

	move(LINES-1,COLS-1);	/* park cursor		*/
	refresh();			/* and show it		*/

	pthread_mutex_unlock(&mx);	/* done with curses	*/
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

		if (reason)
			addstr( "= 10 escaped!! =");
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

void setup(struct propset saucers[], struct propset rockets[], struct propset * launcher)
{
	/*int num_msg = ( nstrings > MAX_THREADS ? MAX_THREADS : nstrings );*/
	int i;

	/* assign rows and velocities to each string */
	srand(getpid());
	for(i=0 ; i<MAX_SAUCERS; i++)
		setup_saucer(i, &saucers[i]);

	for (i = 0; i<MAX_ROCKETS; i++){
		rockets[i].id = i;
		rockets[i].row = 0;		/* the row	*/
		rockets[i].col = 0;		/* the col	*/
		rockets[i].delay = 1;	/* a speed	*/
		rockets[i].state = 0;
	}

	/* set up curses */
	initscr();
	crmode();
	noecho();
	clear();

	mvprintw(LINES-1,0,"'q' to quit, 'SPACE' to fire");

	print_score();

	launcher->col = COLS/2;

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

	pthread_mutex_lock(&mx);	/* only one thread	*/
	move(LINES-2, launcher->col);	/* can call curses	*/
	addstr("  ");
	move(LINES-2, launcher->col + mod );	/* can call curses	*/
	addstr(" |");			/* at a the same time	*/
	move(LINES-1, COLS-1);	/* park cursor		*/
	refresh();			/* and show it		*/
	pthread_mutex_unlock(&mx);	/* done with curses	*/
		launcher->col += mod;
}

/* the code that runs in each thread */
void *animate_saucer(void *arg)
{
	struct propset *info = arg;		/* point to info block	*/
	int	len = 5+2;	/* +2 for padding	*/
	int	col;	/* space for padding	*/
	int row;
	int c_v = 0;
	int id;

	pthread_mutex_lock(&sl);
		id = info->id;
		col = info->col;
	pthread_mutex_unlock(&sl);

	usleep((info->delay*200 % 10)*TUNIT);

	while( 1 )
	{	
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
			pthread_mutex_lock(&mx);	/* only one thread	*/
			move( row, col );	/* can call curses	*/
			addstr(" <---> ");
			move(LINES-1,COLS-1);	/* park cursor		*/
			refresh();			/* and show it		*/
			pthread_mutex_unlock(&mx);	/* done with curses	*/
			pthread_mutex_unlock(&sl);


			/* move item to next column and check for bouncing	*/

			pthread_mutex_lock(&sl);
			info->col += info->dir;

			if ( col <= 0 && info->dir == -1 )
				info->dir = 1;
			else if (  col+len >= COLS && info->dir == 1 ) {
				info->state = 0;

				pthread_mutex_lock(&escaped_lock);
				escaped_count += 1;
				pthread_mutex_unlock(&escaped_lock);
				print_score();
			}

			pthread_mutex_unlock(&sl);
		}
		else {
			pthread_mutex_lock(&mx);	/* only one thread	*/
			move( row, col );	/* can call curses	*/
			addch(' ');			/* at a the same time	*/
			addstr( "     " );		/* Since I doubt it is	*/
			addch(' ');			/* reentrant		*/
			move(LINES-1,COLS-1);	/* park cursor		*/
			refresh();			/* and show it		*/
			pthread_mutex_unlock(&mx);	/* done with curses	*/

			pthread_mutex_lock(&sl);
			setup_saucer(id, info);
			pthread_mutex_unlock(&sl);
		}

		pthread_mutex_lock(&escaped_lock);
		if (escaped_count >= 10)
			game_over(1);
		pthread_mutex_unlock(&escaped_lock);
	}
}

void fire_rocket(struct propset rockets[], struct propset * launcher) {
	int ret = 0;
	int i;

	int enough = 0;

	pthread_mutex_lock(&rcount_lock);
	if (rocket_count > 0) {
		enough = 1;
		rocket_count -= 1;
	}
	else {
		enough = -1;
	}
	pthread_mutex_unlock(&rcount_lock);

	print_score();

	if (enough < 0) {
		game_over(0);
	}

	pthread_mutex_lock(&gamestate_l);
	if (gamestate == -1)
		enough = 0;
	pthread_mutex_unlock(&gamestate_l);

	if (enough)
		for (i = 0; i<MAX_ROCKETS; i++){
			pthread_mutex_lock(&rl);
				if (rockets[i].state == 0) {
					rockets[i].state = 1;
					rockets[i].col = launcher->col + 1;
					ret = 1;
				}
			pthread_mutex_unlock(&rl);

			if (ret)
				return;
		}
	
}

int check_collision(struct propset saucers[], int row, int col)
{
	int i;
	int ret = 0;

	pthread_mutex_lock(&sl);
	for (i = 0; i<MAX_SAUCERS; i++) {
		
		if (saucers[i].row == row) {
			if (col >= saucers[i].col && col <= saucers[i].col + 4) {
				saucers[i].state = 0;

				ret = 1;
			}
		}

		if(ret)
			break;
	}
	pthread_mutex_unlock(&sl);

	return ret;
				
}

/* the code that runs in each thread */
void *animate_rocket(void *arg)
{
	struct propset *info = arg;		/* point to info block	*/
	int	len = 1+2;	/* +2 for padding	*/
	int	col = 5;	/* space for padding	*/
	int c_v = 0;
	int row = LINES-3;
	int offset = 0;
	int id = info->id;
	int hit = 0;

	while( 1 )
	{	

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

			if (row != LINES-3)
				offset = 1;

			pthread_mutex_lock(&rl);
			/* Check if there is a saucer in the place of where the rocket used to be */
			if(!check_collision(info->glob, row + offset, col)) {
				pthread_mutex_lock(&mx);	/* only one thread	*/
				move( row + offset, col );	/* can call curses	*/
				addstr( " " );		/* Since I doubt it is	*/
				move(LINES-1,COLS-1);	/* park cursor		*/
				refresh();			/* and show it		*/
				pthread_mutex_unlock(&mx);	/* done with curses	*/
			}
			pthread_mutex_unlock(&rl);

			pthread_mutex_lock(&mx);	/* only one thread	*/
			move( row, col );	/* can call curses	*/
			addstr( "^" );		/* Since I doubt it is	*/
			move(LINES-1,COLS-1);	/* park cursor		*/
			refresh();			/* and show it		*/
			pthread_mutex_unlock(&mx);	/* done with curses	*/
			

			/* move item to next row */
			row -= 1;

			pthread_mutex_lock(&rl);
			hit = check_collision(info->glob, row, col);
			pthread_mutex_unlock(&rl);

			if (hit) {
				pthread_mutex_lock(&score_lock);
				score += 10;
				pthread_mutex_unlock(&score_lock);
				pthread_mutex_lock(&rcount_lock);
				rocket_count += 2;
				pthread_mutex_unlock(&rcount_lock);
				print_score();
			}

			if ( row < 0 || hit) {
					usleep(info->delay*TUNIT);

					pthread_mutex_lock(&rl);
					info->state = 0;
					pthread_mutex_unlock(&rl);

					pthread_mutex_lock(&mx);	/* only one thread	*/
					move( row + 1, col );	/* can call curses	*/
					addstr( " " );		/* Since I doubt it is	*/
					move(LINES-1,COLS-1);	/* park cursor		*/
					refresh();			/* and show it		*/
					pthread_mutex_unlock(&mx);	/* done with curses	*/
					row = LINES-3;
			}
			
		}
	}
}
