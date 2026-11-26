// Kontrol: a = kiri | d = kanan | s = tembak | . = diam | q = keluar
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

const int WIDTH = 15;
const int HEIGHT = 10;
const int BULLET_POOL_SIZE = 5;
const int ENEMY_POOL_SIZE = 6;
const int SPAWN_EVERY = 3;      // musuh muncul tiap 3 frame
const int ENEMY_MOVE_EVERY = 2; // musuh turun tiap 2 frame

// BULLET (Object Pool)
class Bullet {
private:
    bool active = false;
    int x = 0, y = 0;

public:
    void shoot(int startX, int startY) { active = true; x = startX; y = startY; }
    void reset() { active = false; }

    void update() {
        if (!active) return;
        if (--y < 0) reset(); // keluar arena -> kembali ke pool
    }

    bool isActive() const { return active; }
    int getX() const { return x; }
    int getY() const { return y; }
};

// BULLET POOL (Object Pool)
class BulletPool {
private:
    vector<Bullet> bullets;

public:
    BulletPool(int size) : bullets(size) {}

    Bullet *getBullet() {
        for (Bullet &b : bullets) if (!b.isActive()) return &b;
        return nullptr;
    }

    void update() { for (Bullet &b : bullets) b.update(); }
    vector<Bullet> &getBullets() { return bullets; }

    int countActive() const {
        int n = 0;
        for (const Bullet &b : bullets) if (b.isActive()) n++;
        return n;
    }
};

// ENEMY (Object Pool)
class Enemy {
private:
    bool active = false;
    int x = 0, y = 0;

public:
    void spawn(int startX) { active = true; x = startX; y = 0; }
    void defeat() { active = false; }
    void update() { if (active) y++; } // musuh turun 1 baris

    bool isActive() const { return active; }
    int getX() const { return x; }
    int getY() const { return y; }
};

// PLAYER (Receiver)
class Player {
private:
    BulletPool *bulletPool;

public:
    int x = WIDTH / 2;
    int y = HEIGHT - 1;

    Player(BulletPool *pool) : bulletPool(pool) {}

    void moveLeft() { if (x > 0) x--; }
    void moveRight() { if (x < WIDTH - 1) x++; }

    void shoot() {
        Bullet *b = bulletPool->getBullet(); // ambil dari pool
        if (b != nullptr) b->shoot(x, y - 1);
    }
};

// COMMAND INTERFACE
class Command {
public:
    virtual ~Command() {}
    virtual void execute() = 0;
};

// CONCRETE COMMANDS
class MoveLeftCommand : public Command {
private:
    Player *player;
public:
    MoveLeftCommand(Player *p) : player(p) {}
    void execute() override { player->moveLeft(); }
};

class MoveRightCommand : public Command {
private:
    Player *player;
public:
    MoveRightCommand(Player *p) : player(p) {}
    void execute() override { player->moveRight(); }
};

class ShootCommand : public Command {
private:
    Player *player;
public:
    ShootCommand(Player *p) : player(p) {}
    void execute() override { player->shoot(); }
};

// NULL COMMAND (default tombol, tidak melakukan apa-apa)
class NullCommand : public Command {
public:
    void execute() override {}
};

// INPUT HANDLER / INVOKER
class InputHandler {
private:
    Command *nullCommand = new NullCommand();
    Command *keyA = nullCommand;
    Command *keyD = nullCommand;
    Command *keyS = nullCommand;

public:
    ~InputHandler() { delete nullCommand; }

    void setKeyA(Command *command) { keyA = command; }
    void setKeyD(Command *command) { keyD = command; }
    void setKeyS(Command *command) { keyS = command; }

    void handleInput(char input) {
        if (input == 'a') keyA->execute();
        else if (input == 'd') keyD->execute();
        else if (input == 's') keyS->execute();
    }
};

// GAME (Game Loop: Input -> Update -> Render)
class Game {
private:
    BulletPool bulletPool;
    vector<Enemy> enemies;
    Player player;
    InputHandler inputHandler;

    Command *moveLeft;
    Command *moveRight;
    Command *shoot;

    bool running = true;
    int frame = 0;
    int score = 0;
    int lives = 3;

    void processInput() {
        char input;
        cout << "\nInput: ";
        if (!(cin >> input) || input == 'q') running = false;
        else inputHandler.handleInput(input);
    }

    void spawnEnemy() {
        for (Enemy &e : enemies) {
            if (!e.isActive()) { e.spawn(rand() % WIDTH); return; }
        }
    }

    void checkCollision() {
        for (Bullet &b : bulletPool.getBullets()) {
            if (!b.isActive()) continue;
            for (Enemy &e : enemies) {
                if (e.isActive() && e.getX() == b.getX() && e.getY() == b.getY()) {
                    e.defeat(); // musuh mati
                    b.reset();  // peluru kembali ke pool
                    score += 10;
                    break;
                }
            }
        }
    }

    void update() {
        frame++;
        if (frame % SPAWN_EVERY == 0) spawnEnemy();

        bulletPool.update();
        checkCollision();

        if (frame % ENEMY_MOVE_EVERY == 0) {
            for (Enemy &e : enemies) e.update();
            checkCollision();
        }

        for (Enemy &e : enemies) {
            if (e.isActive() && e.getY() >= HEIGHT - 1) { e.defeat(); lives--; } // musuh lolos
        }
        if (lives <= 0) running = false;
    }

    void render() {
        vector<string> grid(HEIGHT, string(WIDTH, '.'));

        int enemyCount = 0;
        for (Enemy &e : enemies) {
            if (e.isActive()) { grid[e.getY()][e.getX()] = 'E'; enemyCount++; }
        }
        for (Bullet &b : bulletPool.getBullets()) {
            if (b.isActive()) grid[b.getY()][b.getX()] = '|';
        }
        grid[player.y][player.x] = 'R';

        string border = "+" + string(WIDTH, '-') + "+\n";
        cout << "\n=== FRAME " << frame << " ===\n" << border;
        for (string &row : grid) cout << "|" << row << "|\n";
        cout << border;
        cout << "Skor   : " << score << "   Nyawa : " << lives << "\n";
        cout << "Musuh  : " << enemyCount << " aktif\n";
        cout << "Peluru : " << bulletPool.countActive() << " aktif, "
             << (BULLET_POOL_SIZE - bulletPool.countActive()) << " siap di pool\n";
    }

public:
    Game() : bulletPool(BULLET_POOL_SIZE), enemies(ENEMY_POOL_SIZE), player(&bulletPool) {
        moveLeft = new MoveLeftCommand(&player);
        moveRight = new MoveRightCommand(&player);
        shoot = new ShootCommand(&player);

        inputHandler.setKeyA(moveLeft);
        inputHandler.setKeyD(moveRight);
        inputHandler.setKeyS(shoot);

        srand((unsigned)time(nullptr));
    }

    ~Game() {
        delete moveLeft;
        delete moveRight;
        delete shoot;
    }

    void run() {
        cout << "=== ROBOT DEFENSE ARENA ===\n";
        cout << "a = kiri, d = kanan, s = tembak, . = diam, q = keluar\n";
        render();

        while (running) {
            processInput();      // 1. INPUT
            if (!running) break;
            update();            // 2. UPDATE
            render();            // 3. RENDER
        }

        cout << "\n=== GAME SELESAI ===\nSkor akhir: " << score << endl;
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}