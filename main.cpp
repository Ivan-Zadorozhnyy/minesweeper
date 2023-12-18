#include <SFML/Graphics.hpp>
#include <vector>
#include <chrono>
#include <random>
#include <iostream>

class Game;
class Cell;
class Board;
class Renderer;

constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

enum class Difficulty {
    Easy, Medium, Hard
};

enum class CellState {
    Hidden, Revealed, Flagged
};

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
    sf::Font font;
    sf::Text startText;
    sf::Text difficultyText;
    Difficulty currentDifficulty;
    std::vector<std::string> difficultyOptions = {"Easy", "Medium", "Hard"};

public:
    explicit Menu(Game* game) : game(game), currentDifficulty(Difficulty::Easy) {
        // Load the font
        if (!font.loadFromFile("path/to/font.ttf")) {
            // Handle error, e.g., print an error message or use a default font
            std::cerr << "Failed to load font." << std::endl;
        }

        // Initialize the start text
        startText.setFont(font);
        startText.setString("Start Game");
        startText.setCharacterSize(24);
        startText.setFillColor(sf::Color::White);
        startText.setPosition(WIDTH / 2.f, HEIGHT / 2.f);

        // Initialize the difficulty option text
        difficultyText.setFont(font);
        difficultyText.setString("Difficulty: Easy");
        difficultyText.setCharacterSize(24);
        difficultyText.setFillColor(sf::Color::White);
        difficultyText.setPosition(WIDTH / 2.f, HEIGHT / 2.f + 60.f);
    }

    void display(sf::RenderWindow& window) {
        window.draw(startText);
        window.draw(difficultyText);
    }

    void handleInput(const sf::Event& event) {
        if (event.type == sf::Event::KeyPressed) {
            switch (event.key.code) {
                case sf::Keyboard::Up:
                    changeDifficulty(-1); // Cycle up in difficulty
                    break;
                case sf::Keyboard::Down:
                    changeDifficulty(1); // Cycle down in difficulty
                    break;
                case sf::Keyboard::Enter:
                    game->startGame(currentDifficulty); // Start the game with the current difficulty
                    break;
            }
        }
    }

    void changeDifficulty(int change) {
        int difficultyIndex = static_cast<int>(currentDifficulty) + change;
        // Wrap around the difficulty options
        if (difficultyIndex >= static_cast<int>(difficultyOptions.size())) {
            difficultyIndex = 0;
        } else if (difficultyIndex < 0) {
            difficultyIndex = difficultyOptions.size() - 1;
        }
        currentDifficulty = static_cast<Difficulty>(difficultyIndex);
        difficultyText.setString("Difficulty: " + difficultyOptions[difficultyIndex]);
    }

    // Accessor methods for the text objects
    sf::Text& getStartText() { return startText; }
    sf::Text& getDifficultyText() { return difficultyText; }
};

class Cell {
private:
    CellState state;
    bool isMine;
    int adjacentMines;
    sf::Sprite sprite; // SFML sprite for rendering
    sf::Texture hiddenTexture;
    sf::Texture mineTexture;
    sf::Texture flagTexture;
    std::vector<sf::Texture> numberTextures; // Textures for numbers 1-8

public:
    // Constructor
    Cell() : state(CellState::Hidden), isMine(false), adjacentMines(0) {
        // Load the textures and set the sprite if using sprites.
        hiddenTexture.loadFromFile("path/to/hiddenTexture.png");
        mineTexture.loadFromFile("path/to/mineTexture.png");
        flagTexture.loadFromFile("path/to/flagTexture.png");
        // Load number textures
        for (int i = 1; i <= 8; ++i) {
            sf::Texture texture;
            texture.loadFromFile("path/to/number" + std::to_string(i) + ".png");
            numberTextures.push_back(texture);
        }
        sprite.setTexture(hiddenTexture); // Default to hidden texture
    }

    // Reveal the cell
    void reveal() {
        if (state == CellState::Hidden) {
            state = CellState::Revealed;
            // If the cell is a mine, additional game-over logic should be triggered outside this class
            updateSprite(); // Update the sprite based on the new state
        }
    }

    // Toggle the flagged state of the cell
    void toggleFlag() {
        if (state == CellState::Hidden) {
            state = CellState::Flagged;
        } else if (state == CellState::Flagged) {
            state = CellState::Hidden;
        }
        updateSprite(); // Update the sprite to show/hide the flag
    }

    // Set the cell as containing a mine
    void setMine(bool mine) {
        isMine = mine;
        if (mine) {
            adjacentMines = 0; // Ensure adjacentMines is set to 0 if it's a mine
        }
    }

    // Increment the count of adjacent mines
    void incrementAdjacentMines() {
        if (!isMine) {
            ++adjacentMines;
            // No need to update the sprite here since the number is revealed when the cell is revealed
        }
    }

    // Getters for the cell state and properties
    bool containsMine() const { return isMine; }
    int getAdjacentMines() const { return adjacentMines; }
    CellState getState() const { return state; }
    bool isRevealed() const { return state == CellState::Revealed; }
    bool isFlagged() const { return state == CellState::Flagged; }
    sf::Sprite& getSprite() { return sprite; } // Getter to retrieve the sprite for rendering

private:
    // Update the sprite based on the cell's current state
    void updateSprite() {
        switch (state) {
            case CellState::Hidden:
                sprite.setTexture(hiddenTexture);
                break;
            case CellState::Revealed:
                if (isMine) {
                    sprite.setTexture(mineTexture);
                } else {
                    // Texture showing the number of adjacent mines
                    if (adjacentMines > 0) { // Only show a number if there are adjacent mines
                        sprite.setTexture(numberTextures[adjacentMines - 1]);
                    } else {
                        // If no adjacent mines, you might have a different texture for empty revealed cells
                    }
                }
                break;
            case CellState::Flagged:
                sprite.setTexture(flagTexture);
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
            // Assuming Board class has a method to get the grid of cells
            for (const auto& row : board.getCells()) {
                for (const auto& cell : row) {
                    // Assuming Cell class has a method to get its sprite
                    window.draw(cell.getSprite());
                }
            }
        }

        void drawMenu(const Menu& menu) {
            // Draw the menu text objects
            // Assuming Menu class has methods to get the text objects
            window.draw(menu.getStartText());
            window.draw(menu.getDifficultyText());
            // If you have more menu items, draw them here as well
        }

        // You can add additional drawing methods if needed...
};

class Game {
private:
    sf::RenderWindow window;
    Timer timer;
    Board* board; // Use a pointer to dynamically allocate based on difficulty
    Menu menu;
    Renderer renderer;
    Difficulty difficulty;
    bool game_over;

public:
    Game() : window(sf::VideoMode(WIDTH, HEIGHT), "Minesweeper"),
             menu(this),
             renderer(window),
             game_over(false) {
        // Placeholder values for board dimensions and mine count
        // We will initialize the board properly in the startGame function
        board = new Board(10, 10, 10);
    }

    ~Game() {
        delete board; // Clean up dynamically allocated memory
    }

    void run() {
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }

                if (game_over) {
                    menu.handleInput(event);
                } else {
                    // Handle game control input here. For now, we'll just let the menu handle input.
                    // Later, you'll add mouse event handling here for cell reveal and flagging.
                }
            }

            update();
            render();
        }
    }

    void startGame(Difficulty chosenDifficulty) {
        difficulty = chosenDifficulty;
        // Delete the old board if it exists and create a new one based on the chosen difficulty
        delete board;
        board = nullptr;
        int width, height, mines;
        switch (difficulty) {
            case Difficulty::Easy:
                width = height = 10; mines = 10; break;
            case Difficulty::Medium:
                width = height = 14; mines = 40; break;
            case Difficulty::Hard:
                width = height = 18; mines = 60; break;
        }
        board = new Board(width, height, mines);
        timer.start();
        game_over = false;
        // Additional game start logic can go here
    }

    void endGame(bool won) {
        timer.stop();
        game_over = true;
        // Display game over logic and handle restart or exit
    }

private:
    void update() {
        if (!game_over) {
            // Update game logic here, like checking win/loss conditions
            if (board->checkWinCondition()) {
                endGame(true); // Handle win
            } else if (board->checkLossCondition()) {
                endGame(false); // Handle loss
            }
        }
        // Otherwise, update menu or handle other non-gameplay updates
    }

    void render() {
        window.clear();
        if (!game_over) {
            renderer.drawBoard(*board); // Dereference the pointer to pass the board to the renderer
        } else {
            renderer.drawMenu(menu);
        }
        window.display();
    }

    // Other private methods related to game logic can be added here
};

int main() {
    Game minesweeper;
    minesweeper.run();
    return 0;
}
