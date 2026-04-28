#include "Game.h"
#include <iostream>

// =====================================================================
// Game.cpp
// =====================================================================

Game::Game() : m_activeSubGrid(-1)
{
    reset();
}

void Game::reset()
{
    for (auto& sb : m_subBoards) sb.reset();
    m_activeSubGrid = -1; // Premier coup : n'importe ou
}

bool Game::isLegalMove(const Move& move, CellState player) const
{
    if (!move.isValid()) return false;
    if (player == CellState::EMPTY) return false;

    int subIdx = subIndex(move.subGridRow(), move.subGridCol());

    // 1) La sous-grille cible doit etre en cours
    if (m_subBoards[subIdx].isFinished()) return false;

    // 2) La case doit etre vide
    if (!m_subBoards[subIdx].isCellEmpty(move.cellRow(), move.cellCol())) return false;

    // 3) Si une sous-grille est "active" (regle d'envoi), on doit y jouer
    if (m_activeSubGrid != -1 && m_activeSubGrid != subIdx) return false;

    return true;
}

bool Game::playMove(const Move& move, CellState player)
{
    if (!isLegalMove(move, player)) return false;

    int subIdx = subIndex(move.subGridRow(), move.subGridCol());
    bool ok = m_subBoards[subIdx].playMove(move.cellRow(), move.cellCol(), player);
    if (!ok) return false;

    // Determination de la prochaine sous-grille active selon la position
    // jouee DANS la sous-grille (cellRow, cellCol).
    int nextSubIdx = subIndex(move.cellRow(), move.cellCol());

    // Si la sous-grille cible suivante est terminee -> libre choix
    if (m_subBoards[nextSubIdx].isFinished())
        m_activeSubGrid = -1;
    else
        m_activeSubGrid = nextSubIdx;

    return true;
}

std::vector<Move> Game::getLegalMoves() const
{
    std::vector<Move> moves;
    moves.reserve(20);

    auto tryAddSub = [&](int subIdx)
    {
        if (m_subBoards[subIdx].isFinished()) return;
        int subR = subIdx / GRID_SIZE;
        int subC = subIdx % GRID_SIZE;
        for (int r = 0; r < GRID_SIZE; ++r)
        {
            for (int c = 0; c < GRID_SIZE; ++c)
            {
                if (m_subBoards[subIdx].isCellEmpty(r, c))
                {
                    moves.emplace_back(subR * GRID_SIZE + r, subC * GRID_SIZE + c);
                }
            }
        }
    };

    if (m_activeSubGrid == -1)
    {
        for (int i = 0; i < NB_SUBGRIDS; ++i) tryAddSub(i);
    }
    else
    {
        tryAddSub(m_activeSubGrid);
    }

    return moves;
}

bool Game::checkGlobalWin(CellState player) const
{
    // On construit une meta-grille 3x3 ou chaque cellule indique
    // si "player" a gagne la sous-grille correspondante.
    auto won = [&](int idx) -> bool
    {
        SubBoardState s = m_subBoards[idx].getState();
        if (player == CellState::CROSS)  return s == SubBoardState::WON_BY_CROSS;
        if (player == CellState::CIRCLE) return s == SubBoardState::WON_BY_CIRCLE;
        return false;
    };

    // Lignes et colonnes
    for (int i = 0; i < GRID_SIZE; ++i)
    {
        if (won(i*3 + 0) && won(i*3 + 1) && won(i*3 + 2)) return true;
        if (won(0*3 + i) && won(1*3 + i) && won(2*3 + i)) return true;
    }
    // Diagonales
    if (won(0) && won(4) && won(8)) return true;
    if (won(2) && won(4) && won(6)) return true;

    return false;
}

int Game::countSubGridsWon(CellState player) const
{
    int count = 0;
    for (const auto& sb : m_subBoards)
    {
        SubBoardState s = sb.getState();
        if (player == CellState::CROSS  && s == SubBoardState::WON_BY_CROSS)  ++count;
        if (player == CellState::CIRCLE && s == SubBoardState::WON_BY_CIRCLE) ++count;
    }
    return count;
}

bool Game::isGameFinished() const
{
    if (checkGlobalWin(CellState::CROSS))  return true;
    if (checkGlobalWin(CellState::CIRCLE)) return true;

    // Toutes les sous-grilles terminees ?
    for (const auto& sb : m_subBoards)
        if (!sb.isFinished()) return false;
    return true;
}

CellState Game::getGlobalWinner() const
{
    if (checkGlobalWin(CellState::CROSS))  return CellState::CROSS;
    if (checkGlobalWin(CellState::CIRCLE)) return CellState::CIRCLE;

    // Si toutes les sous-grilles sont terminees sans alignement,
    // le vainqueur est celui avec le plus de sous-grilles gagnees.
    bool allDone = true;
    for (const auto& sb : m_subBoards)
        if (!sb.isFinished()) { allDone = false; break; }

    if (allDone)
    {
        int x = countSubGridsWon(CellState::CROSS);
        int o = countSubGridsWon(CellState::CIRCLE);
        if (x > o) return CellState::CROSS;
        if (o > x) return CellState::CIRCLE;
    }
    return CellState::EMPTY; // Egalite ou pas fini
}

void Game::print() const
{
    std::cout << "Active sub-grid: " << m_activeSubGrid << "\n";
    for (int gridR = 0; gridR < GRID_SIZE; ++gridR)
    {
        for (int cellR = 0; cellR < GRID_SIZE; ++cellR)
        {
            for (int gridC = 0; gridC < GRID_SIZE; ++gridC)
            {
                int subIdx = gridR * GRID_SIZE + gridC;
                for (int cellC = 0; cellC < GRID_SIZE; ++cellC)
                {
                    char ch = '.';
                    CellState s = m_subBoards[subIdx].getCell(cellR, cellC);
                    if (s == CellState::CROSS)  ch = 'X';
                    if (s == CellState::CIRCLE) ch = 'O';
                    std::cout << ch << ' ';
                }
                std::cout << "| ";
            }
            std::cout << "\n";
        }
        std::cout << "------+-------+-------\n";
    }
}
