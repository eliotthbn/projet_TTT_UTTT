#include "main.h"
#include <iostream>
#include <vector>

// ==========================================
// TA FONCTION IA
// ==========================================
GameMove calculerCoupBasique(const std::vector<std::vector<int>>& grille) {
    GameMove monCoup;
    // On cherche la première case vide (0) dans la grille 3x3
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            if (grille[r][c] == 0) {
                monCoup.row = r;
                monCoup.col = c;
                return monCoup;
            }
        }
    }
    // Sécurité : si on ne trouve rien (ne devrait pas arriver)
    monCoup.row = 0; monCoup.col = 0;
    return monCoup;
}

// ==========================================
// LA BOUCLE DE JEU
// ==========================================
int main()
{
    // Game initialization
    game.initialize(10, Level::EASY, Mode::DEBUG, false, "MonIA");

    while (!game.isAllGameFinish())
    {
        // On crée la grille 3x3 en mémoire pour CETTE partie
        std::vector<std::vector<int>> maGrille(3, std::vector<int>(3, 0));

        while (!game.isFinish())
        {
            // 1. L'IA adverse joue
            GameMove gameMove;
            game.getMove(gameMove);
            std::cout << "IA move : " << gameMove.row << " "  << gameMove.col << "\n";

            // On note son coup dans notre mémoire s'il est valide
            if (gameMove.row != -1 && gameMove.col != -1) {
                maGrille[gameMove.row][gameMove.col] = 1;
            }

            // 2. Ton IA analyse la grille et calcule son coup
            GameMove mymove = calculerCoupBasique(maGrille);

            // 3. Tu envoies ton coup au jeu
            game.setMove(mymove);
            std::cout << "Mon move : " << mymove.row << " "  << mymove.col << "\n";

            // 4. Tu notes ton propre coup dans ta mémoire
            maGrille[mymove.row][mymove.col] = 2;
        }
    }

    return 0;
}
