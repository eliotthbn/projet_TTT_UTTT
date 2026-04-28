#ifndef SUBBOARD_H_INCLUDED
#define SUBBOARD_H_INCLUDED

#include "Common.h"
#include <array>

// =====================================================================
// SubBoard.h
// Represente une des 9 sous-grilles 3x3 d'Ultimate Tic-Tac-Toe.
// Tient a jour son etat (en cours / gagnee / nulle).
// =====================================================================

class SubBoard
{
public:
    SubBoard();

    // Accesseurs
    CellState     getCell(int r, int c) const;
    SubBoardState getState() const { return m_state; }
    bool          isFinished() const;     // Gagnee OU nulle
    bool          isCellEmpty(int r, int c) const;

    // Modificateurs
    void reset();
    bool playMove(int r, int c, CellState player);  // false si coup invalide

    // Verifie si "player" a aligne 3 sur cette sous-grille
    bool checkWin(CellState player) const;

private:
    std::array<std::array<CellState, GRID_SIZE>, GRID_SIZE> m_cells;
    SubBoardState m_state;

    // Recalcule m_state apres un coup
    void updateState();
};

#endif // SUBBOARD_H_INCLUDED
