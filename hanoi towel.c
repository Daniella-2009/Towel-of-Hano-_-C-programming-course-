#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// during the last session, instead of putting 4, you ask us to set the limit at 10
#define MAX_DISKS 64       // maximum disks we can display nicely
#define PEGS 3
#define DELAY_MS 500        // pause between moves (milliseconds)

// ---- Global state of the puzzle ----
int pegs[PEGS][MAX_DISKS];  // pegs[p][pos] = disk size (0 = empty)
int tops[PEGS];             // number of disks currently on each peg
int num_disks;              // total disks in the game
int p;
int level;
int i;
int ms;

// ---- Simple cross-platform delay (busy-wait) ----
void delay(ms) {
    clock_t start = clock();
    while ((clock() - start) * 1000000000000 / CLOCKS_PER_SEC < ms); // the last time, for 1000, the time where 17 min for 64 disk. since i increased the time it will be much more faster 
}

// ---- Clear the terminal screen ----
void clear_screen(void) {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// ---- Draw the current state of the pegs ----
void draw(void) {
    clear_screen();
    int max_height = num_disks;       // tallest a peg can be
    int max_width = 2 * num_disks - 1; // width of the largest disk

    // Print from top (highest disk position) down to ground
    for (level = max_height - 1; level >= 0; level--) {
        for (p = 0; p < PEGS; p++) {
            // space between pegs
            if (p > 0) printf("   ");

            if (level < tops[p]) {
                // There is a disk at this level
                int size = pegs[p][level];
                int disk_width = 2 * size - 1;
                int padding = (max_width - disk_width) / 2;

                // left padding
                for ( i = 0; i < padding; i++) putchar(' ');
                // disk itself (using '=')
                for ( i = 0; i < disk_width; i++) putchar('=');
                // right padding
                for ( i = 0; i < padding; i++) putchar(' ');
            } else {
                // No disk – just the pole
                int padding = (max_width - 1) / 2;
                for ( i = 0; i < padding; i++) putchar(' ');
                putchar('|');
                for ( i = 0; i < padding; i++) putchar(' ');
            }
        }
        putchar('\n');
    }

    // Ground line
    for (p = 0; p < PEGS; p++) {
        if (p > 0) printf("   ");
        for ( i = 0; i < max_width; i++) putchar('-');
    }
    printf("\n  Peg1          Peg2              Peg3\n\n");
}

// ---- Move the top disk from one peg to another (with animation) ----
void move_disk(int from, int to) {
    // Pop from 'from' peg
    int disk = pegs[from][tops[from] - 1];
    pegs[from][tops[from] - 1] = 0;
    tops[from]--;

    // Push onto 'to' peg
    pegs[to][tops[to]] = disk;
    tops[to]++;

    // Redraw and pause
    draw();
    delay(DELAY_MS);
}

// ---- Recursive Hanoi solution, exactly as described in the book ----
void hanoi(int n, int from, int to, int aux) {
    if (n == 1) {
        move_disk(from, to);            // base case: just move one disk
    } else {
        hanoi(n - 1, from, aux, to);    // Step 1: n-1 disks to auxiliary
        move_disk(from, to);            // Step 2: largest disk to destination
        hanoi(n - 1, aux, to, from);    // Step 3: n-1 disks from auxiliary to destination
    }
}

int main(void) {
    printf("How many disks? (1 to %d): ", MAX_DISKS);
    scanf("%d", &num_disks);
    if (num_disks < 1 || num_disks > MAX_DISKS) {
        printf("Invalid number of disks. Using 3.\n");
        num_disks = 3;
    }

    // Initialize: all disks on peg 0 (peg 1), largest at bottom (index 0)
    tops[0] = num_disks;
    tops[1] = 0;
    tops[2] = 0;
    for ( i = 0; i < num_disks; i++) {
        pegs[0][i] = num_disks - i;   // bottom: num_disks, top: 1
    }
    // Clear other pegs
    for ( p = 1; p < PEGS; p++) {
        for ( i = 0; i < MAX_DISKS; i++) {
            pegs[p][i] = 0;
        }
    }

    // Show initial state
    draw();
    delay(1000);   // let user see the starting position

    // Start the recursive solution
    // Move 'num_disks' from peg 0 (1) to peg 2 (3), using peg 1 (2) as auxiliary
    hanoi(num_disks, 0, 2, 1);

    printf("Puzzle solved!\n");
    return 0;
}
