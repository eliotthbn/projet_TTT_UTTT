#include "game.h"
#include <algorithm>
#include <cassert>
#include <iostream>

// ============================================================
//  Point d'entrée global requis par main.h
// ============================================================
static Game g_game;
IGame& game = g_game;

// ============================================================
//  Helpers statiques
// ============================================================

int Game::gridIndex(int row, int col) {
    // Petite grille : ligne majeure * 3 + colonne majeure
    return (row / BOARD_SIZE) * BOARD_SIZE + (col / BOARD_SIZE);
}

int Game::localRow(int row) { return row % BOARD_SIZE; }
int Game::localCol(int col) { return col % BOARD_SIZE; }

// ============================================================
//  Vérifie si un joueur (AI ou HUMAN) a aligné 3 symboles
//  dans un tableau 1D de 9 cases représentant une grille 3x3.
// ============================================================
static bool hasWon(const std::array<int,9>& g, int player) {
    // Lignes
    for (int r = 0; r < 3; ++r)
        if (g[r*3]==player && g[r*3+1]==player && g[r*3+2]==player) return true;
    // Colonnes
    for (int c = 0; c < 3; ++c)
        if (g[c]==player && g[3+c]==player && g[6+c]==player) return true;
    // Diagonales
    if (g[0]==player && g[4]==player && g[8]==player) return true;
    if (g[2]==player && g[4]==player && g[6]==player) return true;
    return false;
}

// ============================================================
//  Vérifie le résultat d'une petite grille
//  Retourne AI, HUMAN, 2 (nulle) ou EMPTY (en cours)
// ============================================================
int Game::checkMiniGrid(const State& s, int gridIdx) {
    int br = (gridIdx / 3) * 3;   // première ligne globale de cette grille
    int bc = (gridIdx % 3) * 3;   // première colonne globale

    std::array<int,9> cells{};
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            cells[r*3+c] = s.board[br+r][bc+c];

    if (hasWon(cells, AI))    return AI;
    if (hasWon(cells, HUMAN)) return HUMAN;

    // Nulle si toutes les cases sont remplies
    bool full = true;
    for (int v : cells) if (v == EMPTY) { full = false; break; }
    if (full) return 2;

    return EMPTY;
}

// ============================================================
//  Vérifie la macro-grille
// ============================================================
int Game::checkMacroGrid(const State& s) {
    if (hasWon({ s.miniResult[0], s.miniResult[1], s.miniResult[2],
                 s.miniResult[3], s.miniResult[4], s.miniResult[5],
                 s.miniResult[6], s.miniResult[7], s.miniResult[8] }, AI))
        return AI;
    if (hasWon({ s.miniResult[0], s.miniResult[1], s.miniResult[2],
                 s.miniResult[3], s.miniResult[4], s.miniResult[5],
                 s.miniResult[6], s.miniResult[7], s.miniResult[8] }, HUMAN))
        return HUMAN;
    return EMPTY;
}

// ============================================================
//  Génère la liste des coups légaux
// ============================================================
std::vector<GameMove> Game::legalMoves(const State& s) {
    std::vector<GameMove> moves;
    moves.reserve(81);

    auto addMovesInGrid = [&](int gIdx) {
        int br = (gIdx / 3) * 3;
        int bc = (gIdx % 3) * 3;
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                if (s.board[br+r][bc+c] == EMPTY)
                    moves.push_back({br+r, bc+c});
    };

    if (s.forcedGrid >= 0 && s.miniResult[s.forcedGrid] == EMPTY) {
        // Cas normal : jouer dans la grille forcée
        addMovesInGrid(s.forcedGrid);
    } else {
        // Grille forcée terminée ou pas de contrainte : toutes les grilles libres
        for (int g = 0; g < 9; ++g)
            if (s.miniResult[g] == EMPTY)
                addMovesInGrid(g);
    }

    return moves;
}

// ============================================================
//  Applique un coup et retourne le nouvel état
// ============================================================
State Game::applyMove(const State& s, const GameMove& mv) {
    State ns = s;
    ns.board[mv.row][mv.col] = s.currentPlayer;

    // Met à jour le résultat de la petite grille concernée
    int gIdx = gridIndex(mv.row, mv.col);
    if (ns.miniResult[gIdx] == EMPTY) {
        int res = checkMiniGrid(ns, gIdx);
        if (res != EMPTY) ns.miniResult[gIdx] = res;
    }

    // La prochaine grille est déterminée par la position locale du coup
    int nextGrid = localRow(mv.row) * 3 + localCol(mv.col);
    ns.forcedGrid = (ns.miniResult[nextGrid] == EMPTY) ? nextGrid : -1;

    // Change le joueur
    ns.currentPlayer = -s.currentPlayer;

    return ns;
}

// ============================================================
//  Score d'une ligne de 3 cases pour l'heuristique
// ============================================================
int Game::lineScore(int a, int b, int c) {
    int aiCount    = (a==AI)    + (b==AI)    + (c==AI);
    int humanCount = (a==HUMAN) + (b==HUMAN) + (c==HUMAN);

    if (aiCount > 0 && humanCount > 0) return 0; // ligne bloquée
    if (aiCount == 3) return 100;
    if (humanCount == 3) return -100;
    if (aiCount == 2) return 10;
    if (humanCount == 2) return -10;
    if (aiCount == 1) return 1;
    if (humanCount == 1) return -1;
    return 0;
}

// ============================================================
//  Évaluation heuristique de la position
//  > 0 : avantage IA, < 0 : avantage joueur
// ============================================================
int Game::evaluate(const State& s) {
    int score = 0;

    // ----- Score sur la macro-grille -----
    const auto& mr = s.miniResult;

    // Lignes macro
    for (int r = 0; r < 3; ++r)
        score += lineScore(mr[r*3], mr[r*3+1], mr[r*3+2]);
    // Colonnes macro
    for (int c = 0; c < 3; ++c)
        score += lineScore(mr[c], mr[3+c], mr[6+c]);
    // Diagonales macro
    score += lineScore(mr[0], mr[4], mr[8]);
    score += lineScore(mr[2], mr[4], mr[6]);

    // Bonus centre macro (la grille centrale est la plus stratégique)
    score += mr[4] * 5;
    // Bonus coins macro
    for (int idx : {0, 2, 6, 8}) score += mr[idx] * 3;

    // ----- Score à l'intérieur des petites grilles encore ouvertes -----
    for (int g = 0; g < 9; ++g) {
        if (mr[g] != EMPTY) continue; // grille terminée, déjà comptée

        int br = (g / 3) * 3;
        int bc = (g % 3) * 3;

        // Poids de cette petite grille selon sa position
        int weight = (g == 4) ? 3 : ((g==0||g==2||g==6||g==8) ? 2 : 1);

        // Lignes locales
        for (int r = 0; r < 3; ++r)
            score += weight * lineScore(s.board[br+r][bc],
                                        s.board[br+r][bc+1],
                                        s.board[br+r][bc+2]);
        // Colonnes locales
        for (int c = 0; c < 3; ++c)
            score += weight * lineScore(s.board[br][bc+c],
                                        s.board[br+1][bc+c],
                                        s.board[br+2][bc+c]);
        // Diagonales locales
        score += weight * lineScore(s.board[br][bc],
                                     s.board[br+1][bc+1],
                                     s.board[br+2][bc+2]);
        score += weight * lineScore(s.board[br][bc+2],
                                     s.board[br+1][bc+1],
                                     s.board[br+2][bc]);

        // Bonus centre local
        score += s.board[br+1][bc+1] * weight;
    }

    return score;
}

// ============================================================
//  Minimax avec élagage Alpha-Beta
// ============================================================
int Game::minimax(State s, int depth, int alpha, int beta, bool maximizing) {
    // Vérifier l'état terminal sur la macro-grille
    int macro = checkMacroGrid(s);
    if (macro == AI)    return  INF + depth; // victoire rapide = meilleure
    if (macro == HUMAN) return -INF - depth;

    auto moves = legalMoves(s);
    if (moves.empty() || depth == 0)
        return evaluate(s);

    if (maximizing) {
        int best = std::numeric_limits<int>::min();
        for (const auto& mv : moves) {
            int val = minimax(applyMove(s, mv), depth-1, alpha, beta, false);
            best  = std::max(best, val);
            alpha = std::max(alpha, best);
            if (alpha >= beta) break; // coupure beta
        }
        return best;
    } else {
        int best = std::numeric_limits<int>::max();
        for (const auto& mv : moves) {
            int val = minimax(applyMove(s, mv), depth-1, alpha, beta, true);
            best  = std::min(best, val);
            beta  = std::min(beta, best);
            if (alpha >= beta) break; // coupure alpha
        }
        return best;
    }
}

// ============================================================
//  Choisit le meilleur coup pour l'IA
// ============================================================
GameMove Game::bestMove(const State& s) {
    auto moves = legalMoves(s);
    assert(!moves.empty());

    // Ordre des coups : centre en premier (améliore l'élagage)
    std::sort(moves.begin(), moves.end(), [](const GameMove& a, const GameMove& b){
        // Priorité : case du centre de la petite grille (coordonnée locale 1,1)
        int ca = (a.row%3==1 && a.col%3==1) ? 0 : 1;
        int cb = (b.row%3==1 && b.col%3==1) ? 0 : 1;
        return ca < cb;
    });

    GameMove best = moves[0];
    int bestVal   = std::numeric_limits<int>::min();

    for (const auto& mv : moves) {
        int val = minimax(applyMove(s, mv), m_depth - 1,
                          std::numeric_limits<int>::min(),
                          std::numeric_limits<int>::max(), false);
        if (val > bestVal) {
            bestVal = val;
            best    = mv;
        }
    }
    return best;
}

// ============================================================
//  Profondeur selon le niveau (timeout non pris en compte)
// ============================================================
int Game::depthForLevel(Level l) const {
    switch (l) {
        case Level::EASY_1:      return 2;
        case Level::EASY_2:      return 3;
        case Level::MEDIUM_1:    return 4;
        case Level::MEDIUM_2:    return 5;
        case Level::HARD_1:      return 6;
        case Level::HARD_2:      return 7;
        case Level::VERY_HARD_1: return 8;
        case Level::VERY_HARD_2: return 9;
        default:                 return 4;
    }
}

// ============================================================
//  Remise à zéro d'une partie
// ============================================================
void Game::resetGame() {
    m_state      = State{};
    m_winner     = NO_WINNER;
    m_finished   = false;

    // En mode Arena, l'IA joue tantôt en premier tantôt en second
    // On alterne simplement à chaque partie
    m_aiFirst = m_alwaysFirst || (m_gamesDone % 2 == 0);

    // Si l'IA joue en second, le joueur commence => currentPlayer = HUMAN
    if (!m_aiFirst)
        m_state.currentPlayer = HUMAN;
    else
        m_state.currentPlayer = AI;
}

// ============================================================
//  IGame::initialize
// ============================================================
void Game::initialize(unsigned int nbGame, Level level, Mode mode,
                      bool alwaysPlayFirst, const std::string& alias) {
    m_nbGame      = nbGame;
    m_level       = level;
    m_mode        = mode;
    m_alwaysFirst = alwaysPlayFirst;
    m_alias       = alias;
    m_gamesDone   = 0;
    m_depth       = depthForLevel(level);

    resetGame();
}

// ============================================================
//  IGame::getWinner
// ============================================================
Winner Game::getWinner() const {
    return m_winner;
}

// ============================================================
//  IGame::isFinish  — appelé AVANT getMove / setMove
//  Met aussi à jour m_winner et m_finished
// ============================================================
bool Game::isFinish() {
    if (m_finished) return true;

    int macro = checkMacroGrid(m_state);
    if (macro == AI) {
        m_winner   = IA;
        m_finished = true;
        return true;
    }
    if (macro == HUMAN) {
        m_winner   = PLAYER;
        m_finished = true;
        return true;
    }

    // Plus aucun coup légal ?
    if (legalMoves(m_state).empty()) {
        // Compter les petites grilles gagnées
        int aiCount = 0, humanCount = 0;
        for (int v : m_state.miniResult) {
            if (v == AI)    ++aiCount;
            if (v == HUMAN) ++humanCount;
        }
        if (aiCount > humanCount)       m_winner = IA;
        else if (humanCount > aiCount)  m_winner = PLAYER;
        else                            m_winner = IA_AND_PLAYER;
        m_finished = true;
        return true;
    }

    return false;
}

// ============================================================
//  IGame::isAllGameFinish
// ============================================================
bool Game::isAllGameFinish() const {
    return m_gamesDone >= m_nbGame;
}

// ============================================================
//  IGame::getMove  — l'IA calcule et joue son coup
//  Retourne false si ce n'est pas le tour de l'IA
// ============================================================
bool Game::getMove(GameMove& outMove) {
    if (m_state.currentPlayer != AI) return false;

    outMove = bestMove(m_state);

    // Applique le coup dans l'état interne
    m_state = applyMove(m_state, outMove);

    return true;
}

// ============================================================
//  IGame::setMove  — le joueur humain joue son coup
// ============================================================
void Game::setMove(const GameMove& move) {
    // Vérifie que c'est bien le tour du joueur
    if (m_state.currentPlayer != HUMAN) return;

    // Applique le coup
    m_state = applyMove(m_state, move);

    // Si la partie est terminée, on passe à la suivante
    if (isFinish()) {
        ++m_gamesDone;
        if (m_gamesDone < m_nbGame)
            resetGame();
    }
}
