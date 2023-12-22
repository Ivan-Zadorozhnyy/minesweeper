#include <vector>
#include <chrono>
#include <random>
#include <iostream>
#include <fstream>

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Game;
class Menu;
class Renderer;

constexpr int WIDTH = 1280;
constexpr int HEIGHT = 720;

enum class Difficulty {
    Easy = 1, Medium, Hard
};

enum class CellState{
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
            adjacentMines = 0;
        }
    }

    void incrementAdjacentMines() {
        if (!isMine) {
            ++adjacentMines;
        }
    }

    void serialize(std::ostream& os) const {
        os.write(reinterpret_cast<const char*>(&state), sizeof(state));
        os.write(reinterpret_cast<const char*>(&isMine), sizeof(isMine));
        os.write(reinterpret_cast<const char*>(&adjacentMines), sizeof(adjacentMines));
    }

    void deserialize(std::istream& is) {
        is.read(reinterpret_cast<char*>(&state), sizeof(state));
        is.read(reinterpret_cast<char*>(&isMine), sizeof(isMine));
        is.read(reinterpret_cast<char*>(&adjacentMines), sizeof(adjacentMines));
    }

    bool containsMine() const { return isMine; }
    int getAdjacentMines() const { return adjacentMines; }
    CellState getState() const { return state; }
    bool isRevealed() const { return state == CellState::Revealed; }
    bool isFlagged() const { return state == CellState::Flagged; }
    bool getIsMine() const { return isMine; }
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

    int getMineCount() const {
        return mineCount;
    }

    const std::vector<std::vector<Cell>>& getCells() const {
        return cells;
    }

    void serialize(std::ostream& os) const {
        os.write(reinterpret_cast<const char*>(&width), sizeof(width));
        os.write(reinterpret_cast<const char*>(&height), sizeof(height));
        os.write(reinterpret_cast<const char*>(&mineCount), sizeof(mineCount));
        os.write(reinterpret_cast<const char*>(&firstClick), sizeof(firstClick));
        for (const auto& row : cells) {
            for (const auto& cell : row) {
                cell.serialize(os);
            }
        }
    }

    void deserialize(std::istream& is) {
        is.read(reinterpret_cast<char*>(&width), sizeof(width));
        is.read(reinterpret_cast<char*>(&height), sizeof(height));
        is.read(reinterpret_cast<char*>(&mineCount), sizeof(mineCount));
        is.read(reinterpret_cast<char*>(&firstClick), sizeof(firstClick));
        cells.resize(height, std::vector<Cell>(width));
        for (auto& row : cells) {
            for (auto& cell : row) {
                cell.deserialize(is);
            }
        }
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
        if (!uiFont.loadFromFile("font/Lato-Black.ttf")) {
            std::cerr << "Failed to load UI font." << std::endl;
        }
        initializeUIText();
        loadTextures();
    }

    static void loadTextures() {
        if (!hiddenTexture.loadFromFile("sprites/Cell.png")) std::cerr << "Failed to load Cell texture" << std::endl;
        if (!mineTexture.loadFromFile("sprites/Bomb.png")) std::cerr << "Failed to load Bomb texture" << std::endl;
        if (!flagTexture.loadFromFile("sprites/Flag.png")) std::cerr << "Failed to load Flag texture" << std::endl;

        numberTextures.resize(8);
        for (int i = 0; i < 8; ++i) {
            if (!numberTextures[i].loadFromFile("sprites/number" + std::to_string(i + 1) + ".png"))
                std::cerr << "Failed to load number texture: " << i + 1 << std::endl;
        }
    }

    void initializeUIText() {
        flagCounterText.setFont(uiFont);
        flagCounterText.setCharacterSize(24);
        flagCounterText.setFillColor(sf::Color::White);
        flagCounterText.setPosition(10, window.getSize().y - 50);

        timerText.setFont(uiFont);
        timerText.setCharacterSize(24);
        timerText.setFillColor(sf::Color::White);
        timerText.setPosition(window.getSize().x - 110, window.getSize().y - 50);

        endGameText.setFont(uiFont);
        endGameText.setCharacterSize(30);
        endGameText.setFillColor(sf::Color::Red);
        endGameText.setPosition(window.getSize().x / 2.f - endGameText.getGlobalBounds().width / 2,
                                window.getSize().y / 2.f - endGameText.getGlobalBounds().height / 2);
    }

    void drawBoard(const Board& board, int cellWidth, int cellHeight) {
        const auto& cells = board.getCells();

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
                                sprite.setTextureRect(sf::IntRect(0, 0, 1, 1));
                                sprite.setColor(sf::Color::White);
                                sprite.setScale(cellWidth, cellHeight);
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


    void updateTimer(float time) {
        timerText.setString("Time: " + std::to_string(static_cast<int>(time)));
    }

    void setEndGameMessage(const std::string& message) {
        endGameText.setString(message);
        endGameText.setPosition(window.getSize().x / 2.f - endGameText.getGlobalBounds().width / 2,
                                window.getSize().y / 2.f - endGameText.getGlobalBounds().height / 2 - 50);
        window.draw(endGameText);
    }

    void drawUI(int mineCount, int flags, float time) {
        int flagsLeft = mineCount - flags; // Calculate flags left
        flagCounterText.setString("Flags: " + std::to_string(flagsLeft));
        updateTimer(time);
        window.draw(flagCounterText);
        window.draw(timerText);
    }

    void drawEndGameMessage() {
        if (!endGameText.getString().isEmpty()) {
            endGameText.setPosition(
                    window.getSize().x / 2.f - endGameText.getGlobalBounds().width / 2,
                    window.getSize().y / 2.f - endGameText.getGlobalBounds().height / 2 - 50
            );
            window.draw(endGameText);
        }
    }
};

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
    int CELL_WIDTH;
    int CELL_HEIGHT;
    static constexpr int UI_HEIGHT = 100;
    int flagCount;
    float elapsedTime;
    bool isLoaded = false;

public:
    Game() : window(sf::VideoMode(WIDTH, HEIGHT), "Minesweeper", sf::Style::Close),
             board(nullptr), menu(this), renderer(window), game_over(true),
             flagCount(0), elapsedTime(0) {
        window.setFramerateLimit(60);
        //loadGame("savegame.bin");
    }

    ~Game() {
        if (isLoaded) {
            saveGame("savegame.bin");
        }
        delete board;
    }

    void run() {
        std::ifstream testFile("savegame.bin", std::ios::binary);
        if (testFile.good()) {
            loadGame("save.json");
        }
        testFile.close();

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                handleEvent(event);
                if (event.type == sf::Event::Closed) {
                    saveGame("savegame.bin");
                    window.close();
                }
            }

            if (!game_over) {
                update();
            }
            render();
        }
    }


    void handleEvent(const sf::Event& event) {
        if (event.type == sf::Event::Closed) {
            saveGame("save.json");
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
        int y = mouseY / CELL_HEIGHT;
        return {x, y};
    }

    void startGame(Difficulty chosenDifficulty) {
        difficulty = chosenDifficulty;
        setupBoard(difficulty);
        timer.reset();
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

        CELL_WIDTH = WIDTH / width;
        CELL_HEIGHT = (HEIGHT - UI_HEIGHT) / height;

        delete board;
        board = new Board(width, height, mines);
    }

    void endGame(bool won) {
        timer.stop();
        game_over = true;
        renderer.setEndGameMessage(won ? "You Won!" : "Game Over");
        renderer.drawEndGameMessage();
    }

    void checkGameState() {
        if (board && board->checkWinCondition()) {
            endGame(true);
        } else if (board && board->checkLossCondition()) {
            endGame(false);
        }
    }

    void update() {
        if (!game_over) {
            elapsedTime = timer.getElapsedTime();
        }
    }

    void render() {
        window.clear();
        if (!game_over && board) {
            renderer.drawBoard(*board, CELL_WIDTH, CELL_HEIGHT);
        } else {
            renderer.drawMenu(menu);
        }
        if (!game_over && board) {
            renderer.drawUI(board->getMineCount(), flagCount, elapsedTime);
        }
        if (game_over) {
            renderer.drawEndGameMessage();
        }
        window.display();
    }

    void saveGame(const std::string& filename) {
        std::ofstream file(filename);
        if (!file) {
            std::cerr << "Failed to open file for saving." << std::endl;
            return;
        }

        // --- serialize board --- //

        json jsonBoard;
        jsonBoard["width"] = board->getWidth();
        jsonBoard["height"] = board->getHeight();
        jsonBoard["mineCount"] = board->getMineCount();
        jsonBoard["firstClick"] = board->isFirstClick();

        json jsonCells = json::array();
        for (const auto& row : board->getCells()) {
            for (const auto& cell : row) {
                json jsonCell;
                jsonCell["state"] = cell.getState();
                jsonCell["isMine"] = cell.getIsMine();
                jsonCell["adjacentMines"] = cell.getAdjacentMines();
                jsonCells.push_back(jsonCell);
            }
        }

        jsonBoard["cells"] = jsonCells;

        // --- serialize game --- //

        json jsonGame;
        jsonGame["elapsedTime"] = elapsedTime;
        jsonGame["flagCount"] = flagCount;
        jsonGame["difficulty"] = static_cast<int>(difficulty);
        jsonGame["board"] = jsonBoard;

        file << std::setw(2) << jsonGame << std::endl;
        file.close();
    }

    void loadGame(const std::string& filename) {
        std::ifstream qfile(filename);
        json jsonSave;
        qfile >> jsonSave;
        // use jsonSave["game"]["board"]...
        // https://github.com/nlohmann/json?tab=readme-ov-file#tofrom-streams-eg-files-string-streams

        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "No save file found, starting a new game" << std::endl;
            return;
        }

        file.read(reinterpret_cast<char*>(&elapsedTime), sizeof(elapsedTime));
        file.read(reinterpret_cast<char*>(&flagCount), sizeof(flagCount));
        int diff;
        file.read(reinterpret_cast<char*>(&diff), sizeof(diff));
        difficulty = static_cast<Difficulty>(diff);
        game_over = false;

        int width, height, mines;
        bool first_click;
        file.read(reinterpret_cast<char*>(&width), sizeof(width));
        file.read(reinterpret_cast<char*>(&height), sizeof(height));
        file.read(reinterpret_cast<char*>(&mines), sizeof(mines));
        file.read(reinterpret_cast<char*>(&first_click), sizeof(first_click));

        if (board) delete board;
        board = new Board(width, height, mines);
        board->deserialize(file);
        file.close();

        isLoaded = true;
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
    difficultyText.setString("Difficulty: Easy");
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