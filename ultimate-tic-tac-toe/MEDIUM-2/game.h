#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include "main.h"
#include <array>
#include <vector>
#include <limits>

// ============================================================
//  Constantes
// ============================================================
static constexpr int BOARD_SIZE   = 3;   // taille d'une petite grille
static constexpr int GRID_COUNT   = 9;   // nombre de petites grilles
static constexpr int TOTAL_SIZE   = 9;   // largeur/hauteur totale (3*3)

// Valeurs des cellules
static constexpr int EMPTY  =  0;
static constexpr int AI     =  1;   // IA (croix ou rond selon le tour)
static constexpr int HUMAN  = -1;   // Joueur humain

// Score de la fonction d'évaluation
static constexpr int INF    = 1'000'000;

// ============================================================
//  Structure représentant l'état complet du jeu
// ============================================================
struct State {
    // board[row][col] : EMPTY / AI / HUMAN  (coordonnées 0..8 x 0..8)
    std::array<std::array<int, TOTAL_SIZE>, TOTAL_SIZE> board{};

    // Résultat de chaque petite grille : EMPTY / AI / HUMAN / 2 (nulle)
    std::array<int, GRID_COUNT> miniResult{};

    // Quelle petite grille doit jouer le prochain coup (-1 = libre)
    int forcedGrid = -1;

    // Qui joue : AI ou HUMAN
    int currentPlayer = AI;
};

// ============================================================
//  Classe principale du jeu (implémente IGame)
// ============================================================
class Game : public IGame {
public:
    // --- Interface IGame ---
    void initialize(unsigned int nbGame, Level level, Mode mode,
                    bool alwaysPlayFirst, const std::string& alias) override;

    Winner getWinner() const override;
    bool   isFinish() override;
    bool   isAllGameFinish() const override;
    bool   getMove(GameMove& outMove) override;
    void   setMove(const GameMove& move) override;

private:
    // ---- Données de session ----
    unsigned int m_nbGame      = 0;
    unsigned int m_gamesDone   = 0;
    Level        m_level       = Level::EASY_1;
    Mode         m_mode        = Mode::DEBUG;
    bool         m_alwaysFirst = false;
    std::string  m_alias;

    // ---- État courant ----
    State        m_state;
    Winner       m_winner      = NO_WINNER;
    bool         m_finished    = false;

    // ---- IA joue-t-elle en premier dans cette partie ? ----
    bool         m_aiFirst     = true;

    // ---- Profondeur Minimax selon le niveau ----
    int          m_depth       = 4;

    // ========================================================
    //  Méthodes internes
    // ========================================================
    void  resetGame();

    // Calcule l'index de la petite grille (0-8) depuis les coords globales
    static int gridIndex(int row, int col);

    // Coords locales dans la petite grille
    static int localRow(int row);
    static int localCol(int col);

    // Vérifie si une petite grille est gagnée/nulle
    static int  checkMiniGrid(const State& s, int gridIdx);

    // Vérifie la macro-grille (les 9 résultats de miniResult)
    static int  checkMacroGrid(const State& s);

    // Génère tous les coups légaux
    static std::vector<GameMove> legalMoves(const State& s);

    // Applique un coup et retourne le nouvel état
    static State applyMove(const State& s, const GameMove& mv);

    // Fonction d'évaluation statique (heuristique)
    static int evaluate(const State& s);

    // Valeur d'une ligne de 3 cellules pour l'évaluation
    static int lineScore(int a, int b, int c);

    // Minimax avec élagage Alpha-Beta
    static int minimax(State s, int depth, int alpha, int beta, bool maximizing);

    // Choisit le meilleur coup pour l'IA
    GameMove bestMove(const State& s);

    // Profondeur de recherche selon le niveau
    int depthForLevel(Level l) const;
};

#endif // GAME_H_INCLUDED
