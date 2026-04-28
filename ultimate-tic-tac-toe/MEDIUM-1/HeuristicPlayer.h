#ifndef HEURISTICPLAYER_H_INCLUDED
#define HEURISTICPLAYER_H_INCLUDED

#include "IPlayer.h"

// =====================================================================
// HeuristicPlayer.h
// Joueur IA base sur Alpha-Beta avec une heuristique d'evaluation
// adaptee a Ultimate Tic-Tac-Toe.
//
// Strategie :
//   - Profondeur de recherche configurable (defaut 5)
//   - Heuristique multi-criteres :
//       * Sous-grilles gagnees (avec bonus selon position : centre
//         et coins valent plus que les bords)
//       * Lignes potentielles sur la meta-grille (2 sous-grilles
//         alignees + 1 encore en cours = forte menace)
//       * Pour chaque sous-grille en cours : score local 3x3
//       * Penalite si on envoie l'adversaire vers une sous-grille
//         "libre choix" (mauvais : on lui donne tout)
// =====================================================================

class HeuristicPlayer : public IPlayer
{
public:
    explicit HeuristicPlayer(int searchDepth = 5);

    Move chooseMove(const Game& game, CellState mySymbol) override;

    void setDepth(int d) { m_depth = d; }

private:
    int m_depth;
    CellState m_me;
    CellState m_opp;

    // Coeur de l'algorithme : Alpha-Beta avec memo de profondeur
    int alphaBeta(Game& g, int depth, int alpha, int beta,
                  bool maximizing, CellState toPlay);

    // Evaluation statique de l'etat du jeu du point de vue de m_me
    int evaluate(const Game& g) const;

    // Evaluation d'une sous-grille 3x3 du point de vue de "player"
    int evaluateSubGrid(const SubBoard& sb, CellState player) const;

    // Score d'une "ligne" (3 cellules d'une meta-grille ou sous-grille)
    static int scoreLine(int myCount, int oppCount);
};

#endif // HEURISTICPLAYER_H_INCLUDED
