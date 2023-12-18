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
    sf::Sprite sprite;

    static sf::Texture hiddenTexture;
    static sf::Texture mineTexture;
    static sf::Texture flagTexture;
    static std::vector<sf::Texture> numberTextures;

public:
    Cell() : state(CellState::Hidden), isMine(false), adjacentMines(0) {
        sprite.setTexture(hiddenTexture); // Set the default texture
    }

    // Static method to load textures (call this method before creating Cell instances)
    static void loadTextures() {
        hiddenTexture.loadFromFile("path/to/hiddenTexture.png");
        mineTexture.loadFromFile("path/to/mineTexture.png");
        flagTexture.loadFromFile("path/to/flagTexture.png");
        numberTextures.resize(8); // Resize the vector to hold 8 textures

        for (int i = 0; i < 8; ++i) {
            numberTextures[i].loadFromFile("path/to/number" + std::to_string(i + 1) + ".png");
        }
    }

    void reveal() {
        if (state == CellState::Hidden) {
            state = CellState::Revealed;
            updateSprite(); // Update the sprite based on the new state
        }
    }

    void toggleFlag() {
        if (state == CellState::Hidden) {
            state = CellState::Flagged;
        } else if (state == CellState::Flagged) {
            state = CellState::Hidden;
        }
        updateSprite(); // Update the sprite to show/hide the flag
    }

    void setMine(bool mine) {
        isMine = mine;
        if (mine) {
            adjacentMines = 0; // Reset adjacent mines count if it's a mine
        }
    }

    void incrementAdjacentMines() {
        if (!isMine) {
            ++adjacentMines;
        }
    }

    // Getters for cell properties
    bool containsMine() const { return isMine; }
    int getAdjacentMines() const { return adjacentMines; }
    CellState getState() const { return state; }
    bool isRevealed() const { return state == CellState::Revealed; }
    bool isFlagged() const { return state == CellState::Flagged; }

    const sf::Sprite& getSprite() const { return sprite; }

private:
    void updateSprite() {
        switch (state) {
            case CellState::Hidden:
                sprite.setTexture(hiddenTexture);
                break;
            case CellState::Revealed:
                if (isMine) {
                    sprite.setTexture(mineTexture);
                } else {
                    if (adjacentMines > 0) {
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

// Static texture initialization
sf::Texture Cell::hiddenTexture;
sf::Texture Cell::mineTexture;
sf::Texture Cell::flagTexture;
std::vector<sf::Texture> Cell::numberTextures;


class Board {
private:
    std::vector<std::vector<Cell>> cells;
    int width;
    int height;
    int mineCount;
    bool firstClick;

public:
    Board(int w, int h, int m) : width(w), height(h), mineCount(m), firstClick(true) {
        cells.resize(height, std::vector<Cell>(width));
        // Initialize each cell (assuming Cell class has a method for texture loading)
        Cell::loadTextures();
    }

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
                incrementAdjacentMines(x, y);
                minesPlaced++;
            }
        }
    }

    void incrementAdjacentMines(int x, int y) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = x + dx, ny = y + dy;
                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    cells[ny][nx].incrementAdjacentMines();
                }
            }
        }
    }

    void firstReveal(int x, int y) {
        if (firstClick) {
            placeMines(x, y);
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
                // Recursively reveal adjacent cells
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

    bool checkWinCondition() const {
        for (const auto& row : cells) {
            for (const auto& cell : row) {
                if (!cell.containsMine() && !cell.isRevealed()) {
                    return false;
                }
            }
        }
        return true;
    }

    bool checkLossCondition() const {
        for (const auto& row : cells) {
            for (const auto& cell : row) {
                if (cell.containsMine() && cell.isRevealed()) {
                    return true;
                }
            }
        }
        return false;
    }

    // Method to get the grid of cells for rendering
    const std::vector<std::vector<Cell>>& getCells() const {
        return cells;
    }
};

class Renderer {
private:
    sf::RenderWindow& window;

public:
    explicit Renderer(sf::RenderWindow& win) : window(win) {}

    void drawBoard(const Board& board) {
        // Assuming Board class has a method to get the grid of cells
        const auto& cells = board.getCells();
        for (const auto& row : cells) {
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
    Board* board; // Dynamically allocated based on difficulty
    Menu menu;
    Renderer renderer;
    Difficulty difficulty;
    bool game_over;

public:
    Game() : window(sf::VideoMode(WIDTH, HEIGHT), "Minesweeper"),
             board(nullptr), // Initialize as nullptr
             menu(this),
             renderer(window),
             game_over(true) { // Start with the menu displayed
        Cell::loadTextures(); // Load textures for cells
    }

    ~Game() {
        delete board; // Clean up board memory
    }

    void run() {
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                handleEvent(event);
            }

            update();
            render();
        }
    }

    void handleEvent(const sf::Event& event) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }

        if (game_over) {
            menu.handleInput(event);
        } else {
            // Handle game interactions here (mouse events)
            if (event.type == sf::Event::MouseButtonPressed) {
                handleMouseInput(event.mouseButton);
            }
        }
    }

    void handleMouseInput(const sf::Event::MouseButtonEvent& mouseEvent) {
        // Example: Convert mouse position to board coordinates and reveal or flag the cell
        if (mouseEvent.button == sf::Mouse::Left) {
            // Left click: reveal cell
            // Convert mouse position to board coordinates (you'll need to implement this)
            int x, y;
            std::tie(x, y) = convertToBoardCoordinates(mouseEvent.x, mouseEvent.y);
            board->revealCell(x, y);
        } else if (mouseEvent.button == sf::Mouse::Right) {
            // Right click: flag cell
            int x, y;
            std::tie(x, y) = convertToBoardCoordinates(mouseEvent.x, mouseEvent.y);
            board->flagCell(x, y);
        }
    }

    std::pair<int, int> convertToBoardCoordinates(int mouseX, int mouseY) {
        // Convert mouse coordinates to board cell indices
        // This depends on your cell size and board layout
        int x = mouseX / cellWidth;  // cellWidth to be defined based on your cell size
        int y = mouseY / cellHeight; // cellHeight to be defined
        return {x, y};
    }

    void startGame(Difficulty chosenDifficulty) {
        difficulty = chosenDifficulty;
        delete board; // Delete the old board if it exists
        // Create a new board based on the chosen difficulty
        int width, height, mines;
        // Define width, height, and mines based on difficulty...
        board = new Board(width, height, mines);
        timer.start();
        game_over = false;
    }

    void endGame(bool won) {
        timer.stop();
        game_over = true;
        // Handle game end scenario (display message, etc.)
    }

private:
    void update() {
        if (!game_over) {
            if (board->checkWinCondition()) {
                endGame(true);
            } else if (board->checkLossCondition()) {
                endGame(false);
            }
            // Update other game logic if necessary
        }
    }

    void render() {
        window.clear();
        if (!game_over) {
            renderer.drawBoard(*board);
        } else {
            renderer.drawMenu(menu);
        }
        window.display();
    }

    // Other private methods...
};

int main() {
    Game minesweeper;
    minesweeper.run();
    return 0;
}
