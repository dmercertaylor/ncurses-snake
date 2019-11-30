#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

// May be overwritten by argv in main
int SCREEN_HEIGHT = 16;
int SCREEN_WIDTH = 32;

int BASE_DELAY = 250000;
const char ON = '0';
const char OFF = ' ';
const char * heads = "<>^v";

enum Direction{LEFT = 0, RIGHT = 1, UP = 2, DOWN = 3};

void check_alloc(void * ptr);       // exits program if ptr is null
bool is_number(char * str);         // check if a string contains only numbers
void update_screen(char ** frame);  // draw all the strings in frame to the screen
void draw_at_coords(char ** board, int * coords, char c);   // put char c into board at coords
void move_snake(int direction, int * coords);               // move coords in direction
bool check_position_legal(char ** board, int * pos);
void place_apple(char ** board, int * pos, int offset); // puts a 0 at a random, non-snake location
int game_loop(char ** game_world);  // Runs a Snake game in the game_world.

int main(int argc, char ** argv)
{
    // GET ARGUMENTS
    //
    // If there is a way to do this with just getopt, please let me know.
    if(argc > 1){
        // check if agrv[1] is valid
        if( !is_number(argv[1]) || (SCREEN_WIDTH = atoi(argv[1])) == 0){
            printf("Unknown command %s\nUsage: snake [screen width] [screen height]\n", argv[1]);
            return 1;
        }
        // if only 1 input is given, the board is a square
        SCREEN_HEIGHT = SCREEN_WIDTH;

        if(argc > 2){
            if( !is_number(argv[2]) || (SCREEN_HEIGHT = atoi(argv[2])) == 0){
                printf("Unknown command %s\nUsage: life [screen width] [screen height]\n", argv[2]);
                return 2;
            }
        }
        if(SCREEN_HEIGHT < 0){ SCREEN_HEIGHT *= -1; }
        if(SCREEN_WIDTH < 0){ SCREEN_WIDTH *= -1; }

        if(SCREEN_HEIGHT < 8){ SCREEN_HEIGHT = 8; }
        if(SCREEN_WIDTH < 8){ SCREEN_WIDTH = 8; }

        SCREEN_WIDTH *= 2;
    }

    // SET UP NCURSES

    WINDOW *window;     // pointer to ncurses window structure.

    initscr();              // init ncurses window
    noecho();               // stop inputs from appearing in the terminal
    cbreak();               // disable line buffering, so getch can read inputs without \n being pressed
    nodelay(stdscr, TRUE);  // program does not pause for input at getch calls.
    curs_set(0);            // make cursor invisible.

    srand((unsigned) time(NULL));   // Set rand() seed by current time.

    // ALLOCATE FRAME AND BUFFER
    char ** game_world = malloc(sizeof(char *) * (SCREEN_HEIGHT + 1));
    check_alloc(game_world);


    for(int i=0; i<SCREEN_HEIGHT + 1; i++){
        game_world[i] = malloc(sizeof(char) * (SCREEN_WIDTH + 2));
        check_alloc(game_world[i]);

        for(int j=0; j<SCREEN_WIDTH; j++){
            game_world[i][j] = OFF;
        }
        game_world[i][SCREEN_WIDTH+1] = '\0';

        // draw frame around board.
        game_world[i][0] = '#';
        game_world[i][SCREEN_WIDTH] = '#';
    }

    // finish frame around board.
    for(int i=1; i<SCREEN_WIDTH; i++){
        game_world[0][i] = '#';
        game_world[SCREEN_HEIGHT][i] = '#';
    }
    char score_str[64];
    char c; // general utility char
    int score;

    START_GAME_LOOP: // naught, naughty goto
    score = game_loop(game_world);
    // DISPLAY END SCREEN
    clear();
    
    sprintf(score_str, "SCORE: %d", score);
    mvprintw(SCREEN_HEIGHT/2-3, SCREEN_WIDTH/2 - 4, "         ");
    mvprintw(SCREEN_HEIGHT/2-2, SCREEN_WIDTH/2 - 4, "GAME OVER");
    mvprintw(SCREEN_HEIGHT/2-1, SCREEN_WIDTH/2 - 4, "          ");
    mvprintw(SCREEN_HEIGHT/2, SCREEN_WIDTH/2 - (strlen(score_str)/2), "%s", score_str);
    mvprintw(SCREEN_HEIGHT/2+1, SCREEN_WIDTH/2 - 4, "           ");
    mvprintw(SCREEN_HEIGHT/2+2, SCREEN_WIDTH/2 - 4, "[N]EW GAME");
    mvprintw(SCREEN_HEIGHT/2+3, SCREEN_WIDTH/2 - 4, "[Q]UIT");
    mvprintw(SCREEN_HEIGHT/2+4, SCREEN_WIDTH/2 - 4, "           ");
    nodelay(stdscr, FALSE);
    refresh();
    while((c = getch()) != 'q' && c != 'Q' && c != 'n' && c != 'N');

    // Set up for restart game
    if(c == 'n' || c == 'N'){
        clear();
        nodelay(stdscr, TRUE);
        for(int i=0; i<SCREEN_HEIGHT + 1; i++){
            for(int j=0; j<SCREEN_WIDTH; j++){
                game_world[i][j] = OFF;
            }
            game_world[i][SCREEN_WIDTH+1] = '\0';

            game_world[i][0] = '#';
            game_world[i][SCREEN_WIDTH] = '#';
        }

        for(int i=1; i<SCREEN_WIDTH; i++){
            game_world[0][i] = '#';
            game_world[SCREEN_HEIGHT][i] = '#';
        }           
        goto START_GAME_LOOP;
    }

    // DEALLOCATION
    for(int i=0; i<SCREEN_HEIGHT; i++) free(game_world[i]);
    free(game_world);

    endwin();
    return 0;
}

int game_loop(char ** game_world){
    
    // DECLARING GAME VARIABLES
    int c; // inputs since last frame
    int direction = UP;
    int status = 0;
    bool end_game = false;
    int snake_head[2] = {SCREEN_WIDTH / 2, SCREEN_HEIGHT - 2};
    int snake_end[3] = {SCREEN_WIDTH / 2, SCREEN_HEIGHT - 1, UP};
    int apple[2];
    place_apple(game_world, apple, snake_head[0] % 2);
    int snek_length = 2;
    draw_at_coords(game_world, snake_head, heads[UP]);
    draw_at_coords(game_world, snake_end, heads[UP]);
    draw_at_coords(game_world, apple, ON);

    // ACTUAL EVENT LOOP
    while(1){
        clear();
        int delay = BASE_DELAY;

        // GET INPUTS SINCE LAST DRAW
        while((c = getch()) != ERR){
            if(c == 'w' && direction != DOWN){
                direction = UP;
            } else if (c=='a' && direction != RIGHT){
                direction = LEFT;
            } else if(c=='s' && direction != UP){
                direction = DOWN;
            } else if(c=='d' && direction != LEFT){
                direction = RIGHT;
            } else if(c=='q'){
                end_game = true;
            }
        }

        // Replace the current snake head with a snake head in the new direction
        draw_at_coords(game_world, snake_head, heads[direction]);
        move_snake(direction, snake_head);

        // Check if snake head overlaps with apple
        if(snake_head[0] == apple[0] && snake_head[1] == apple[1]){
            draw_at_coords(game_world, snake_head, heads[direction]);
            place_apple(game_world, apple, snake_head[0] % 2);
            draw_at_coords(game_world, apple, ON);
            snek_length++;
        } else {
        
            // Behavior if no apple is present: delete current snake tail and move it forward
            if(!check_position_legal(game_world, snake_head)){
                update_screen(game_world);
                break;
            }
            draw_at_coords(game_world, snake_head, heads[direction]);

            for(int i=0; i<4; i++){
                if(heads[i] == game_world[snake_end[1]][snake_end[0]]){
                    snake_end[2] = i;
                    break;
                }
            }
            draw_at_coords(game_world, snake_end, OFF);
            move_snake(snake_end[2], snake_end);
        }

        update_screen(game_world);
        refresh();
        usleep(delay);
    }
    return snek_length;
}

void check_alloc(void * ptr){
    if(ptr == NULL){
        fprintf(stderr, "\nFailed to allocate memory!\n");
        exit(EXIT_FAILURE);
    }
}

void place_apple(char ** board, int * pos, int offset){
    pos[1] = (rand() % SCREEN_HEIGHT) + 1;
    pos[0] = (rand() % SCREEN_WIDTH/2) * 2 + offset;
    while(board[pos[1]][pos[0]] != ' '){
        pos[1] = (rand() % SCREEN_HEIGHT) + 1;
        pos[0] = (rand() % SCREEN_WIDTH/2) * 2 + offset;
    }
}

bool check_position_legal(char ** board, int * pos){
    if (pos[0] < 1 || pos[1] < 1 || pos[0] >= SCREEN_WIDTH || pos[1] >= SCREEN_HEIGHT|| board[pos[1]][pos[0]] != OFF ){
        return false;
    }
    return true;
}

void update_screen(char ** frame){
    for(int i=0; i<=SCREEN_HEIGHT; i++){
        printw("%s\n", frame[i]);
    }
}

bool is_number(char * str){
    for(char *c = str; *c!='\0'; c++){
        if(*c < '0' || *c > '9'){
            if(*c != '-') return false;
        }
    }
    return true;
}

void draw_at_coords(char ** board, int * coords, char c){
    board[coords[1]][coords[0]] = c;
}

void move_snake(int direction, int * coords){
    switch(direction){
        case UP:
            coords[1]--; break;
        case DOWN:
            coords[1]++; break;
        case LEFT:
            coords[0] -= 2; break;
        case RIGHT:
            coords[0] += 2; break;
        default:
            break;
    }
}