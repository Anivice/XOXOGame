#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <random>
#include <string>
#include <thread>
#include <sstream>
#include "space.h"
#include "log.hpp"

// Define a type alias for our Q-table:
// Key: string encoding of the board + current player
// Value: vector of Q-values for each of the 9 possible cell positions.
using QTable = std::unordered_map<std::string, std::vector<double>>;

// Q-learning hyperparameters
const double alpha = 0.1;
const double gamma = 0.9;
const double epsilon = 0.2;
// Total number of episodes to train.
const unsigned long long numEpisodes = 5000000ULL;

// Number of threads to use.
const unsigned int numThreads = 20;

// Get a string key for the current board state and player turn.
// The board is encoded row by row as:
// '-' for empty, 'X' for cell with 0, and 'O' for cell with 1.
// Then we append the current player's identifier.
std::string getStateKey(const Space &game, char currentPlayer) {
    std::string key;
    key.reserve(10); // 9 board cells + 1 character
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

// Return the list of legal moves (cell indices) from the current state.
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

// This function runs a block of episodes on a separate thread and
// stores its learned Q-table in localQ.
void trainEpisodes(unsigned long long episodes, QTable &localQ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    for (unsigned long long episode = 0; episode < episodes; ++episode) {
        debug::log(episode, "/", episodes, " ...\n");
        Space game;
        game.resize(3, 3);
        char currentPlayer = 'X';  // start with X
        bool gameOver = false;
        // Record history as a sequence of (state, action) pairs.
        std::vector<std::tuple<std::string, int>> history;
        history.reserve(9);

        while (!gameOver) {
            std::string state = getStateKey(game, currentPlayer);
            auto legalMoves = getLegalMoves(game);
            // If no legal moves remain, it's a draw.
            if (legalMoves.empty()) {
                double reward = 0.0;
                double target = reward;
                // Backpropagate reward through the moves.
                for (int i = static_cast<int>(history.size()) - 1; i >= 0; --i) {
                    std::string s;
                    int a;
                    std::tie(s, a) = history[i];
                    if (localQ.find(s) == localQ.end()) {
                        localQ[s] = std::vector<double>(9, 0.0);
                    }
                    localQ[s][a] += alpha * (target - localQ[s][a]);
                    target *= gamma;
                }
                break;
            }

            // If the state is unseen, initialize its Q vector.
            if (localQ.find(state) == localQ.end()) {
                localQ[state] = std::vector<double>(9, 0.0);
            }

            int action;
            // Epsilon-greedy action selection.
            std::uniform_real_distribution<> dis(0.0, 1.0);
            if (dis(gen) < epsilon) {
                std::uniform_int_distribution<> moveDis(0, static_cast<int>(legalMoves.size()) - 1);
                action = legalMoves[moveDis(gen)];
            } else {
                double bestValue = -1e9;
                int bestAction = legalMoves[0];
                for (int a : legalMoves) {
                    if (localQ[state][a] > bestValue) {
                        bestValue = localQ[state][a];
                        bestAction = a;
                    }
                }
                action = bestAction;
            }
            history.emplace_back(state, action);
            int x = action % 3;
            int y = action / 3;
            // Place the symbol: X is represented by 0, O by 1.
            signed char symbol = (currentPlayer == 'X') ? 0 : 1;
            game.place(x, y, symbol);

            // Check for a win.
            int result = game.check_win(); // returns 0 for X win, 1 for O win, -1 for no win.
            if (result != -1) {
                gameOver = true;
                // Determine reward from the perspective of the player who just moved.
                double reward = ((currentPlayer == 'X' && result == 0) || (currentPlayer == 'O' && result == 1)) ? 1.0 : -1.0;
                double target = reward;
                // Update all moves in history in reverse order.
                for (int i = static_cast<int>(history.size()) - 1; i >= 0; --i) {
                    std::string s;
                    int a;
                    std::tie(s, a) = history[i];
                    if (localQ.find(s) == localQ.end()) {
                        localQ[s] = std::vector<double>(9, 0.0);
                    }
                    localQ[s][a] += alpha * (target - localQ[s][a]);
                    target *= gamma;
                }
            } else if (getLegalMoves(game).empty()) {
                // Board is full; it's a draw.
                gameOver = true;
                double reward = 0.0;
                double target = reward;
                for (int i = static_cast<int>(history.size()) - 1; i >= 0; --i) {
                    std::string s;
                    int a;
                    std::tie(s, a) = history[i];
                    if (localQ.find(s) == localQ.end()) {
                        localQ[s] = std::vector<double>(9, 0.0);
                    }
                    localQ[s][a] += alpha * (target - localQ[s][a]);
                    target *= gamma;
                }
            } else {
                // Switch player and continue.
                currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
            }
        } // end while
    } // end episodes
}

int main() {
    // Create one QTable per thread.
    std::vector<QTable> localQTables(numThreads);
    std::vector<std::thread> threads;
    unsigned long long episodesPerThread = numEpisodes / numThreads;
    unsigned long long remainder = numEpisodes % numThreads;

    // Launch training threads.
    for (unsigned int i = 0; i < numThreads; i++) {
        // Distribute the remainder among the first few threads.
        unsigned long long episodesForThisThread = episodesPerThread + (i < remainder ? 1 : 0);
        threads.emplace_back(trainEpisodes, episodesForThisThread, std::ref(localQTables[i]));
    }

    // Wait for all threads to complete.
    for (auto &t : threads) {
        t.join();
    }

    // Merge the per-thread Q-tables.
    // For states that appear in multiple threads, average their Q-values.
    std::unordered_map<std::string, std::pair<std::vector<double>, int>> merged;
    for (const auto &qt : localQTables) {
        for (const auto &entry : qt) {
            const std::string &state = entry.first;
            const std::vector<double> &qvals = entry.second;
            if (merged.find(state) == merged.end()) {
                merged[state] = { qvals, 1 };
            } else {
                auto &p = merged[state];
                for (size_t i = 0; i < qvals.size(); ++i) {
                    p.first[i] += qvals[i];
                }
                p.second++;
            }
        }
    }

    // Compute the average Q-values for each state.
    QTable globalQ;
    for (auto &entry : merged) {
        const std::string &state = entry.first;
        std::vector<double> qvec = entry.second.first;
        int count = entry.second.second;
        for (auto &q : qvec) {
            q /= count;
        }
        globalQ[state] = qvec;
    }

    // Save the merged Q-table to a file.
    std::ofstream out("ai_model.dat");
    if (!out) {
        std::cerr << "Error: could not open ai_model.dat for writing.\n";
        return 1;
    }
    for (const auto &entry : globalQ) {
        out << entry.first;
        for (double qVal : entry.second) {
            out << " " << qVal;
        }
        out << "\n";
    }
    out.close();
    std::cout << "Training complete. Q table saved to ai_model.dat\n";

    return 0;
}
