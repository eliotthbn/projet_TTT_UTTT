#include "main.h"
#include "Common.h"
#include "Move.h"
#include "Game.h"
#include "HeuristicPlayer.h"

#include <iostream>
#include <chrono>
#include <memory>

// =====================================================================
// main.cpp - Ultimate Tic-Tac-Toe
//
// Boucle principale : on synchronise notre etat de jeu local avec le
// moteur fourni en lisant les coups adverses via getMove() et en
// envoyant nos coups via setMove(). Notre IA (HeuristicPlayer) calcule
// le coup optimal selon son heuristique + Alpha-Beta.
// =====================================================================

// On part du principe que l'on joue les ronds (O = CIRCLE) en 2eme.
static constexpr CellState OUR_SYMBOL = CellState::CIRCLE;
static constexpr CellState OPP_SYMBOL = CellState::CROSS;

int main()
{
    // Initialisation du jeu :
    //  - 10 parties (mode Debug)
    //  - Niveau MEDIUM_1 pour commencer (a augmenter une fois valide)
    //  - Mode DEBUG pour voir les logs (passer en ARENA pour valider)
    //  - alwaysPlayFirst = false : on joue en 2eme (ronds)
    game.initialize(10, Level::MEDIUM_1, Mode::ARENA, false, "AlphaBeta_AI");

    // Notre joueur IA (profondeur 5 par defaut)
    std::unique_ptr<IPlayer> ia = std::make_unique<HeuristicPlayer>(5);

    // Boucle sur l'ensemble des parties
    while (!game.isAllGameFinish())
    {
        // Reinitialisation de notre etat local pour la nouvelle partie
        Game localGame;
        std::cerr << "\n========== NOUVELLE PARTIE ==========\n";

        // Boucle d'une partie unique
        while (!game.isFinish())
        {
            // 1) Recuperer le coup de l'adversaire (s'il y en a un)
            //    On se fie au CONTENU de oppMove (coordonnees valides)
            //    plutot qu'au booleen retourne, qui n'est pas fiable.
            GameMove oppMove = {-1, -1};
            game.getMove(oppMove);

            if (oppMove.row >= 0 && oppMove.row < 9
                && oppMove.col >= 0 && oppMove.col < 9)
            {
                Move opp(oppMove.row, oppMove.col);
                int subIdx = opp.subGridRow() * 3 + opp.subGridCol();
                CellState cur = localGame.getSubBoard(subIdx)
                                         .getCell(opp.cellRow(), opp.cellCol());
                // On ne re-traite pas un coup deja enregistre
                if (cur == CellState::EMPTY)
                {
                    if (!localGame.playMove(opp, OPP_SYMBOL))
                    {
                        std::cerr << "[WARN] Coup adverse refuse : "
                                  << opp.row << "," << opp.col << "\n";
                    }
                    else
                    {
                        std::cerr << "Coup adverse : " << opp.row << " " << opp.col << "\n";
                    }
                }
            }

            // 2) Securite : la partie peut s'etre terminee sur le coup adverse
            if (game.isFinish()) break;

            // 3) Calcul de notre coup (mesure du temps pour info)
            auto t0 = std::chrono::high_resolution_clock::now();
            Move myMove = ia->chooseMove(localGame, OUR_SYMBOL);
            auto t1 = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

            if (!myMove.isValid())
            {
                std::cerr << "[WARN] Aucun coup legal trouve.\n";
                break;
            }

            // 4) Appliquer notre coup sur notre etat local
            if (!localGame.playMove(myMove, OUR_SYMBOL))
            {
                std::cerr << "[ERROR] Coup auto-genere illegal !\n";
                break;
            }

            std::cerr << "Mon coup : " << myMove.row << " " << myMove.col
                      << " (" << ms << " ms)\n";

            // 5) Envoyer le coup au moteur
            GameMove out;
            out.row = myMove.row;
            out.col = myMove.col;
            game.setMove(out);
        }

        // Affichage du resultat de la partie
        Winner w = game.getWinner();
        std::cerr << "Resultat : ";
        switch (w)
        {
            case NO_WINNER:     std::cerr << "Match nul\n"; break;
            case IA:            std::cerr << "L'IA adverse a gagne\n"; break;
            case PLAYER:        std::cerr << "VICTOIRE !\n"; break;
            case IA_AND_PLAYER: std::cerr << "Egalite\n"; break;
        }
    }

    std::cerr << "\n=== Toutes les parties sont terminees ===\n";
    return 0;
}
