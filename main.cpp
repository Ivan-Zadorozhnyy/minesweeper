#include <SFML/Graphics.hpp>
#include <vector>
#include <chrono>
#include <random>
#include <iostream>

class Game;
class Menu;
class Renderer;

constexpr int WIDTH = 1280;
constexpr int HEIGHT = 720;

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

class Cell {
private:
    CellState state;
    bool isMine;
    int adjacentMines;

public:
    Cell() : state(CellState::Hidden), isMine(false), adjacentMines(0) {}

    void reveal() {
        if (state == CellState::Hidden) {
            state = CellState::Revealed;
        }
    }

    void toggleFlag() {
        if (state == CellState::Hidden) {
            state = CellState::Flagged;
        } else if (state == CellState::Flagged) {
            state = CellState::Hidden;
        }
    }

    void setMine(bool mine) {
        isMine = mine;
        if (mine) {
            adjacentMines = 0; // A mine cell has no adjacent mines
        }
    }

    void incrementAdjacentMines() {
        if (!isMine) {
            ++adjacentMines;
        }
    }

    // Accessors
    bool containsMine() const { return isMine; }
    int getAdjacentMines() const { return adjacentMines; }
    CellState getState() const { return state; }
    bool isRevealed() const { return state == CellState::Revealed; }
    bool isFlagged() const { return state == CellState::Flagged; }
};

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
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    bool isFirstClick() const {
        return firstClick;
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
        cells[y][x].toggleFlag();
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

    int countFlaggedCells() const {
        int flagCount = 0;
        for (const auto& row : cells) {
            for (const auto& cell : row) {
                if (cell.isFlagged()) {
                    ++flagCount;
                }
            }
        }
        return flagCount;
    }

    const std::vector<std::vector<Cell>>& getCells() const {
        return cells;
    }
};

class Menu {
private:
    Game *game;
    sf::Font font;
    sf::Text startText;
    sf::Text difficultyText;
    Difficulty currentDifficulty;
    std::vector<std::string> difficultyOptions = {"Easy", "Medium", "Hard"};

public:
    explicit Menu(Game *game);
    void handleInput(const sf::Event &event);
    void changeDifficulty(int change);
    const sf::Text &getStartText() const;
    const sf::Text &getDifficultyText() const;
};

class Renderer {
private:
    sf::RenderWindow& window;
    sf::Font uiFont;
    sf::Text flagCounterText;
    sf::Text timerText;
    sf::Text endGameText;

    static sf::Texture hiddenTexture;
    static sf::Texture mineTexture;
    static sf::Texture flagTexture;
    static std::vector<sf::Texture> numberTextures;

public:
    explicit Renderer(sf::RenderWindow& win) : window(win) {
        if (!uiFont.loadFromFile("Lato-Black.ttf")) {
            std::cerr << "Failed to load UI font." << std::endl;
        }
        initializeUIText();
        loadTextures();
    }

    static void loadTextures() {
        if (!hiddenTexture.loadFromFile("Cell.png")) std::cerr << "Failed to load Cell texture" << std::endl;
        if (!mineTexture.loadFromFile("Bomb.png")) std::cerr << "Failed to load Bomb texture" << std::endl;
        if (!flagTexture.loadFromFile("Flag.png")) std::cerr << "Failed to load Flag texture" << std::endl;

        numberTextures.resize(8);
        for (int i = 0; i < 8; ++i) {
            if (!numberTextures[i].loadFromFile("number" + std::to_string(i + 1) + ".png"))
                std::cerr << "Failed to load number texture: " << i + 1 << std::endl;
        }
    }

    void initializeUIText() {
        flagCounterText.setFont(uiFont);
        flagCounterText.setCharacterSize(24);
        flagCounterText.setFillColor(sf::Color::White);
        flagCounterText.setPosition(10, window.getSize().y - 50); // Adjusted for bottom

        timerText.setFont(uiFont);
        timerText.setCharacterSize(24);
        timerText.setFillColor(sf::Color::White);
        timerText.setPosition(window.getSize().x - 110, window.getSize().y - 50); // Adjusted for bottom

        endGameText.setFont(uiFont);
        endGameText.setCharacterSize(30);
        endGameText.setFillColor(sf::Color::Red);
        endGameText.setPosition(window.getSize().x / 2.f - endGameText.getGlobalBounds().width / 2,
                                window.getSize().y / 2.f - endGameText.getGlobalBounds().height / 2);
    }

    void drawBoard(const Board& board) {
        const auto& cells = board.getCells();
        float cellWidth = window.getSize().x / static_cast<float>(board.getWidth());
        float cellHeight = (window.getSize().y - 100) / static_cast<float>(board.getHeight());

        for (size_t y = 0; y < cells.size(); ++y) {
            for (size_t x = 0; x < cells[y].size(); ++x) {
                sf::Sprite sprite;
                sprite.setPosition(x * cellWidth, y * cellHeight);
                sprite.setScale(cellWidth / hiddenTexture.getSize().x, cellHeight / hiddenTexture.getSize().y);

                switch(cells[y][x].getState()) {
                    case CellState::Hidden:
                        sprite.setTexture(hiddenTexture);
                        break;
                    case CellState::Revealed:
                        if(cells[y][x].containsMine()) {
                            sprite.setTexture(mineTexture);
                        } else {
                            int adjacentMines = cells[y][x].getAdjacentMines();
                            if(adjacentMines > 0) {
                                sprite.setTexture(numberTextures[adjacentMines - 1]);
                            } else {
                                sprite.setTextureRect(sf::IntRect(0, 0, 1, 1)); // Empty cell
                                sprite.setColor(sf::Color::White);
                                sprite.setScale(cellWidth, cellHeight); // Fill cell
                            }
                        }
                        break;
                    case CellState::Flagged:
                        sprite.setTexture(flagTexture);
                        break;
                }
                window.draw(sprite);
            }
        }
    }

    void drawMenu(const Menu& menu) {
        window.draw(menu.getStartText());
        window.draw(menu.getDifficultyText());
    }

    void updateFlagCounter(int flags) {
        flagCounterText.setString("Flags: " + std::to_string(flags));
    }

    void updateTimer(float time) {
        timerText.setString("Time: " + std::to_string(static_cast<int>(time)));
    }

    void setEndGameMessage(const std::string& message) {
        endGameText.setString(message);
        window.draw(endGameText); // Draw immediately to display the message
    }

    void drawUI(int flags, float time) {
        updateFlagCounter(flags);
        updateTimer(time);
        window.draw(flagCounterText);
        window.draw(timerText);
        // Do not draw endGameText here as it should be drawn only when the game ends
    }

    void drawEndGameMessage() {
        if (!endGameText.getString().isEmpty()) {
            endGameText.setPosition(
                    (window.getSize().x - endGameText.getLocalBounds().width) / 2.f,
                    (window.getSize().y - endGameText.getLocalBounds().height) / 2.f
            );
            window.draw(endGameText);
        }
    }
};

// Define static members
sf::Texture Renderer::hiddenTexture;
sf::Texture Renderer::mineTexture;
sf::Texture Renderer::flagTexture;
std::vector<sf::Texture> Renderer::numberTextures;


class Game {
private:
    sf::RenderWindow window;
    Timer timer;
    Board* board;
    Menu menu;
    Renderer renderer;
    Difficulty difficulty;
    bool game_over;

    static constexpr int CELL_WIDTH = 30; // Adjusted to better fill the screen
    static constexpr int CELL_HEIGHT = 30; // Adjusted to better fill the screen
    static constexpr int UI_HEIGHT = 100; // Space for UI elements like timer and flag count

    int flagCount;
    float elapsedTime;

public:
    Game() : window(sf::VideoMode(WIDTH, HEIGHT), "Minesweeper", sf::Style::Close),
             board(nullptr), menu(this), renderer(window), game_over(true),
             flagCount(0), elapsedTime(0) {
        window.setFramerateLimit(60); // Limit the framerate to 60 frames per second
    }

    ~Game() {
        delete board;
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
        } else if (game_over) {
            menu.handleInput(event);
        } else if (event.type == sf::Event::MouseButtonPressed) {
            handleMouseInput(event.mouseButton);
        }
    }

    void handleMouseInput(const sf::Event::MouseButtonEvent& mouseEvent) {
        if (!board) return;

        int x, y;
        std::tie(x, y) = convertToBoardCoordinates(mouseEvent.x, mouseEvent.y);

        if (mouseEvent.button == sf::Mouse::Left && board->isFirstClick()) {
            board->firstReveal(x, y);
            updateFlagCount();
        } else if (mouseEvent.button == sf::Mouse::Left) {
            board->revealCell(x, y);
            updateFlagCount();
        } else if (mouseEvent.button == sf::Mouse::Right) {
            board->flagCell(x, y);
            updateFlagCount();
        }
        checkGameState();
    }

    void updateFlagCount() {
        if (board) {
            flagCount = board->countFlaggedCells();
        }
    }

    std::pair<int, int> convertToBoardCoordinates(int mouseX, int mouseY) {
        int x = mouseX / CELL_WIDTH;
        int y = (mouseY - UI_HEIGHT) / CELL_HEIGHT; // Adjust y coordinate for UI_HEIGHT
        return {x, y};
    }

    void startGame(Difficulty chosenDifficulty) {
        difficulty = chosenDifficulty;
        setupBoard(difficulty);
        timer.start();
        game_over = false;
        flagCount = 0;
        elapsedTime = 0;
    }

    void setupBoard(Difficulty difficulty) {
        int width, height, mines;
        switch (difficulty) {
            case Difficulty::Easy:
                width = 10; height = 8; mines = 10;
                break;
            case Difficulty::Medium:
                width = 16; height = 16; mines = 40;
                break;
            case Difficulty::Hard:
                width = 24; height = 20; mines = 99;
                break;
        }
        window.setSize(sf::Vector2u(width * CELL_WIDTH, (height * CELL_HEIGHT) + UI_HEIGHT));
        delete board;
        board = new Board(width, height, mines);
    }

    void endGame(bool won) {
        timer.stop();
        game_over = true;
        renderer.setEndGameMessage(won ? "You Won!" : "Game Over");
        // Draw the end game message immediately
        renderer.drawEndGameMessage();
    }

    void checkGameState() {
        if (board && board->checkWinCondition()) {
            endGame(true);
        } else if (board && board->checkLossCondition()) {
            endGame(false);
        }
    }

private:
    void update() {
        if (!game_over) {
            elapsedTime = timer.getElapsedTime();
        }
    }

    void render() {
        window.clear();
        if (!game_over && board) {
            renderer.drawBoard(*board);
        } else {
            renderer.drawMenu(menu);
        }
        renderer.drawUI(flagCount, elapsedTime); // Pass flags and time
        if (game_over) {
            renderer.drawEndGameMessage(); // Call this method to draw the message
        }
        window.display();
    }
};

Menu::Menu(Game *game) : game(game), currentDifficulty(Difficulty::Easy) {
    if (!font.loadFromFile("font/Lato-Black.ttf")) {
        std::cerr << "Failed to load font." << std::endl;
    }

    startText.setFont(font);
    startText.setString("Start Game");
    startText.setCharacterSize(24);
    startText.setFillColor(sf::Color::White);
    startText.setPosition(WIDTH / 2.f, HEIGHT / 2.f);

    difficultyText.setFont(font);
    difficultyText.setString("Difficulty: Easy"); // Default difficulty
    difficultyText.setCharacterSize(24);
    difficultyText.setFillColor(sf::Color::White);
    difficultyText.setPosition(WIDTH / 2.f, HEIGHT / 2.f + 60.f);
}

void Menu::handleInput(const sf::Event &event) {
    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Up:
                changeDifficulty(-1);
                break;
            case sf::Keyboard::Down:
                changeDifficulty(1);
                break;
            case sf::Keyboard::Enter:
                game->startGame(currentDifficulty);
                break;
        }
    }
}

void Menu::changeDifficulty(int change) {
    int difficultyIndex = static_cast<int>(currentDifficulty) + change;
    if (difficultyIndex >= static_cast<int>(difficultyOptions.size())) {
        difficultyIndex = 0;
    } else if (difficultyIndex < 0) {
        difficultyIndex = difficultyOptions.size() - 1;
    }
    currentDifficulty = static_cast<Difficulty>(difficultyIndex);
    difficultyText.setString("Difficulty: " + difficultyOptions[difficultyIndex]);
}

const sf::Text &Menu::getStartText() const {
    return startText;
}

const sf::Text &Menu::getDifficultyText() const {
    return difficultyText;
}


int main() {
    Game minesweeper;
    minesweeper.run();
    return 0;
}
