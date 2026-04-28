#ifndef MOVE_H_INCLUDED
#define MOVE_H_INCLUDED

// =====================================================================
// Move.h
// Representation d'un coup. row/col sont les coordonnees absolues
// dans la grille 9x9 (0..8).
// On peut deduire :
//   - sous-grille    : (row/3, col/3)
//   - case dans la sous-grille : (row%3, col%3)
// =====================================================================

struct Move {
    int row;
    int col;

    Move() : row(-1), col(-1) {}
    Move(int r, int c) : row(r), col(c) {}

    bool isValid() const { return row >= 0 && row < 9 && col >= 0 && col < 9; }

    int subGridRow() const { return row / 3; }
    int subGridCol() const { return col / 3; }
    int cellRow()    const { return row % 3; }
    int cellCol()    const { return col % 3; }
};

#endif // MOVE_H_INCLUDED
