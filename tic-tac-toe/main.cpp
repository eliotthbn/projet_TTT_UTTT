#include "main.h"
#include <iostream>
#include <vector>

using namespace std;

// Fonction pour trouver le premier coup valide (niveau "très naïf")
// Tu devras remplacer ça par une vraie logique (Heuristique ou Minimax)
// 1. Fonction utilitaire pour vérifier si un joueur a aligné 3 pions
bool estVictoire(const vector<vector<char>>& grille, char joueur) {
    // Vérification des lignes et colonnes
    for (int i = 0; i < 3; i++) {
        if ((grille[i][0] == joueur && grille[i][1] == joueur && grille[i][2] == joueur) ||
            (grille[0][i] == joueur && grille[1][i] == joueur && grille[2][i] == joueur)) {
            return true;
        }
    }
    // Vérification des deux diagonales
    if ((grille[0][0] == joueur && grille[1][1] == joueur && grille[2][2] == joueur) ||
        (grille[0][2] == joueur && grille[1][1] == joueur && grille[2][0] == joueur)) {
        return true;
    }
    return false;
}

// 2. Fonction qui simule un coup pour voir s'il est gagnant
GameMove trouverCoupGagnant(vector<vector<char>> grille, char joueur) {
    GameMove move = {-1, -1};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (grille[i][j] == ' ') {
                grille[i][j] = joueur; // On simule le pion sur la case vide
                if (estVictoire(grille, joueur)) {
                    move.row = i;
                    move.col = j;
                    return move; // Si c'est gagnant, on renvoie ces coordonnées
                }
                grille[i][j] = ' '; // On annule la simulation et on cherche ailleurs
            }
        }
    }
    return move;
}

// 3. Le nouveau "Cerveau" de ton IA
GameMove calculerCoupIA(const vector<vector<char>>& grille) {
    GameMove move;

    // Règle 1 : Gagner si possible (avec nos 'O')
    move = trouverCoupGagnant(grille, 'O');
    if (move.row != -1) return move;

    // Règle 2 : Bloquer l'adversaire (avec ses 'X')
    move = trouverCoupGagnant(grille, 'X');
    if (move.row != -1) return move;

    // Règle 3 : Prendre le centre si libre
    if (grille[1][1] == ' ') return {1, 1};

    // Règle 4 : Prendre un coin disponible
    int coins[4][2] = {{0, 0}, {0, 2}, {2, 0}, {2, 2}};
    for (int i = 0; i < 4; i++) {
        if (grille[coins[i][0]][coins[i][1]] == ' ') {
            return {coins[i][0], coins[i][1]};
        }
    }

    // Règle 5 : Prendre la première case vide restante (les bords)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (grille[i][j] == ' ') {
                return {i, j};
            }
        }
    }

    return {-1, -1};
}

int main() {
    // Initialisation du jeu selon tes consignes
    // 10 parties, Mode DEBUG, Niveau EASY, l'IA joue en 2ème (false)
    game.initialize(10, Level::HARD, Mode::ARENA, false, "Mon_IA_Morpion");

    // Boucle sur l'ensemble des parties
    while (!game.isAllGameFinish()) {

        // On réinitialise la grille locale au début de chaque NOUVELLE partie
        vector<vector<char>> grilleLocale(3, vector<char>(3, ' '));

        // Boucle d'une partie unique
        // Boucle d'une partie unique
        // Boucle d'une partie unique
        while (!game.isFinish()) {

            // On initialise avec des coordonnées invalides par sécurité
            GameMove moveAdversaire = {-1, -1};

            // On appelle getMove SANS faire de "if" dessus.
            // On se fiche de son booléen de retour, on veut juste qu'il remplisse moveAdversaire.
            game.getMove(moveAdversaire);

            // 1. On met à jour notre grille locale si l'adversaire a joué un vrai coup (>= 0)
            if (moveAdversaire.row >= 0 && moveAdversaire.col >= 0 && moveAdversaire.row < 3 && moveAdversaire.col < 3) {
                // On vérifie que la case est vide pour éviter de re-traiter un coup
                if (grilleLocale[moveAdversaire.row][moveAdversaire.col] == ' ') {
                    grilleLocale[moveAdversaire.row][moveAdversaire.col] = 'X';
                    cout << "L'adversaire a joue : Ligne " << moveAdversaire.row << ", Colonne " << moveAdversaire.col << "\n";
                }
            }

            // 2. Sécurité : le jeu peut se terminer pile après le coup de l'adversaire
            if (game.isFinish()) break;

            // 3. C'est à notre tour : on calcule notre coup avec notre grille locale à jour
            GameMove monCoup = calculerCoupIA(grilleLocale);

            // 4. On enregistre notre coup et on l'envoie à la librairie
            if (monCoup.row != -1 && monCoup.col != -1) {
                grilleLocale[monCoup.row][monCoup.col] = 'O';
                cout << "Mon IA joue : Ligne " << monCoup.row << ", Colonne " << monCoup.col << "\n";

                // setMove envoie le coup. La librairie va le traiter, faire jouer
                // l'IA adverse en interne, puis nous rendre la main pour le prochain tour.
                game.setMove(monCoup);
            } else {
                // Sécurité si calculerCoupIA ne trouve plus de case vide
                break;
            }
        }
    }

    return 0;
}
