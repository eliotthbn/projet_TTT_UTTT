#include "SubBoard.h"

// =====================================================================
// SubBoard.cpp
// =====================================================================

SubBoard::SubBoard()
{
    reset();
}

void SubBoard::reset()
{
    for (int r = 0; r < GRID_SIZE; ++r)
        for (int c = 0; c < GRID_SIZE; ++c)
            m_cells[r][c] = CellState::EMPTY;
    m_state = SubBoardState::IN_PROGRESS;
}

CellState SubBoard::getCell(int r, int c) const
{
    if (r < 0 || r >= GRID_SIZE || c < 0 || c >= GRID_SIZE)
        return CellState::EMPTY;
    return m_cells[r][c];
}

bool SubBoard::isCellEmpty(int r, int c) const
{
    return getCell(r, c) == CellState::EMPTY;
}

bool SubBoard::isFinished() const
{
    return m_state != SubBoardState::IN_PROGRESS;
}

bool SubBoard::playMove(int r, int c, CellState player)
{
    // Securite : on n'accepte un coup que sur une case vide
    // d'une sous-grille pas encore terminee
    if (isFinished()) return false;
    if (r < 0 || r >= GRID_SIZE || c < 0 || c >= GRID_SIZE) return false;
    if (m_cells[r][c] != CellState::EMPTY) return false;
    if (player == CellState::EMPTY) return false;

    m_cells[r][c] = player;
    updateState();
    return true;
}

bool SubBoard::checkWin(CellState player) const
{
    // Lignes et colonnes
    for (int i = 0; i < GRID_SIZE; ++i)
    {
        if (m_cells[i][0] == player && m_cells[i][1] == player && m_cells[i][2] == player) return true;
        if (m_cells[0][i] == player && m_cells[1][i] == player && m_cells[2][i] == player) return true;
    }
    // Diagonales
    if (m_cells[0][0] == player && m_cells[1][1] == player && m_cells[2][2] == player) return true;
    if (m_cells[0][2] == player && m_cells[1][1] == player && m_cells[2][0] == player) return true;

    return false;
}

void SubBoard::updateState()
{
    if (checkWin(CellState::CROSS))       { m_state = SubBoardState::WON_BY_CROSS;  return; }
    if (checkWin(CellState::CIRCLE))      { m_state = SubBoardState::WON_BY_CIRCLE; return; }

    // Verifie si pleine -> match nul
    for (int r = 0; r < GRID_SIZE; ++r)
        for (int c = 0; c < GRID_SIZE; ++c)
            if (m_cells[r][c] == CellState::EMPTY) return; // Encore en cours

    m_state = SubBoardState::DRAW;
}
