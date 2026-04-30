#include <iostream>
#include "main.h"
#include "UltimateAI.h"

/**
 * Boucle de jeu :
 *  - Pour chaque partie (boucle externe) on remet l'état interne à zéro.
 *  - À chaque demi-tour (boucle interne) :
 *      1) on récupère le coup de l'adversaire via game.getMove()
 *      2) on l'applique à notre état interne (sauf si c'est NOTRE 1er coup)
 *      3) on calcule notre réponse avec MCTS
 *      4) on l'applique à notre état interne et on l'envoie via game.setMove()
 *
 *  IMPORTANT — Lecture du coup adverse :
 *  -------------------------------------
 *  La VALEUR DE RETOUR de game.getMove() s'est révélée NON FIABLE pendant
 *  les tests : elle peut renvoyer false alors que oppMove a bien été
 *  renseigné par le moteur. On l'IGNORE donc volontairement.
 *
 *  Pour distinguer "le moteur m'a donné un coup adverse" de
 *  "le moteur ne m'a rien donné (à moi de jouer en premier)", on initialise
 *  oppMove à un SENTINEL (-1,-1) avant l'appel : si après l'appel les
 *  coordonnées sont toujours hors plage, c'est qu'aucun coup n'est arrivé.
 */
int main()
{
    UltimateAI ai;

    // Budget de réflexion par coup (ms). Les timeouts ayant été retirés du sujet,
    // on peut monter à 500-1000 ms pour les niveaux difficiles. 200 ms suffit
    // largement pour battre EASY/MEDIUM tout en gardant une exécution rapide.
    ai.setThinkingTime(200);

    // === Configuration de la partie ===
    // - DEBUG : utile en développement (timeouts non bloquants, choix premier/second)
    // - ARENA : SEUL mode validant un niveau (100 parties, 50/50 premier/second)
    game.initialize(100, Level::MEDIUM_2, Mode::ARENA, false, "MCTS_AI");

    // ===== Boucle externe : une itération = une partie =====
    while (!game.isAllGameFinish())
    {
        ai.reset();  // état interne neuf pour la nouvelle partie

        // ===== Boucle interne : une itération = un demi-tour =====
        while (!game.isFinish())
        {
            // ---- 1) Récupération du coup adverse ----
            //
            // On initialise un SENTINEL pour détecter si la struct a été
            // remplie par le moteur. On NE consulte PAS la valeur de retour
            // de getMove() — celle-ci s'est avérée renvoyer false même
            // quand un coup adverse valide est fourni dans oppMove.
            GameMove oppMove;
            oppMove.row = -1;
            oppMove.col = -1;

            game.getMove(oppMove);   // valeur de retour ignorée (cf. commentaire)

            const bool oppValid =
                oppMove.row >= 0 && oppMove.col >= 0
                && oppMove.row <  9 && oppMove.col <  9;

            // ---- 2) Synchronisation de l'état interne ----
            if (oppValid) {
                // Garde-fou : si le moteur nous renvoie un coup déjà connu de
                // notre état (cas très improbable mais possible si l'API change),
                // on évite de l'appliquer deux fois.
                if (ai.isCellOccupied(oppMove)) {
                    std::cerr << "[WARN] coup deja present, ignore : ("
                              << oppMove.row << "," << oppMove.col << ")\n";
                } else {
                    ai.applyMove(oppMove);
                    std::cerr << "[OPP] (" << oppMove.row << "," << oppMove.col << ")\n";
                }
            } else if (ai.isEmpty()) {
                // Grille interne vide + aucun coup adverse fourni  =>  on ouvre la partie
                std::cerr << "[INFO] On commence la partie.\n";
            }
            // (autre cas : pas de coup adverse mais grille non vide)
            //  -> ne devrait pas arriver ; on laisse le MCTS répondre comme si
            //     c'était à nous de jouer (l'état interne reste cohérent)

            // ---- 3) Calcul de notre coup (MCTS) ----
            GameMove myMove = ai.computeMove();

            // ---- 4) Mise à jour interne + envoi au moteur ----
            ai.applyMove(myMove);
            game.setMove(myMove);

            std::cerr << "[ME ] (" << myMove.row << "," << myMove.col << ")\n";
        }

        // Bilan de la partie
        std::cerr << "----- Fin de partie : ";
        switch (game.getWinner()) {
            case Winner::PLAYER:        std::cerr << "Victoire";      break;
            case Winner::IA:            std::cerr << "Defaite";       break;
            case Winner::IA_AND_PLAYER: std::cerr << "Egalite";       break;
            default:                    std::cerr << "Indetermine";   break;
        }
        std::cerr << " -----\n\n";
    }

    return 0;
}
