#ifndef IPLAYER_H_INCLUDED
#define IPLAYER_H_INCLUDED

#include "Common.h"
#include "Move.h"
#include "Game.h"

// =====================================================================
// IPlayer.h
// Interface abstraite pour un joueur IA.
// Permet de basculer facilement d'une strategie a une autre
// (Random, Heuristique, Alpha-Beta, MCTS...)
// =====================================================================

class IPlayer
{
public:
    virtual ~IPlayer() = default;

    // Choisit le meilleur coup pour le joueur "mySymbol" sur l'etat
    // donne. La methode NE modifie PAS l'etat du jeu.
    virtual Move chooseMove(const Game& game, CellState mySymbol) = 0;
};

#endif // IPLAYER_H_INCLUDED
