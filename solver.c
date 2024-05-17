// $ gcc -O3 -march=native -flto solver.c -o solver
// Execute as:   $ ./solver May 1   or  $ ./solver Nov 11

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH 13
#define HEIGHT 13
#define MIN_R 3
#define MIN_C 3
#define MAX_R 9
#define MAX_C 9

//                           1 1 1
//   c = 0 1 2 3 4 5 6 7 8 9 0 1 2 (column)
//     0 * * * * * * * * * * * * * 
//     1 * * * * * * * * * * * * *      We only iterate between (r,c)=(3,3)to(9,9)
//     2 * * * * * * * * * * * * *      We never worry about r-1,r-2,r-3,r+1,r+2,r+3  
//     3 * * * X X X X X X * * * *      We never worry about c-1,c-2,c-3,c+1,c+2,c+3
//     4 * * * X X X X X X * * * *         because their values fall into valid cells that will be set to occupied  
//     5 * * * X X X X X X X * * * 
//     6 * * * X X X X X X X * * *      (r,c)=(3,9) & (4,9) in the real board game are also blocked  
//     7 * * * X X X X X X X * * *      (r,c)=(9,3) & (9,4) & (9,8) & (9,9) in the real board game are also blocked
//     8 * * * X X X X X X X * * * 
//     9 * * * * * X X X * * * * * 
//    10 * * * * * * * * * * * * * 
//    11 * * * * * * * * * * * * *
//    12 * * * * * * * * * * * * * 
//     ^ 
//     r = row
//

typedef struct {
    int id, width, height;
    bool shape[5][5];  // Max piece dimensions
} Piece;

int tempBoard[HEIGHT][WIDTH];
int board[HEIGHT][WIDTH];
bool used[9];  // Tracks which pieces have been used
clock_t start, end;
double cpu_time_used;


// Pieces definition based on your inputs
Piece pieces[9] = {
    {0, 2, 4, {{false, true},{false, true}, {false, true}, {true, true}}}, // Large L-shape
    {1, 3, 3, {{false, false, true},{true, true, true},{true, false, false}}}, // 2-Ls Fused Tetromino shape  
    {2, 3, 3, {{false, true, false}, {true, true, true}, {false, true, false}}}, // + sign shape [Symmetrical]
    {3, 3, 2, {{true, false, true}, {true, true, true}}}, // U-shape
    {4, 2, 3, {{true, false},{true, true},{true, true}}}, // Square with a lip shape
    {5, 2, 2, {{true, true}, {true, true}}}, // Square 2x2 shape  [Symmetrical]
    {6, 2, 3, {{true, false},{true, true},{false,true}}}, // Knight-move shape
    {7, 3, 2, {{true, true, true},{false, true, false}}}, // T-shape
    {8, 2, 3, {{true, false},{true, false}, {true, true}}} // Small L-shape  
};

void initBoard() {
	int rr,cc;
	
    for (rr = 0; rr < HEIGHT; rr++) 
        for (cc = 0; cc < WIDTH; cc++) 
            board[rr][cc] = -2; // Set every cell to '-2' (empty)
			
	for (rr = 0; rr <= 2; rr++)
        for (cc = 0; cc < WIDTH; cc++)
            board[rr][cc] = -1; // Set every cells to '-1' (full)			
			
	for (rr = 10; rr <= 12; rr++)
        for (cc = 0; cc < WIDTH; cc++)
            board[rr][cc] = -1; // Set every cells to '-1' (full)				
			
	for (cc = 0; cc <= 2; cc++)
        for (rr = 0; rr < HEIGHT; rr++)
            board[rr][cc] = -1; // Set every cells to '-1' (full)			
			
	for (cc = 10; cc <= 12; cc++)
        for (rr = 0; rr < HEIGHT; rr++)
            board[rr][cc] = -1; // Set every cells to '-1' (full)			
}

void printBoard() {
	// int status = system("clear");  // Clears the screen in Unix/Linux
	printf("   3 4 5 6 7 8 9\n");
    for (int i = 3; i < 10; i++) {
        for (int j = 3; j < 10; j++) {
			if (j==3) printf("%2d ",i);
            if (board[i][j] == -1) printf("X ");
              else if (board[i][j] == -2) printf(". ");
                  else printf("%d ", board[i][j]);
        }
    printf("\n");
	} 
}

void rotate90(Piece *p) {
    Piece temp = *p;
    p->width = temp.height;
    p->height = temp.width;
    for (int i = 0; i < temp.height; i++) {
        for (int j = 0; j < temp.width; j++) {
            p->shape[j][temp.height - 1 - i] = temp.shape[i][j];
        }
    }
}

void mirrorPiece(Piece *p) {
    Piece temp = *p;
    for (int i = 0; i < temp.height; i++) {
        for (int j = 0; j < temp.width; j++) {
            p->shape[i][temp.width - 1 - j] = temp.shape[i][j];
        }
    }
}


bool isIsolated(int r, int c) {
    // Basic boundary check
    bool up =   (tempBoard[r-1][c] != -2);
    bool down = (tempBoard[r+1][c] != -2);
    bool left = (tempBoard[r][c-1] != -2);
    bool right = (tempBoard[r][c+1] != -2);
    bool isolatedSingle = up && down && left && right;

    if (isolatedSingle) return 1;

    // Check for horizontal and vertical pair isolation
    bool isolatedHorizontalPair = (   (tempBoard[r][c]   == -2 && tempBoard[r][c + 1] == -2)
                                   && (tempBoard[r][c-1] != -2 && tempBoard[r][c+2]   != -2)
                                   && (tempBoard[r-1][c] != -2 && tempBoard[r-1][c+1] != -2)
                                   && (tempBoard[r+1][c] != -2 && tempBoard[r+1][c+1] != -2) );                                  
    
    
    bool isolatedVerticalPair =   (    (tempBoard[r][c] == -2 && tempBoard[r + 1][c] == -2)
                                    && (tempBoard[r-1][c] != -2 && tempBoard[r + 2][c] != -2)
                                    && (tempBoard[r][c-1] != -2 && tempBoard[r][c+1] != -2)
                                    && (tempBoard[r+1][c-1] != -2 && tempBoard[r+1][c+1] != -2));
                                     
    if (isolatedHorizontalPair || isolatedVerticalPair) return (isolatedHorizontalPair || isolatedVerticalPair);

    // Check for horizontal and vertical triple isolation
    bool isolatedHorizontalTriple = (   (tempBoard[r][c]   == -2 && tempBoard[r][c + 1] == -2 && tempBoard[r][c + 2] == -2)
                                      && (tempBoard[r][c-1] != -2 && tempBoard[r][c+3]   != -2)
                                      && (tempBoard[r-1][c] != -2 && tempBoard[r-1][c+1] != -2 && tempBoard[r-1][c+2] != -2)
                                      && (tempBoard[r+1][c] != -2 && tempBoard[r+1][c+1] != -2 && tempBoard[r+1][c+2] != -2) );
    
    bool isolatedVerticalTriple =   (   (tempBoard[r][c] == -2 && tempBoard[r + 1][c] == -2 && tempBoard[r + 2][c] == -2)
                                      && (tempBoard[r-1][c] != -2 && tempBoard[r + 3][c] != -2)
                                      && (tempBoard[r][c-1] != -2 && tempBoard[r+1][c-1] != -2 && tempBoard[r+2][c-1] != -2)
                                      && (tempBoard[r][c+1] != -2 && tempBoard[r+1][c+1] != -2 && tempBoard[r+2][c+1] != -2) );

    if (isolatedHorizontalTriple || isolatedVerticalTriple) return 1;

    // Check for "little L" shape isolation
    bool isolatedLShape1 = (tempBoard[r][c] == -2 && tempBoard[r + 1][c] == -2 && tempBoard[r][c + 1] == -2
                            && tempBoard[r - 1][c] != -2 && tempBoard[r + 2][c] != -2
                            && tempBoard[r][c - 1] != -2 && tempBoard[r][c + 2] != -2
                            && tempBoard[r + 1][c - 1] != -2 && tempBoard[r + 1][c + 2] != -2);
    
    bool isolatedLShape2 = (tempBoard[r][c] == -2 && tempBoard[r + 1][c] == -2 && tempBoard[r][c - 1] == -2
                            && tempBoard[r - 1][c] != -2 && tempBoard[r + 2][c] != -2
                            && tempBoard[r][c + 1] != -2 && tempBoard[r][c - 2] != -2
                            && tempBoard[r + 1][c + 1] != -2 && tempBoard[r + 1][c - 2] != -2);
    
    bool isolatedLShape3 = (tempBoard[r][c] == -2 && tempBoard[r - 1][c] == -2 && tempBoard[r][c + 1] == -2
                            && tempBoard[r + 1][c] != -2 && tempBoard[r - 2][c] != -2
                            && tempBoard[r][c - 1] != -2 && tempBoard[r][c + 2] != -2
                            && tempBoard[r - 1][c - 1] != -2 && tempBoard[r - 1][c + 2] != -2);
    
    bool isolatedLShape4 = (tempBoard[r][c] == -2 && tempBoard[r - 1][c] == -2 && tempBoard[r][c - 1] == -2
                            && tempBoard[r + 1][c] != -2 && tempBoard[r - 2][c] != -2
                            && tempBoard[r][c + 1] != -2 && tempBoard[r][c - 2] != -2
                            && tempBoard[r - 1][c + 1] != -2 && tempBoard[r - 1][c - 2] != -2);

    if (isolatedLShape1 || isolatedLShape2 || isolatedLShape3 || isolatedLShape4) return 1;

    // Check for four contiguous squares in a horizontal line
    bool isolatedHorizontalFour = (tempBoard[r][c] == -2 && tempBoard[r][c + 1] == -2 && tempBoard[r][c + 2] == -2 && tempBoard[r][c + 3] == -2
                                   && tempBoard[r][c - 1] != -2 && tempBoard[r][c + 4] != -2
                                   && tempBoard[r - 1][c] != -2 && tempBoard[r - 1][c + 1] != -2 && tempBoard[r - 1][c + 2] != -2 && tempBoard[r - 1][c + 3] != -2
                                   && tempBoard[r + 1][c] != -2 && tempBoard[r + 1][c + 1] != -2 && tempBoard[r + 1][c + 2] != -2 && tempBoard[r + 1][c + 3] != -2);
    
	if (isolatedHorizontalFour) return 1;
	
    // Check for four contiguous squares in a vertical line
    bool isolatedVerticalFour = (tempBoard[r][c] == -2 && tempBoard[r + 1][c] == -2 && tempBoard[r + 2][c] == -2 && tempBoard[r + 3][c] == -2
                                 && tempBoard[r - 1][c] != -2 && tempBoard[r + 4][c] != -2
                                 && tempBoard[r][c - 1] != -2 && tempBoard[r + 1][c - 1] != -2 && tempBoard[r + 2][c - 1] != -2 && tempBoard[r + 3][c - 1] != -2
                                 && tempBoard[r][c + 1] != -2 && tempBoard[r + 1][c + 1] != -2 && tempBoard[r + 2][c + 1] != -2 && tempBoard[r + 3][c + 1] != -2);

    if (isolatedVerticalFour) return 1; 

    return isolatedSingle || isolatedHorizontalPair || isolatedVerticalPair || isolatedHorizontalTriple || isolatedVerticalTriple || isolatedLShape1 || isolatedLShape2 || isolatedLShape3 || isolatedLShape4 || isolatedHorizontalFour || isolatedVerticalFour;
}



bool canPlace(Piece p, int row, int col, int w_i) {
    if (((row + p.height) >= HEIGHT) || ((col + p.width) >= WIDTH))  return false;
 
    memcpy(tempBoard, board, sizeof(tempBoard));

    // Simulate placing the piece
    for (int i = 0; i < p.height; i++) {
        for (int j = 0; j < p.width; j++) {
            if (p.shape[i][j]) {
                if (tempBoard[row + i ][col + j ] != -2) // Already occupied
				    return false;
				tempBoard[row + i ][col + j ] = w_i; // Simulate placement
            }
        }
    }
 

    // Check for isolated empty squares
    for (int r = MIN_R; r <= MAX_R; r++) {
        for (int c = MIN_C; c <= MAX_C; c++) {
            if (tempBoard[r][c] == -2 && isIsolated(r, c)) 
     		   return false; // Found an isolated empty square so canPlace() needs to return FALSE
        }
    }
    return true;
}

void placePiece(Piece p, int row, int col, int pieceIndex, bool place) {
    for (int i = 0; i < p.height; i++) {
        for (int j = 0; j < p.width; j++) {
            if (p.shape[i][j])
                board[row + i ][col + j ] = place ? pieceIndex : -2;
        }
    }
}

bool solve(int placedPieces, int depth) {
    if (placedPieces == 9)
        return true;  // All pieces placed successfully

    for (int i = 0; i < 9; i++) {
        if (!used[i]) {
            used[i] = true;
            int transformations = 8;  // 4 rotations + 4 mirrored rotations

            for (int t = 0; t < transformations; t++) {
                if (t == 4) mirrorPiece(&pieces[i]);  // Mirror after 4 rotations

                for (int r = MIN_R; r <= MAX_R; r++) {
                    for (int c = MIN_C; c <= MAX_C; c++) {
                        if (canPlace(pieces[i], r, c, i)) {
                            placePiece(pieces[i], r, c, i, true);
                            if (solve(placedPieces + 1, depth + 1)) return true;
                            placePiece(pieces[i], r, c, i, false);
                        }
                    }
                }
                rotate90(&pieces[i]);  // Rotate piece
            }
            used[i] = false;  // Backtrack
            return false;  // Immediately backtrack if no valid placement found for this piece
        }
    }
    return false;  // No valid placement found
}

const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const int r_months[] = {3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4};
const int c_months[] = {3, 4, 5, 6, 7, 8, 3, 4, 5, 6, 7, 8};

const int r_days[] = {5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9};
const int c_days[] = {3, 4, 5, 6, 7, 8, 9, 3, 4, 5, 6, 7, 8, 9, 3, 4, 5, 6, 7, 8, 9, 3, 4, 5, 6, 7, 8, 9, 5, 6, 7};

void getPosition(const char *month, int day, int *m_row, int *m_col, int *d_row, int *d_col) {
    for (int i = 0; i < 12; i++) {
        if (strcmp(month, months[i]) == 0) {
            *m_row = r_months[i];
            *m_col = c_months[i];
            break;
        }
    }
    if (day >= 1 && day <= 31) {
        *d_row = r_days[day - 1];
        *d_col = c_days[day - 1];
    }
}


int main(int argc, char *argv[]) {
	
	if (argc != 3) {
    printf("Usage: %s <month> <day>\n", argv[0]);
    return 1;
    }
    
    char *month = argv[1];
    int day = atoi(argv[2]);
    int m_row, m_col, d_row, d_col;
    
    getPosition(month, day, &m_row, &m_col, &d_row, &d_col);
	
	initBoard();

    // Marking fixed unavailable spaces as 'X' (blocked)
    board[3][9] = -1;
    board[4][9] = -1;
    board[9][3] = -1;
    board[9][4] = -1;
    board[9][8] = -1;
    board[9][9] = -1;

    board[m_row][m_col] = -1; // Mark the first square as blocked
    board[d_row][d_col] = -1; // Mark the second square as blocked

    start = clock();

    printf("%s %s\n\n",argv[1],argv[2]);

    if (solve(0,0)) {
        printBoard();
    } else {
        printf("No solution found.\n");
    }
	
	end = clock();
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("\nTime taken to execute: %f seconds\n\n", cpu_time_used);
	
    return 0;
}
	
	