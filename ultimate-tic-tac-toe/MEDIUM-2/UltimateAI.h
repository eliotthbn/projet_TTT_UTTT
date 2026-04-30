#ifndef ULTIMATE_AI_H_INCLUDED
#define ULTIMATE_AI_H_INCLUDED

#include "main.h"
#include <array>
#include <cstdint>
#include <chrono>
#include <vector>
#include <memory>
#include <random>

/**
 * @file   UltimateAI.h
 * @brief  IA pour Ultimate Tic-Tac-Toe basée sur Monte Carlo Tree Search.
 *
 *  Choix techniques :
 *  ------------------
 *  - Représentation par BITBOARDS : chaque sous-grille 3x3 tient dans 9 bits
 *    (uint16_t). On a donc 2 joueurs * 9 sous-grilles = 18 entiers 16-bits +
 *    quelques métadonnées. Tout l'état de jeu fait < 64 octets => très friendly
 *    pour le cache CPU et les copies (essentielles dans les simulations MCTS).
 *
 *  - Détection de victoire 3-en-ligne par TABLE DE LOOKUP de 512 entrées
 *    (un masque 9 bits => booléen "est gagnant"). Pré-calculée une seule fois.
 *
 *  - MCTS classique avec ses 4 phases :
 *      1. Sélection  : descente dans l'arbre via la formule UCB1
 *      2. Expansion  : ajout d'un fils pour un coup non encore exploré
 *      3. Simulation : "light playout" pseudo-aléatoire (avec une heuristique :
 *                      si un coup gagne la partie => on le joue)
 *      4. Backprop   : remontée des scores le long du chemin
 *
 *  - L'IA est "agnostique" sur son identité de joueur : elle se contente de
 *    calculer le meilleur coup pour le joueur dont c'est le tour. Le détecteur
 *    isEmpty() permet à main.cpp de savoir qu'on commence si la grille est
 *    vide au moment de jouer.
 */
class UltimateAI {
public:
    UltimateAI();

    // -- Cycle de vie --
    void reset();                         ///< À appeler entre chaque partie.
    bool isEmpty() const;                 ///< true si aucun coup n'a été joué.

    // -- Interaction avec le moteur de jeu --
    void     applyMove(const GameMove& m); ///< Applique un coup (adverse OU notre).
    GameMove computeMove();                ///< Lance MCTS et retourne le coup choisi.

    /// Vrai si la case (row,col) est déjà occupée dans notre état interne.
    /// Sert de garde-fou pour détecter une désynchronisation avec le moteur.
    bool     isCellOccupied(const GameMove& m) const;

    // -- Configuration --
    void setThinkingTime(int ms) { m_thinkingTimeMs = ms; }

private:
    // ============================================================
    //  ÉTAT DU JEU (compact, ~50 octets)
    // ============================================================
    struct State {
        // boards[p][b] = masque 9 bits des cases du joueur p dans la sous-grille b
        std::array<std::array<uint16_t, 9>, 2> boards;
        // metaBoard[p] = masque 9 bits des sous-grilles GAGNÉES par le joueur p
        std::array<uint16_t, 2> metaBoard;
        // Masque 9 bits des sous-grilles "fermées" (gagnées par qqn ou pleines)
        uint16_t finishedBoards;
        // Sous-grille où le prochain joueur DOIT jouer (-1 = libre)
        int8_t   forcedBoard;
        // Joueur qui doit jouer (0 ou 1)
        int8_t   playerToMove;

        State() noexcept;
    };

    // ============================================================
    //  COUP : indice de sous-grille (0..8) + indice de case (0..8)
    // ============================================================
    struct Move {
        uint8_t board;
        uint8_t cell;
        Move() noexcept : board(0), cell(0) {}
        Move(uint8_t b, uint8_t c) noexcept : board(b), cell(c) {}
    };

    // ============================================================
    //  NŒUD DE L'ARBRE MCTS
    // ============================================================
    struct Node {
        Node*   parent;
        Move    move;              ///< Coup ayant mené à ce nœud.
        int8_t  playerJustMoved;   ///< Joueur qui a joué `move` (0 ou 1).
        std::vector<std::unique_ptr<Node>> children;
        std::vector<Move> untriedMoves;
        double  totalScore;        ///< Cumul des résultats du POV de playerJustMoved.
        int     visits;

        Node(Node* p, Move m, int8_t pjm) noexcept
            : parent(p), move(m), playerJustMoved(pjm),
              totalScore(0.0), visits(0) {}
    };

    // ============================================================
    //  MEMBRES
    // ============================================================
    State m_state;
    int   m_thinkingTimeMs;
    std::mt19937 m_rng;
    std::vector<Move> m_simBuffer;   ///< Buffer réutilisé en simulation (évite des allocs).

    // Table 9 bits -> "configuration gagnante ?", calculée une seule fois.
    static std::array<bool, 512> s_winLookup;
    static bool s_winLookupInitialized;
    static void initWinLookup();

    // ============================================================
    //  LOGIQUE DU JEU
    // ============================================================
    static Move     toMove(const GameMove& gm) noexcept;
    static GameMove toGameMove(const Move& m)  noexcept;

    void applyMoveTo(State& s, const Move& m) const noexcept;
    void getLegalMoves(const State& s, std::vector<Move>& out) const;

    /// Renvoie 0 / 1 / 2 (égalité) / -1 (partie en cours).
    int  checkWinner(const State& s) const noexcept;

    // ============================================================
    //  MCTS
    // ============================================================
    Node* selectBestUCB(Node* node, double C) const;
    int   simulate(State s);
    void  backpropagate(Node* node, int winner);
    Move  runMCTS(const State& root);
};

#endif // ULTIMATE_AI_H_INCLUDED
