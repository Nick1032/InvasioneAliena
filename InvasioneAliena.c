#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <ncurses.h>

#include <time.h>

#include <stdbool.h>

#include <sys/types.h>

#include <sys/wait.h>



#define PLAYER '^'

#define ENEMY '='

#define PLAYER_PROJECTILE '|'

#define ENEMY_PROJECTILE 'o'



#define SLEEP_TIME 100000



#define ID1 1

#define ID2 2

#define ID3 3

#define ID4 4



typedef struct {

    int id;        // Identificatore

    int x, y;      // Coordinate

    bool active;   // Attivo

} GameState;



void player_process(int pipe_fd[2], int maxX, int maxY);

void enemy_process(int pipe_fd[2], int maxX, int maxY);

void parent_process(int pipe_fd[2], int maxX, int maxY);



int main() {

    int pipe_fd[2];

    pid_t pid1, pid2;



    // Inizializzazione di ncurses

    initscr();

    curs_set(FALSE);

    keypad(stdscr, TRUE);

    noecho();

    int maxY, maxX;

    getmaxyx(stdscr, maxY, maxX);



    // Creazione della pipe

    if (pipe(pipe_fd) == -1) {

        perror("Errore nella creazione della pipe");

        exit(EXIT_FAILURE);

    }



    // Creazione del processo giocatore

    pid1 = fork();

    if (pid1 == 0) {

        close(pipe_fd[0]); // Chiusura lettura pipe

        player_process(pipe_fd, maxX, maxY);

        exit(0);

    }



    // Creazione del processo nemico

    pid2 = fork();

    if (pid2 == 0) {

        close(pipe_fd[0]); // Chiusura lettura pipe

        enemy_process(pipe_fd, maxX, maxY);

        exit(0);

    }



    // Processo principale

    close(pipe_fd[1]); // Chiusura scrittura pipe

    parent_process(pipe_fd, maxX, maxY);



    // Attendi i processi figli

    wait(NULL);

    wait(NULL);



    // Ripristina terminale

    endwin();

    return 0;

}



void player_process(int pipe_fd[2], int maxX, int maxY) {

    GameState player = {ID1, maxX / 2, maxY - 1, true};

    GameState projectile = {ID3, -1, -1, false};



    // Configura ncurses per non bloccare l'input

    nodelay(stdscr, TRUE);



    while (player.active) {

        int ch = getch(); 

        switch (ch) {

            case KEY_LEFT:

                if (player.x > 0) player.x--;

                break;

            case KEY_RIGHT:

                if (player.x < maxX - 1) player.x++;

                break;

            case ' ':

                if (!projectile.active) {

                    projectile.id = ID3;

                    projectile.x = player.x;

                    projectile.y = player.y - 1;

                    projectile.active = true;

                }

                break;

            case KEY_DOWN:

                player.active = false;

                break;

        }



        write(pipe_fd[1], &player, sizeof(GameState));



        // Aggiorna posizione del proiettile

        if (projectile.active) {

            projectile.y--;

            if (projectile.y < 0) {

                projectile.active = false; // Cancella il proiettile

            }

            write(pipe_fd[1], &projectile, sizeof(GameState));

        }



        usleep(SLEEP_TIME);

    }

}





void enemy_process(int pipe_fd[2], int maxX, int maxY) {

    GameState enemy = {ID2, maxX / 2, 0, true};

    GameState projectile = {ID4, -1, -1, false};

    

    nodelay(stdscr, TRUE);

    srand(time(NULL));



    while (enemy.active) {

        int direction = rand() % 2;

        if (direction == 0 && enemy.x > 0) enemy.x--; // Muovi a sinistra

        else if (direction == 1 && enemy.x < maxX - 1) enemy.x++; // Muovi a destra



        write(pipe_fd[1], &enemy, sizeof(GameState));



        // Il nemico spara proiettili casualmente con possibilità del 10%

        if (!projectile.active && rand() % 10 == 0) {

            projectile.id = ID4;

            projectile.x = enemy.x;

            projectile.y = enemy.y + 1;

            projectile.active = true;

        }



        if (projectile.active) {

            projectile.y++;

            if (projectile.y >= maxY) {

                projectile.active = false;

            }

            write(pipe_fd[1], &projectile, sizeof(GameState));

        }



        usleep(SLEEP_TIME);

    }

}



void parent_process(int pipe_fd[2], int maxX, int maxY) {

    GameState msg;

    int playerX = maxX / 2, playerY = maxY - 1;

    int enemyX = maxX / 2, enemyY = 0;

    int playerProjectileX = -1, playerProjectileY = -1;

    int enemyProjectileX = -1, enemyProjectileY = -1;

    bool game_over = false;



    while (!game_over) {

        read(pipe_fd[0], &msg, sizeof(GameState));



        if (msg.id == ID1) {

            playerX = msg.x;

            playerY = msg.y;

        } else if (msg.id == ID2) {

            enemyX = msg.x;

            enemyY = msg.y;

        } else if (msg.id == ID3) {

            playerProjectileX = msg.x;

            playerProjectileY = msg.y;

        } else if (msg.id == ID4) {

            enemyProjectileX = msg.x;

            enemyProjectileY = msg.y;

        }



        clear();

        mvprintw(playerY, playerX, "%c", PLAYER);

        mvprintw(enemyY, enemyX, "%c", ENEMY);



        if (playerProjectileY >= 0) {

            mvprintw(playerProjectileY, playerProjectileX, "%c", PLAYER_PROJECTILE);

        }

        if (enemyProjectileY >= 0) {

            mvprintw(enemyProjectileY, enemyProjectileX, "%c", ENEMY_PROJECTILE);

        }



        refresh();



        // Verifica collisioni

        if (playerProjectileX == enemyX && playerProjectileY == enemyY) {

            game_over = true;

        } else if (enemyProjectileX == playerX && enemyProjectileY == playerY) {

            game_over = true;

        }

    }



    mvprintw(maxY / 2, maxX / 2 - 5, "GAME OVER");

    refresh();

    sleep(2);

}
