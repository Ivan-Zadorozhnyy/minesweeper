#include <SFML/Graphics.hpp>
#include <vector>
#include <chrono>
#include <random>

class Game;
class Cell;
class Board;
class Renderer;

constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

class Timer {
private:
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    std::chrono::duration<float> elapsed_time;
    bool running;

public:
    Timer() : running(false), elapsed_time(std::chrono::duration<float>::zero()) {}

    void start() {
        start_time = std::chrono::steady_clock::now();
        running = true;
    }

    void stop() {
        if (running) {
            elapsed_time = std::chrono::steady_clock::now() - start_time;
            running = false;
        }
    }

    void reset() {
        running = false;
        elapsed_time = std::chrono::duration<float>::zero();
    }

    float getElapsedTime() const {
        if (running) {
            auto current_time = std::chrono::steady_clock::now();
            auto time = current_time - start_time + elapsed_time;
            return std::chrono::duration<float>(time).count();
        } else {
            return elapsed_time.count();
        }
    }
};

class Menu {
private:
    Game* game;

public:
    explicit Menu(Game* game) : game(game) {}

    void display(sf::RenderWindow& window) {
        // display the menu on the game window
        // draw menu
    }

    void handleInput(const sf::Event& event) {
        // handle menu input and start the game when required
        // process user input to navigate through the menu
    }
};

enum class Difficulty {
    Easy, Medium, Hard
};

enum class CellState {
    Hidden, Revealed, Flagged
};

class Cell {
private:
    CellState state;
    bool isMine;
    int adjacentMines;
    sf::Sprite sprite; // SFML sprite for rendering

public:
    // Constructor
    Cell() : state(CellState::Hidden), isMine(false), adjacentMines(0) {
        // Load the textures and set the sprite if using sprites.
        // Ensure you have initialized SFML textures before setting them here.
        // sprite.setTexture(hiddenTexture); // Placeholder for actual texture
    }

    // Reveal the cell
    void reveal() {
        if (state == CellState::Hidden) {
            state = CellState::Revealed;
            // If the cell is a mine, you will need to trigger additional game-over logic here
            updateSprite(); // Update the sprite based on the new state
        }
    }

    // Toggle the flagged state of the cell
    void toggleFlag() {
        if (state == CellState::Hidden) {
            state = CellState::Flagged;
            updateSprite(); // Update the sprite to show a flag
        } else if (state == CellState::Flagged) {
            state = CellState::Hidden;
            updateSprite(); // Update the sprite to hide the flag
        }
    }

    // Set the cell as containing a mine
    void setMine(bool mine) {
        isMine = mine;
        // If setting as mine, make sure adjacentMines is set to 0 or adjust the logic as needed.
    }

    // Increment the count of adjacent mines
    void incrementAdjacentMines() {
        if (!isMine) {
            ++adjacentMines;
            // Optionally update the sprite to display the number of adjacent mines
        }
    }

    // Getters for the cell state and properties
    bool containsMine() const { return isMine; }
    int getAdjacentMines() const { return adjacentMines; }
    CellState getState() const { return state; }
    bool isRevealed() const { return state == CellState::Revealed; }
    bool isFlagged() const { return state == CellState::Flagged; }

private:
    // Update the sprite based on the cell's current state
    void updateSprite() {
        // Here you will set the appropriate texture based on the cell state.
        // You will need to load these textures from your resources.
        switch (state) {
            case CellState::Hidden:
                // sprite.setTexture(hiddenTexture);
                break;
            case CellState::Revealed:
                if (isMine) {
                    // sprite.setTexture(mineTexture);
                } else {
                    // sprite.setTexture(numberTexture); // Texture showing the number of adjacent mines
                    // You'll likely need some logic to select the correct number texture based on adjacentMines.
                }
                break;
            case CellState::Flagged:
                // sprite.setTexture(flagTexture);
                break;
        }
    }
};

class Board {
private:
    std::vector<std::vector<Cell>> cells;
    int width;
    int height;
    int mineCount;
    bool firstClick;

    void placeMines(int excludedX, int excludedY) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, width * height - 1);

        int minesPlaced = 0;
        while (minesPlaced < mineCount) {
            int index = dis(gen);
            int x = index % width;
            int y = index / width;

            if ((x != excludedX || y != excludedY) && !cells[y][x].containsMine()) {
                cells[y][x].setMine(true);
                minesPlaced++;
            }
        }
    }

    void calculateAdjacentMines() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (cells[y][x].containsMine()) {
                    continue;
                }
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx, ny = y + dy;
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            if (cells[ny][nx].containsMine()) {
                                cells[y][x].incrementAdjacentMines();
                            }
                        }
                    }
                }
            }
        }
    }

public:
    Board(int w, int h, int m)
            : width(w), height(h), mineCount(m), firstClick(true) {
        cells.resize(height, std::vector<Cell>(width));
    }

    void firstReveal(int x, int y) {
        if (firstClick) {
            placeMines(x, y);
            calculateAdjacentMines();
            firstClick = false;
        }
        revealCell(x, y);
    }

    void revealCell(int x, int y) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        Cell& cell = cells[y][x];

        if (!cell.isRevealed() && !cell.isFlagged()) {
            cell.reveal();

            if (cell.getAdjacentMines() == 0 && !cell.containsMine()) {
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        revealCell(x + dx, y + dy);
                    }
                }
            }
        }
    }

    void flagCell(int x, int y) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        Cell& cell = cells[y][x];

        if (!cell.isRevealed()) {
            cell.toggleFlag();
        }
    }

    bool checkWinCondition() {
        for (const auto& row : cells) {
            for (const auto& cell : row) {
                if (!cell.containsMine() && !cell.isRevealed()) {
                    return false;
                }
            }
        }
        return true;
    }

    bool checkLossCondition() {
        for (const auto& row : cells) {
            for (const auto& cell : row) {
                if (cell.containsMine() && cell.isRevealed()) {
                    return true;
                }
            }
        }
        return false;
    }
};

class Renderer {
private:
    sf::RenderWindow& window;

public:
    explicit Renderer(sf::RenderWindow& win) : window(win) {}

    void drawBoard(const Board& board) {
        // draw the game board
        // loop through the board and draw cells based on their state
    }

    void drawMenu(const Menu& menu) {
        // Draw the menu
        // menu display logic
    }

    // additional drawing methods...
};

class Game {
private:
    sf::RenderWindow window;
    Timer timer;
    Board board;
    Menu menu;
    Renderer renderer;
    Difficulty difficulty;
    bool game_over;

public:
    Game() : window(sf::VideoMode(WIDTH, HEIGHT), "Minesweeper"),
             board(10, 10, 10), // Placeholder values for board dimensions and mine count
             menu(this),
             renderer(window),
             game_over(false) {
        // initialization logic for the game state
    }

    void run() {
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }

                if (!game_over) {
                    // handle game control input
                } else {
                    menu.handleInput(event);
                }
            }

            update();
            render();
        }
    }

    void startGame(Difficulty chosenDifficulty) {
        difficulty = chosenDifficulty;
        timer.start();
        game_over = false;
        // Game start logic
    }

    void endGame(bool won) {
        timer.stop();
        game_over = true;
        // Game end logic
    }

private:
    void update() {
        if (!game_over) {
            // update game logic
        }
        // otherwise, update the menu
    }

    void render() {
        window.clear();
        if (!game_over) {
            renderer.drawBoard(board);
        } else {
            renderer.drawMenu(menu);
        }
        window.display();
    }

    // add any other methods needed for game logic
};

int main() {
    Game minesweeper;
    minesweeper.run();
    return 0;
}
