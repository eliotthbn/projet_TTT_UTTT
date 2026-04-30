#include "UltimateAI.h"
#include <cmath>
#include <iostream>

// ============================================================================
//  CONSTANTES INTERNES
// ============================================================================
namespace {

// Les 8 alignements gagnants sur une grille 3x3 (cases numérotées 0..8 row-major)
constexpr std::array<uint16_t, 8> WIN_PATTERNS = {
    0b000000111, 0b000111000, 0b111000000, // lignes
    0b001001001, 0b010010010, 0b100100100, // colonnes
    0b100010001, 0b001010100               // diagonales
};

constexpr uint16_t FULL_MASK = 0b111111111;   // 9 cases occupées
constexpr double   UCB1_C    = 1.41421356;    // sqrt(2) – constante d'exploration

// popcount portable (utilise __builtin_popcount sur GCC, fallback Brian Kernighan ailleurs)
inline int popcount9(uint16_t x) noexcept {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(x);
#else
    int c = 0;
    while (x) { x &= x - 1; ++c; }
    return c;
#endif
}

} // namespace anonyme

// ============================================================================
//  TABLE DE LOOKUP STATIQUE
// ============================================================================
std::array<bool, 512> UltimateAI::s_winLookup{};
bool UltimateAI::s_winLookupInitialized = false;

void UltimateAI::initWinLookup() {
    if (s_winLookupInitialized) return;
    for (uint16_t mask = 0; mask < 512; ++mask) {
        bool win = false;
        for (uint16_t pat : WIN_PATTERNS) {
            if ((mask & pat) == pat) { win = true; break; }
        }
        s_winLookup[mask] = win;
    }
    s_winLookupInitialized = true;
}

// ============================================================================
//  CONSTRUCTEURS
// ============================================================================
UltimateAI::State::State() noexcept
    : boards{}, metaBoard{0, 0}, finishedBoards(0),
      forcedBoard(-1), playerToMove(0)
{}

UltimateAI::UltimateAI()
    : m_thinkingTimeMs(200),
      m_rng(std::random_device{}())
{
    initWinLookup();
    m_simBuffer.reserve(81);
}

// ============================================================================
//  CYCLE DE VIE
// ============================================================================
void UltimateAI::reset() {
    m_state = State{};
}

bool UltimateAI::isEmpty() const {
    if (m_state.metaBoard[0] | m_state.metaBoard[1]) return false;
    for (int i = 0; i < 9; ++i)
        if (m_state.boards[0][i] | m_state.boards[1][i]) return false;
    return true;
}

// ============================================================================
//  CONVERSIONS DE COORDONNÉES
//  Le moteur fournit (row, col) ∈ [0,8]² ; on stocke (board, cell) ∈ [0,8]²
// ============================================================================
UltimateAI::Move UltimateAI::toMove(const GameMove& gm) noexcept {
    uint8_t b = static_cast<uint8_t>((gm.row / 3) * 3 + (gm.col / 3));
    uint8_t c = static_cast<uint8_t>((gm.row % 3) * 3 + (gm.col % 3));
    return Move(b, c);
}

GameMove UltimateAI::toGameMove(const Move& m) noexcept {
    GameMove gm;
    gm.row = (m.board / 3) * 3 + (m.cell / 3);
    gm.col = (m.board % 3) * 3 + (m.cell % 3);
    return gm;
}

// ============================================================================
//  APPLICATION D'UN COUP (mute l'état)
// ============================================================================
void UltimateAI::applyMoveTo(State& s, const Move& m) const noexcept {
    const int p = s.playerToMove;

    // 1) Pose du symbole dans la sous-grille
    s.boards[p][m.board] |= static_cast<uint16_t>(1u << m.cell);

    // 2) Cette sous-grille devient-elle gagnée ?
    if (s_winLookup[s.boards[p][m.board]]) {
        s.metaBoard[p]   |= static_cast<uint16_t>(1u << m.board);
        s.finishedBoards |= static_cast<uint16_t>(1u << m.board);
    }
    // 3) Sinon, est-elle pleine (toutes les cases occupées) ?
    else if ((s.boards[0][m.board] | s.boards[1][m.board]) == FULL_MASK) {
        s.finishedBoards |= static_cast<uint16_t>(1u << m.board);
    }

    // 4) Détermination de la sous-grille suivante
    //    => imposée par la position de la case jouée, sauf si elle est fermée
    const uint8_t next = m.cell;
    s.forcedBoard = (s.finishedBoards & (1u << next))
                    ? static_cast<int8_t>(-1)
                    : static_cast<int8_t>(next);

    // 5) On passe la main
    s.playerToMove = static_cast<int8_t>(1 - p);
}

void UltimateAI::applyMove(const GameMove& move) {
    applyMoveTo(m_state, toMove(move));
}

bool UltimateAI::isCellOccupied(const GameMove& move) const {
    const Move m = toMove(move);
    const uint16_t occ = static_cast<uint16_t>(
        m_state.boards[0][m.board] | m_state.boards[1][m.board]);
    return (occ & (1u << m.cell)) != 0;
}

// ============================================================================
//  GÉNÉRATION DES COUPS LÉGAUX
// ============================================================================
void UltimateAI::getLegalMoves(const State& s, std::vector<Move>& out) const {
    out.clear();

    // Helper : ajoute toutes les cases vides de la sous-grille b
    auto addBoard = [&](int b) {
        const uint16_t occ = static_cast<uint16_t>(s.boards[0][b] | s.boards[1][b]);
        for (int c = 0; c < 9; ++c)
            if (!(occ & (1u << c)))
                out.emplace_back(static_cast<uint8_t>(b), static_cast<uint8_t>(c));
    };

    // Cas 1 : sous-grille forcée et encore active
    if (s.forcedBoard >= 0 && !(s.finishedBoards & (1u << s.forcedBoard))) {
        addBoard(s.forcedBoard);
    }
    // Cas 2 : libre choix sur toutes les sous-grilles non fermées
    else {
        for (int b = 0; b < 9; ++b)
            if (!(s.finishedBoards & (1u << b)))
                addBoard(b);
    }
}

// ============================================================================
//  DÉTECTION DE FIN DE PARTIE
//  0  -> joueur 0 gagne
//  1  -> joueur 1 gagne
//  2  -> égalité
//  -1 -> partie en cours
// ============================================================================
int UltimateAI::checkWinner(const State& s) const noexcept {
    // Victoire absolue : 3 sous-grilles alignées
    if (s_winLookup[s.metaBoard[0]]) return 0;
    if (s_winLookup[s.metaBoard[1]]) return 1;

    // Tant qu'il reste des sous-grilles ouvertes, la partie continue
    if (s.finishedBoards != FULL_MASK) return -1;

    // Toutes les sous-grilles sont fermées sans alignement absolu :
    // celui qui a gagné le plus de sous-grilles l'emporte (règle du sujet)
    const int c0 = popcount9(s.metaBoard[0]);
    const int c1 = popcount9(s.metaBoard[1]);
    if (c0 > c1) return 0;
    if (c1 > c0) return 1;
    return 2; // égalité
}

// ============================================================================
//  MCTS — PHASE 1 : SÉLECTION (UCB1)
//
//  On choisit le fils qui maximise :
//        winRate(c)  +  C * sqrt( ln(N_parent) / N_c )
//  où winRate est calculé du point de vue du joueur ayant joué `c.move`.
// ============================================================================
UltimateAI::Node*
UltimateAI::selectBestUCB(Node* node, double C) const {
    Node*  best    = nullptr;
    double bestVal = -1.0e18;
    const double logN = std::log(static_cast<double>(node->visits));

    for (auto& cu : node->children) {
        Node* c = cu.get();
        const double exploit = c->totalScore / static_cast<double>(c->visits);
        const double explore = C * std::sqrt(logN / static_cast<double>(c->visits));
        const double ucb     = exploit + explore;
        if (ucb > bestVal) {
            bestVal = ucb;
            best    = c;
        }
    }
    return best;
}

// ============================================================================
//  MCTS — PHASE 3 : SIMULATION (light playout)
//
//  On joue jusqu'à la fin de la partie selon une politique simple :
//    - Heuristique : si un coup légal GAGNE la partie immédiatement, on le joue
//    - Sinon : choix uniforme aléatoire parmi les coups légaux
//
//  L'heuristique n'augmente pratiquement pas le coût (test bitboard pur)
//  mais accélère énormément la convergence des estimations MCTS.
// ============================================================================
int UltimateAI::simulate(State s) {
    auto& moves = m_simBuffer;

    for (int turn = 0; turn < 100; ++turn) { // 81 coups max, 100 par sécurité
        const int w = checkWinner(s);
        if (w != -1) return w;

        getLegalMoves(s, moves);
        if (moves.empty()) break; // ne devrait pas arriver

        const int p = s.playerToMove;
        Move chosen;
        bool tactical = false;

        // Heuristique : un coup qui gagne la partie ?
        for (const Move& mv : moves) {
            const uint16_t newSub =
                static_cast<uint16_t>(s.boards[p][mv.board] | (1u << mv.cell));
            if (s_winLookup[newSub]) {
                const uint16_t newMeta =
                    static_cast<uint16_t>(s.metaBoard[p] | (1u << mv.board));
                if (s_winLookup[newMeta]) {
                    chosen   = mv;
                    tactical = true;
                    break;
                }
            }
        }

        if (!tactical) {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(moves.size()) - 1);
            chosen = moves[dist(m_rng)];
        }

        applyMoveTo(s, chosen);
    }

    // Filet de sécurité : décompte final
    const int c0 = popcount9(s.metaBoard[0]);
    const int c1 = popcount9(s.metaBoard[1]);
    if (c0 > c1) return 0;
    if (c1 > c0) return 1;
    return 2;
}

// ============================================================================
//  MCTS — PHASE 4 : RÉTROPROPAGATION
//
//  Chaque nœud accumule un score du POINT DE VUE du joueur ayant joué le
//  coup correspondant :
//     - victoire de ce joueur  => +1.0
//     - égalité                => +0.5
//     - défaite                => +0.0
// ============================================================================
void UltimateAI::backpropagate(Node* node, int winner) {
    while (node != nullptr) {
        ++node->visits;
        if (winner == 2)                          node->totalScore += 0.5;
        else if (winner == node->playerJustMoved) node->totalScore += 1.0;
        node = node->parent;
    }
}

// ============================================================================
//  MCTS — BOUCLE PRINCIPALE
// ============================================================================
UltimateAI::Move UltimateAI::runMCTS(const State& rootState) {
    // Garde-fou : si la partie est déjà terminée
    if (checkWinner(rootState) != -1) return Move(0, 0);

    // La racine représente l'état AVANT notre coup.
    // playerJustMoved à la racine est, par convention, l'adversaire.
    auto root = std::make_unique<Node>(
        nullptr, Move(),
        static_cast<int8_t>(1 - rootState.playerToMove));

    getLegalMoves(rootState, root->untriedMoves);

    if (root->untriedMoves.empty())          return Move(0, 0);
    if (root->untriedMoves.size() == 1)      return root->untriedMoves[0]; // forcé

    const auto deadline = std::chrono::steady_clock::now()
                        + std::chrono::milliseconds(m_thinkingTimeMs);

    int iterations = 0;

    while (std::chrono::steady_clock::now() < deadline) {
        State state = rootState; // copie locale (état petit -> peu coûteux)
        Node* node  = root.get();

        // ------------------------------------------------------------
        // 1) SÉLECTION : descendre tant qu'il n'y a plus de coup à explorer
        //                et que des fils existent
        // ------------------------------------------------------------
        while (node->untriedMoves.empty() && !node->children.empty()) {
            node = selectBestUCB(node, UCB1_C);
            applyMoveTo(state, node->move);
        }

        // ------------------------------------------------------------
        // 2) EXPANSION : créer un nouveau fils si possible
        // ------------------------------------------------------------
        if (!node->untriedMoves.empty() && checkWinner(state) == -1) {
            std::uniform_int_distribution<int> dist(
                0, static_cast<int>(node->untriedMoves.size()) - 1);
            const int idx  = dist(m_rng);
            const Move mv  = node->untriedMoves[idx];

            // swap-pop : retrait O(1)
            node->untriedMoves[idx] = node->untriedMoves.back();
            node->untriedMoves.pop_back();

            const int8_t mover = state.playerToMove;
            applyMoveTo(state, mv);

            auto child   = std::make_unique<Node>(node, mv, mover);
            // On ne génère les coups légaux du fils que si l'état n'est pas terminal
            if (checkWinner(state) == -1) {
                getLegalMoves(state, child->untriedMoves);
            }
            Node* childPtr = child.get();
            node->children.push_back(std::move(child));
            node = childPtr;
        }

        // ------------------------------------------------------------
        // 3) SIMULATION
        // ------------------------------------------------------------
        const int winner = simulate(state);

        // ------------------------------------------------------------
        // 4) RÉTROPROPAGATION
        // ------------------------------------------------------------
        backpropagate(node, winner);

        ++iterations;
    }

    // -- Sélection finale du coup à jouer : le fils LE PLUS VISITÉ --
    //    (statistique la plus robuste, recommandée dans la litt.)
    Node* best       = nullptr;
    int   bestVisits = -1;
    for (auto& cu : root->children) {
        if (cu->visits > bestVisits) {
            bestVisits = cu->visits;
            best       = cu.get();
        }
    }

    if (!best) {
        // Aucune expansion n'a eu lieu (cas pathologique) : on renvoie
        // n'importe quel coup légal restant.
        return root->untriedMoves.empty() ? Move(0, 0) : root->untriedMoves[0];
    }

    // Trace de debug pour la soutenance / DEBUG mode
    std::cerr << "[MCTS] " << iterations << " iters | choix : (b="
              << static_cast<int>(best->move.board) << ",c="
              << static_cast<int>(best->move.cell)  << ") | visites="
              << best->visits << " | winRate="
              << (best->totalScore / static_cast<double>(best->visits))
              << '\n';

    return best->move;
}

GameMove UltimateAI::computeMove() {
    return toGameMove(runMCTS(m_state));
}
