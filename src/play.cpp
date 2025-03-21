#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <random>
#include <tuple>
#include "space.h"

// Type alias for Q-table.
using QTable = std::unordered_map<std::string, std::vector<double>>;

// Q-learning hyperparameters.
const double alpha = 0.1;
const double gamma = 0.9;

// Encode the current board state and current player into a key.
// Board cells are encoded as '-' for empty, 'X' for 0, and 'O' for 1, then appended by the current player's char.
std::string getStateKey(const Space &game, char currentPlayer) {
    std::string key;
    key.reserve(10); // 9 board cells + 1 player char.
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            signed char cell = game.get(x, y);
            if (cell == -1)
                key.push_back('-');
            else if (cell == 0)
                key.push_back('X');
            else if (cell == 1)
                key.push_back('O');
        }
    }
    key.push_back(currentPlayer);
    return key;
}

// Returns a list of legal moves (cell indices) on the board.
std::vector<int> getLegalMoves(const Space &game) {
    std::vector<int> moves;
    moves.reserve(9);
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            if (game.get(x, y) == -1)
                moves.push_back(y * 3 + x);
        }
    }
    return moves;
}

// Loads the Q-table from a file.
QTable loadQTable(const std::string &filename) {
    QTable Q;
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Error: failed to open " << filename << "\n";
        return Q;
    }
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string state;
        iss >> state;
        std::vector<double> values;
        double val;
        while (iss >> val) {
            values.push_back(val);
        }
        Q[state] = values;
    }
    return Q;
}

// Saves the Q-table to a file.
void saveQTable(const QTable &Q, const std::string &filename) {
    std::ofstream out(filename);
    if (!out) {
        std::cerr << "Error: failed to open " << filename << " for writing.\n";
        return;
    }
    for (const auto &entry : Q) {
        out << entry.first;
        for (double qVal : entry.second) {
            out << " " << qVal;
        }
        out << "\n";
    }
    out.close();
}

int main() {
    // Load the pre-trained Q-table.
    QTable Q = loadQTable("ai_model.dat");
    if (Q.empty()) {
        std::cerr << "Error: Q table is empty. Exiting.\n";
        return 1;
    }

    Space game;
    game.resize(3, 3);

    std::cout << "Welcome to XXO! You are X and the AI is O.\n";
    game.print();

    char currentPlayer = 'X';  // Human is X; AI is O.
    std::random_device rd;
    std::mt19937 gen(rd());

    // Record only AI's moves for online learning.
    std::vector<std::tuple<std::string, int>> aiHistory;

    while (true) {
        if (currentPlayer == 'X') {
            // Human's turn.
            int x, y;
            std::cout << "Enter your move (x y): ";
            std::cin >> x >> y;
            try {
                if (game.get(x, y) != -1) {
                    std::cout << "Cell is already occupied. Try again.\n";
                    continue;
                }
                game.place(x, y, 0);  // X is represented by 0.
            } catch (std::exception &e) {
                std::cout << e.what() << "\n";
                continue;
            }
        } else {
            // AI's turn.
            std::string state = getStateKey(game, 'O');
            auto legalMoves = getLegalMoves(game);
            int action = -1;
            if (Q.find(state) != Q.end()) {
                double bestValue = -1e9;
                int bestAction = legalMoves[0];
                for (int a : legalMoves) {
                    if (Q[state][a] > bestValue) {
                        bestValue = Q[state][a];
                        bestAction = a;
                    }
                }
                action = bestAction;
            } else {
                // If state not seen, choose a random legal move.
                std::uniform_int_distribution<> moveDis(0, legalMoves.size() - 1);
                action = legalMoves[moveDis(gen)];
            }
            // Record the AI's move for later learning.
            aiHistory.emplace_back(state, action);

            int x = action % 3;
            int y = action / 3;
            game.place(x, y, 1);  // O is represented by 1.
            std::cout << "AI placed an O at (" << x << ", " << y << ")\n";
        }

        game.print();

        int result = game.check_win();  // 0 for X win, 1 for O win, -1 for no win.
        if (result != -1) {
            if (result == 0)
                std::cout << "X wins!\n";
            else if (result == 1)
                std::cout << "O wins!\n";

            // Determine reward from AI's perspective.
            // AI win: reward = 1, loss: reward = -1.
            double reward = (result == 1) ? 1.0 : -1.0;
            double target = reward;
            // Update the Q-values for the AI's moves in reverse order.
            for (int i = static_cast<int>(aiHistory.size()) - 1; i >= 0; --i) {
                std::string s;
                int a;
                std::tie(s, a) = aiHistory[i];
                if (Q.find(s) == Q.end()) {
                    Q[s] = std::vector<double>(9, 0.0);
                }
                Q[s][a] += alpha * (target - Q[s][a]);
                target *= gamma;
            }
            break;
        }

        // Check for a draw.
        if (getLegalMoves(game).empty()) {
            std::cout << "It's a draw!\n";
            double reward = 0.0;
            double target = reward;
            for (int i = static_cast<int>(aiHistory.size()) - 1; i >= 0; --i) {
                std::string s;
                int a;
                std::tie(s, a) = aiHistory[i];
                if (Q.find(s) == Q.end()) {
                    Q[s] = std::vector<double>(9, 0.0);
                }
                Q[s][a] += alpha * (target - Q[s][a]);
                target *= gamma;
            }
            break;
        }

        // Switch turns.
        currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    }

    // Save the updated Q-table back to file.
    saveQTable(Q, "ai_model.dat");
    std::cout << "Game over. The AI has updated its knowledge from the game.\n";

    return 0;
}
