#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

// =====================================================================
// Common.h
// Definitions communes a tout le projet : enums, constantes
// =====================================================================

// Etat d'une case
enum class CellState {
    EMPTY = 0,
    CROSS = 1,   // X
    CIRCLE = 2   // O
};

// Etat d'une sous-grille 3x3
enum class SubBoardState {
    IN_PROGRESS,    // Partie en cours sur cette sous-grille
    WON_BY_CROSS,   // Gagnee par X
    WON_BY_CIRCLE,  // Gagnee par O
    DRAW            // Pleine sans vainqueur
};

// Constantes globales
static constexpr int GRID_SIZE = 3;          // Taille d'une sous-grille (3x3)
static constexpr int NB_SUBGRIDS = 9;        // Nombre de sous-grilles
static constexpr int BOARD_FULL_SIZE = 9;    // 9x9 = 81 cases

// Renvoie le symbole adverse
inline CellState opponent(CellState c)
{
    if (c == CellState::CROSS)  return CellState::CIRCLE;
    if (c == CellState::CIRCLE) return CellState::CROSS;
    return CellState::EMPTY;
}

#endif // COMMON_H_INCLUDED
