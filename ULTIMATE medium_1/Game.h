#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include "Common.h"
#include "Move.h"
#include "SubBoard.h"
#include <array>
#include <vector>

// =====================================================================
// Game.h
// Represente l'etat complet d'une partie d'Ultimate Tic-Tac-Toe :
//   - 9 sous-grilles
//   - sous-grille active (-1 si libre choix)
//   - joueur courant
// Gere les regles d'envoi et la victoire globale.
// =====================================================================

class Game
{
public:
    Game();

    // Reinitialise une partie
    void reset();

    // Joue un coup (coordonnees absolues 0..8). Retourne false si invalide.
    bool playMove(const Move& move, CellState player);

    // Verifie si un coup serait legal (selon la regle d'envoi)
    bool isLegalMove(const Move& move, CellState player) const;

    // Genere tous les coups legaux pour le joueur courant
    std::vector<Move> getLegalMoves() const;

    // Indices (0..8) des sous-grilles ouvertes ou la regle d'envoi
    // autorise a jouer (cf. logique "envoye sur grille pleine -> libre choix")
    int getActiveSubGrid() const { return m_activeSubGrid; } // -1 = libre choix

    // Etat global
    bool isGameFinished() const;
    CellState getGlobalWinner() const; // EMPTY si pas (encore) de vainqueur

    // Score : nombre de sous-grilles gagnees par chaque joueur
    int countSubGridsWon(CellState player) const;

    // Acces lecture aux sous-grilles
    const SubBoard& getSubBoard(int idx) const { return m_subBoards[idx]; }

    // Pour le debug
    void print() const;

private:
    std::array<SubBoard, NB_SUBGRIDS> m_subBoards;

    // Index (0..8) de la sous-grille ou le prochain joueur DOIT jouer.
    // -1 si libre choix (premier coup ou envoye sur grille terminee).
    int m_activeSubGrid;

    // Verifie l'alignement de 3 sous-grilles gagnees par "player"
    // sur la meta-grille 3x3.
    bool checkGlobalWin(CellState player) const;

    // Convertit (row,col) absolus -> indice de sous-grille (0..8)
    static int subIndex(int subRow, int subCol) { return subRow * GRID_SIZE + subCol; }
};

#endif // GAME_H_INCLUDED
