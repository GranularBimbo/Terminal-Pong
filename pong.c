#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define ROWS 65
#define COLS 60

#define START_Y 6
#define START_X 20

//Position variables
int player_x = 40, player_y = START_Y;
int cpu_x = 2, cpu_y = START_Y;
int ball_x = 4, ball_y = START_Y;

//Scoring variables
int player_score = 0, cpu_score = 0;

//AI movement and difficulty variables
int vel_x = 0, vel_y = 0, x_interval = 2, y_interval = 5;
int cpu_follow_chance = 90; //Percentage based, higher follow chance % = more CPU accuracy.

//90 seems deceptively high, but it's good for a couple back and forths. It's checked every time the ball changes height
//so a lower chance than even 90 means a fairly inaccurate CPU.

bool running = true;


//Formatting and input
struct termios original_termios;

//Uses escape codes to show or hide the terminal cursor. Otherwise you see it flashing across the screen each frame
//which just hurts your eyes.
void hide_cursor(){
	printf("\033[?25l");
}

void show_cursor(){
	printf("\033[?25h");
}


//Restore terminal settings on exit
void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    show_cursor();
}

//Allows terminal to collect raw input directly without needing to press enter to input controls
void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(restore_terminal); // Ensure restoration on exit

    struct termios raw = original_termios;
    raw.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    raw.c_cc[VMIN] = 0; // Read at least 1 byte
    raw.c_cc[VTIME] = 0; // No timeout

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}


char field[ROWS][COLS] = {
        "###########################################\n",
        "#                                         #\n",
        "#                                         #\n",
        "#                                         #\n",
        "#                                         #\n",
        "#                                         #\n",
        "#                                         #\n",
        "#                                         #\n",
        "#                                         #\n",
        "#                                         #\n",
        "#                                         #\n",
        "#                                         #\n",
        "#                                         #\n",
        "#                                         #\n",
        "###########################################\n"};


void show_map(){
	printf("          --Terminal Pong v1.0.0--\n");
	printf("First to 5 wins!\n\n");

	printf("Press ↑ to move up\n");
	printf("Press ↓ to move down\n\n");

	//Menu display between rounds
	if(vel_x == 0 || vel_y == 0){
		if(cpu_score == 5){
			field[START_Y][17] = 'Y';
			field[START_Y][18] = 'O';
			field[START_Y][19] = 'U';
			field[START_Y][20] = ' ';
			field[START_Y][21] = 'L';
			field[START_Y][22] = 'O';
			field[START_Y][23] = 'S';
			field[START_Y][24] = 'E';
			field[START_Y][25] = '!';
		}
		else if(player_score == 5){
			field[START_Y][18] = 'Y';
			field[START_Y][19] = 'O';
			field[START_Y][20] = 'U';
			field[START_Y][21] = ' ';
			field[START_Y][22] = 'W';
			field[START_Y][23] = 'I';
			field[START_Y][24] = 'N';
			field[START_Y][25] = '!';
		}
		
		if((cpu_score < 5 && player_score < 5) && (cpu_score > 0 || player_score > 0)){
			field[START_Y][16] = '[';
			field[START_Y][17] = 'N';
			field[START_Y][18] = ']';
			field[START_Y][19] = 'e';
			field[START_Y][20] = 'w';
			field[START_Y][21] = ' ';
			field[START_Y][22] = 'G';
			field[START_Y][23] = 'a';
			field[START_Y][24] = 'm';
			field[START_Y][25] = 'e';

			field[START_Y+1][16] = '[';
			field[START_Y+1][17] = 'C';
			field[START_Y+1][18] = ']';
			field[START_Y+1][19] = 'o';
			field[START_Y+1][20] = 'n';
			field[START_Y+1][21] = 't';
			field[START_Y+1][22] = 'i';
			field[START_Y+1][23] = 'n';
			field[START_Y+1][24] = 'u';
			field[START_Y+1][25] = 'e';	

			field[START_Y+2][18] = '[';
			field[START_Y+2][19] = 'Q';
			field[START_Y+2][20] = ']';
			field[START_Y+2][21] = 'u';
			field[START_Y+2][22] = 'i';
			field[START_Y+2][23] = 't';
		}
		else{
			field[START_Y][16] = '[';
			field[START_Y][17] = 'N';
			field[START_Y][18] = ']';
			field[START_Y][19] = 'e';
			field[START_Y][20] = 'w';
			field[START_Y][21] = ' ';
			field[START_Y][22] = 'G';
			field[START_Y][23] = 'a';
			field[START_Y][24] = 'm';
			field[START_Y][25] = 'e';

			field[START_Y+1][18] = '[';
			field[START_Y+1][19] = 'Q';
			field[START_Y+1][20] = ']';
			field[START_Y+1][21] = 'u';
			field[START_Y+1][22] = 'i';
			field[START_Y+1][23] = 't';
		}
	}	

	//Main gameplay interface
	printf("   CPU SCORE: %d     |    PLAYER SCORE: %d\n", cpu_score, player_score);

	for(int r = 0; r < ROWS; r++){
		for(int c = 0; c < COLS; c++){
			if((r == cpu_y && c == cpu_x) || (r == player_y && c == player_x)){
				printf("|");
			}
			else if(r == ball_y && c == ball_x){
				printf("*");
			}
			else{
				printf("%c", field[r][c]);
			}
		}
	}

	printf("\033[H"); //Escape code to return the cursor back to top left of the screen. Avoids the flickering that comes with using system("clear")
}


//Helpers and cleanup

//Generates a random number between the given min and max (inclusive)
int rand_between(int min, int max){
	return rand() % (max - min + 1) + min;
}

//Generates a random movement direction for the ball at the start of a round.
int rand_dir(){
	int vel = 1;
	int flip_vel = rand_between(0, 1);
	if(flip_vel == 1){
		vel *= -1;
	}
	return vel;
}

//Clears the menu options so the field is clear during a match
void reset_message(){
	field[START_Y][16] = ' ';
	field[START_Y][17] = ' ';
	field[START_Y][18] = ' ';
	field[START_Y][19] = ' ';
	field[START_Y][20] = ' ';
	field[START_Y][21] = ' ';
	field[START_Y][22] = ' ';
	field[START_Y][23] = ' ';
	field[START_Y][24] = ' ';
	field[START_Y][25] = ' ';

	field[START_Y+1][16] = ' ';
	field[START_Y+1][17] = ' ';
	field[START_Y+1][18] = ' ';
	field[START_Y+1][19] = ' ';
	field[START_Y+1][20] = ' ';
	field[START_Y+1][21] = ' ';
	field[START_Y+1][22] = ' ';
	field[START_Y+1][23] = ' ';
	field[START_Y+1][24] = ' ';
	field[START_Y+1][25] = ' ';

	field[START_Y+2][18] = ' ';
	field[START_Y+2][19] = ' ';
	field[START_Y+2][20] = ' ';
	field[START_Y+2][21] = ' ';
	field[START_Y+2][22] = ' ';
	field[START_Y+2][23] = ' ';

}


//Game loop and Logic

//Resets the paddles and ball and sends it moving in a random direction
void new_game(){
	reset_message();	

	ball_x = START_X;
	ball_y = START_Y;

	vel_x = rand_dir();
	vel_y = rand_dir();

	cpu_y = START_Y;
	player_y = START_Y;

	if(vel_x > 0){
		ball_x -= 2;
	}
	else{
		ball_x += 2;
	}	
}


int main(int argc, char *argv){
	//Formatting and setup
	srand(time(NULL)); //Sets the random seed based on the current CPU time
	enable_raw_mode();
	system("clear");
	hide_cursor();

	//Logic and controls
	int frame_count = 0;
	while(running){
		struct timespec ts = {0, 18 * 1000000}; //18ms
		nanosleep(&ts, NULL); //Sleep for 18ms to make the speed more manageable

		frame_count++;

		char c; //This contains the key press data
		
		//Reads key press data into c
		read(STDIN_FILENO, &c, 1);
		if(c == 0x41 && player_y > 1){ //Up arrow movement and bound checking
			player_y -= 1;
		}
		else if(c == 0x42 && player_y < 13){ //Down arrow movement and bound checking
			player_y += 1;
		}
		else if(c == 'q'){ //Quitting the game
			running = false;
			break;
		}
		else if(c == 'n'){ //Starting a new game
			new_game();
			cpu_score = 0;
			player_score = 0;
		}
		else if(c == 'c' && (cpu_score < 5 && player_score < 5) && (cpu_score > 0 || player_score > 0)){ //Continue to next match
			new_game();
		}

		c = '\0'; //Clears input so it doesn't spam once you let go of the key
		show_map();
	

		//Ball movement and collision
		if(frame_count % y_interval == 0){ //y axis movement
			if(ball_y + vel_y > 0 && ball_y + vel_y < 13){ //Bound checking and y axis movement
				ball_y += vel_y;
			}
			else{ //y axis bounce logic
				vel_y *= -1;
				ball_y += vel_y;
			}

			//Makes the CPU paddle follow the ball as it changes y value based on its follow chance
			int cpu_follow = rand_between(1, 100);
			if(cpu_follow <= cpu_follow_chance && cpu_y + vel_y > 0 && cpu_y + vel_y < 13){
				cpu_y += vel_y;
			}
		}

		if (frame_count % x_interval == 0) { //x axis movement
    		int next_x = ball_x + vel_x;

			//paddle collision, next_y not needed since y was already updated
			if((next_x == player_x && ball_y == player_y) || (next_x == cpu_x && ball_y == cpu_y)){
				vel_x *= -1;
			}

			if(next_x <= 0 && vel_x < 0){
				player_score++;
			}	
			else if(next_x >= 42 && vel_x > 0){
				cpu_score++;
			}

			if(next_x <= 0 || next_x >= 42){ //Stops the ball and opens menu on score
				vel_x = 0;
				vel_y = 0;
			}

			ball_x += vel_x;
		}	
	}

	system("clear");
	restore_terminal();
	return 0;
}


