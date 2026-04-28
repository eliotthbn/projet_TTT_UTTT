#include "HeuristicPlayer.h"
#include <algorithm>
#include <limits>

// =====================================================================
// HeuristicPlayer.cpp
// =====================================================================

// --- Constantes d'evaluation ---
// Tres grandes valeurs pour la victoire / defaite globale
static constexpr int WIN_SCORE  = 1000000;
static constexpr int LOSE_SCORE = -1000000;

// Poids des sous-grilles selon leur position sur la meta-grille
// (centre > coins > bords). Bien connu en Ultimate TTT.
//   0 1 2
//   3 4 5
//   6 7 8
static const int SUBGRID_POSITION_WEIGHT[9] = {
    4, 3, 4,
    3, 5, 3,
    4, 3, 4
};

// Poids des cellules dans une sous-grille 3x3 (idem)
static const int CELL_POSITION_WEIGHT[3][3] = {
    {3, 2, 3},
    {2, 4, 2},
    {3, 2, 3}
};

HeuristicPlayer::HeuristicPlayer(int searchDepth)
    : m_depth(searchDepth), m_me(CellState::EMPTY), m_opp(CellState::EMPTY)
{}

// -----------------------------------------------------------------
// Score d'une ligne (3 cellules) :
//   - 3 a moi : tres bon
//   - 2 a moi + 0 adverse : menace
//   - 1 a moi + 0 adverse : leger avantage
//   - mixte : 0
//   - symetrique pour l'adversaire (negatif)
// -----------------------------------------------------------------
int HeuristicPlayer::scoreLine(int myCount, int oppCount)
{
    if (myCount > 0 && oppCount > 0) return 0;
    if (myCount == 3) return 100;
    if (myCount == 2) return 10;
    if (myCount == 1) return 1;
    if (oppCount == 3) return -100;
    if (oppCount == 2) return -10;
    if (oppCount == 1) return -1;
    return 0;
}

// -----------------------------------------------------------------
// Evaluation d'une sous-grille 3x3 du point de vue de "player".
// Cumule le score des 8 lignes possibles (3 H + 3 V + 2 D).
// -----------------------------------------------------------------
int HeuristicPlayer::evaluateSubGrid(const SubBoard& sb, CellState player) const
{
    SubBoardState st = sb.getState();
    if (player == CellState::CROSS)
    {
        if (st == SubBoardState::WON_BY_CROSS)  return 150;
        if (st == SubBoardState::WON_BY_CIRCLE) return -150;
    }
    else if (player == CellState::CIRCLE)
    {
        if (st == SubBoardState::WON_BY_CIRCLE) return 150;
        if (st == SubBoardState::WON_BY_CROSS)  return -150;
    }
    if (st == SubBoardState::DRAW) return 0;

    CellState opp = opponent(player);
    int score = 0;

    // Construction d'une matrice locale
    CellState g[3][3];
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            g[r][c] = sb.getCell(r, c);

    auto countLine = [&](int r1,int c1,int r2,int c2,int r3,int c3)
    {
        int my = 0, op = 0;
        CellState a = g[r1][c1], b = g[r2][c2], c = g[r3][c3];
        if (a == player) my++; else if (a == opp) op++;
        if (b == player) my++; else if (b == opp) op++;
        if (c == player) my++; else if (c == opp) op++;
        score += scoreLine(my, op);
    };

    // Lignes
    for (int r = 0; r < 3; ++r) countLine(r,0, r,1, r,2);
    // Colonnes
    for (int c = 0; c < 3; ++c) countLine(0,c, 1,c, 2,c);
    // Diagonales
    countLine(0,0, 1,1, 2,2);
    countLine(0,2, 1,1, 2,0);

    // Bonus : valeur des cellules occupees
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
        {
            if (g[r][c] == player)  score += CELL_POSITION_WEIGHT[r][c];
            else if (g[r][c] == opp) score -= CELL_POSITION_WEIGHT[r][c];
        }

    return score;
}

// -----------------------------------------------------------------
// Evaluation globale du plateau du point de vue de m_me.
// -----------------------------------------------------------------
int HeuristicPlayer::evaluate(const Game& g) const
{
    // 1) Si quelqu'un a gagne globalement
    CellState w = g.getGlobalWinner();
    if (w == m_me)  return WIN_SCORE;
    if (w == m_opp) return LOSE_SCORE;

    int score = 0;

    // 2) Score de la meta-grille (lignes/colonnes/diags de sous-grilles gagnees)
    auto subWonBy = [&](int idx, CellState player) -> bool
    {
        SubBoardState s = g.getSubBoard(idx).getState();
        if (player == CellState::CROSS)  return s == SubBoardState::WON_BY_CROSS;
        if (player == CellState::CIRCLE) return s == SubBoardState::WON_BY_CIRCLE;
        return false;
    };

    auto countMetaLine = [&](int a, int b, int c)
    {
        int my = 0, op = 0;
        if (subWonBy(a, m_me))  my++; else if (subWonBy(a, m_opp)) op++;
        if (subWonBy(b, m_me))  my++; else if (subWonBy(b, m_opp)) op++;
        if (subWonBy(c, m_me))  my++; else if (subWonBy(c, m_opp)) op++;
        // Lignes meta plus importantes : on multiplie
        score += scoreLine(my, op) * 50;
    };

    // 3 lignes
    countMetaLine(0,1,2);
    countMetaLine(3,4,5);
    countMetaLine(6,7,8);
    // 3 colonnes
    countMetaLine(0,3,6);
    countMetaLine(1,4,7);
    countMetaLine(2,5,8);
    // 2 diagonales
    countMetaLine(0,4,8);
    countMetaLine(2,4,6);

    // 3) Bonus pour chaque sous-grille gagnee, ponderee par sa position
    for (int i = 0; i < NB_SUBGRIDS; ++i)
    {
        SubBoardState s = g.getSubBoard(i).getState();
        if ((m_me == CellState::CROSS  && s == SubBoardState::WON_BY_CROSS) ||
            (m_me == CellState::CIRCLE && s == SubBoardState::WON_BY_CIRCLE))
            score += 30 * SUBGRID_POSITION_WEIGHT[i];
        else if ((m_opp == CellState::CROSS  && s == SubBoardState::WON_BY_CROSS) ||
                 (m_opp == CellState::CIRCLE && s == SubBoardState::WON_BY_CIRCLE))
            score -= 30 * SUBGRID_POSITION_WEIGHT[i];
    }

    // 4) Pour chaque sous-grille en cours, ajouter le score local
    //    pondere par l'importance de la sous-grille
    for (int i = 0; i < NB_SUBGRIDS; ++i)
    {
        if (g.getSubBoard(i).getState() == SubBoardState::IN_PROGRESS)
        {
            int local = evaluateSubGrid(g.getSubBoard(i), m_me);
            score += local * SUBGRID_POSITION_WEIGHT[i] / 5;
        }
    }

    return score;
}

// -----------------------------------------------------------------
// Alpha-Beta classique avec elagage.
// On joue en simulation directement sur l'objet Game (copie).
// -----------------------------------------------------------------
int HeuristicPlayer::alphaBeta(Game& g, int depth, int alpha, int beta,
                                bool maximizing, CellState toPlay)
{
    // Conditions terminales
    if (g.isGameFinished())
    {
        CellState w = g.getGlobalWinner();
        if (w == m_me)  return WIN_SCORE - (m_depth - depth);  // gagne vite > tard
        if (w == m_opp) return LOSE_SCORE + (m_depth - depth); // perd tard > vite
        return 0; // egalite
    }
    if (depth == 0) return evaluate(g);

    std::vector<Move> moves = g.getLegalMoves();
    if (moves.empty()) return evaluate(g);

    if (maximizing)
    {
        int best = std::numeric_limits<int>::min();
        for (const Move& m : moves)
        {
            Game copy = g;
            if (!copy.playMove(m, toPlay)) continue;
            int v = alphaBeta(copy, depth - 1, alpha, beta, false, opponent(toPlay));
            if (v > best) best = v;
            if (best > alpha) alpha = best;
            if (beta <= alpha) break; // elagage beta
        }
        return best;
    }
    else
    {
        int best = std::numeric_limits<int>::max();
        for (const Move& m : moves)
        {
            Game copy = g;
            if (!copy.playMove(m, toPlay)) continue;
            int v = alphaBeta(copy, depth - 1, alpha, beta, true, opponent(toPlay));
            if (v < best) best = v;
            if (best < beta) beta = best;
            if (beta <= alpha) break; // elagage alpha
        }
        return best;
    }
}

// -----------------------------------------------------------------
// Choisit le meilleur coup en testant chaque coup legal et en
// evaluant l'etat resultant via alphaBeta.
// -----------------------------------------------------------------
Move HeuristicPlayer::chooseMove(const Game& game, CellState mySymbol)
{
    m_me = mySymbol;
    m_opp = opponent(mySymbol);

    std::vector<Move> moves = game.getLegalMoves();
    if (moves.empty()) return Move(-1, -1);

    // Adapter la profondeur si peu de coups : on peut chercher plus loin
    int dynamicDepth = m_depth;
    if (moves.size() <= 9)  dynamicDepth = std::max(m_depth, 6);
    if (moves.size() <= 4)  dynamicDepth = std::max(m_depth, 8);

    int bestScore = std::numeric_limits<int>::min();
    Move bestMove = moves[0];

    int alpha = std::numeric_limits<int>::min();
    int beta  = std::numeric_limits<int>::max();

    for (const Move& mv : moves)
    {
        Game copy = game;
        if (!copy.playMove(mv, m_me)) continue;
        int score = alphaBeta(copy, dynamicDepth - 1, alpha, beta, false, m_opp);

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = mv;
        }
        if (bestScore > alpha) alpha = bestScore;
    }

    return bestMove;
}
