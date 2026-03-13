#include <iostream>
#include <conio.h>
#include <windows.h>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <cstdio>

#define SCREEN_WIDTH 90
#define SCREEN_HEIGHT 26
#define WIN_WIDTH 80

using namespace std;

HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
COORD CursorPosition;

void gotoxy(int x, int y)
{
    CursorPosition.X = x;
    CursorPosition.Y = y;
    SetConsoleCursorPosition(console, CursorPosition);
}

void setcursor(bool visible, DWORD size)
{
    CONSOLE_CURSOR_INFO lpCursor;
    lpCursor.bVisible = visible;
    lpCursor.dwSize = size;
    SetConsoleCursorInfo(console, &lpCursor);
}

void drawBorder()
{
    for (int i = 17; i < WIN_WIDTH; i++)
    {
        gotoxy(i, 0);
        cout << "#";
        gotoxy(i, SCREEN_HEIGHT - 1);
        cout << "#";
    }
    for (int i = 0; i < SCREEN_HEIGHT; i++)
    {
        gotoxy(17, i);
        cout << "#";
        gotoxy(WIN_WIDTH - 1, i);
        cout << "#";
    }
}

class Entity
{
protected:
    int x, y;

public:
    Entity(int startX = 0, int startY = 0) : x(startX), y(startY) {}

    virtual void draw() = 0;
    virtual void erase() = 0;
    virtual void move() = 0;

    int getX() const { return x; }
    int getY() const { return y; }
    void setX(int newX) { x = newX; }
    void setY(int newY) { y = newY; }
};

class Plane : public Entity
{
    char design[3][3] = {
        {' ', '^', ' '},
        {'/', '|', '\\'},
        {'^', '-', '^'}};

public:
    Plane() : Entity(WIN_WIDTH / 2, SCREEN_HEIGHT - 5) {}

    void draw()
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                gotoxy(x + j, y + i), cout << design[i][j];
    }

    void erase()
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                gotoxy(x + j, y + i), cout << " ";
    }

    void moveLeft()
    {
        erase();
        if (x > 18) x -= 2;
        draw();
    }

    void moveRight()
    {
        erase();
        if (x < WIN_WIDTH - 5) x += 2;
        draw();
    }

    void move() override {}
};

class Bullet
{
    int x, y;

public:
    Bullet(int startX, int startY) : x(startX), y(startY) {}

    void draw() { gotoxy(x, y); cout << "^"; }

    void erase() { gotoxy(x, y); cout << " "; }

    void move()
    {
        erase();
        y--;
        if (y > 1) draw();
    }

    bool offScreen() const { return y < 1; }

    int getX() const { return x; }
    int getY() const { return y; }
};

class Enemy : public Entity
{
public:
    bool active = false;

    Enemy() : Entity(0, 0) {}

    void draw()
    {
        gotoxy(x, y);
        cout << " ^ ";
        gotoxy(x, y + 1);
        cout << "<\">";
    }

    void erase()
    {
        gotoxy(x, y);
        cout << "   ";
        gotoxy(x, y + 1);
        cout << "   ";
    }

    void move()
    {
        erase();
        y++;
        if (y < SCREEN_HEIGHT - 3) draw();
    }

    void reset()
    {
        x = 18 + rand() % (WIN_WIDTH - 36);
        y = 1;
        active = true;
        draw();
    }

    bool offScreen() const { return y >= SCREEN_HEIGHT - 2; }

    bool hitPlane(const Plane &p) const
    {
        return (p.getY() <= y + 1 && p.getY() + 2 >= y &&
                p.getX() <= x + 2 && p.getX() + 2 >= x);
    }

    bool hitBullet(const Bullet &b) const
    {
        return (b.getX() >= x && b.getX() <= x + 2 &&
                b.getY() >= y && b.getY() <= y + 1);
    }
};

class Game
{
    Plane player;
    vector<Bullet> bullets;
    Enemy enemies[2];

    int score = 0;
    int highScore = 0;
    int speed = 120;
    DWORD lastShootTime = 0;
    const DWORD shootDelay = 150;

public:
    void loadHighScore()
    {
        FILE *f = fopen("output/highscore.txt", "r");
        if (f) {
            fscanf(f, "%d", &highScore);
            fclose(f);
        }
    }

    void saveHighScore()
    {
        FILE *f = fopen("output/highscore.txt", "w");
        if (f) {
            fprintf(f, "%d", highScore);
            fclose(f);
        }
    }

    void start()
    {
        system("cls");
        setcursor(false, 0);
        srand(time(0));

        drawBorder();
        player.draw();
        loadHighScore();

        // Initial enemies
        for (int i = 0; i < 2; ++i) enemies[i].reset();

        while (true)
        {
            // Input with repeat support
            while (_kbhit())
            {
                int code = _getch();
                if (code == 0 || code == 224) {
                    code = _getch();
                    switch (code)
                    {
                    case 75: player.moveLeft(); break;  // Left
                    case 77: player.moveRight(); break; // Right
                    case 72: // Up - rapid shoot
                        if (GetTickCount() - lastShootTime > shootDelay) {
                            bullets.emplace_back(player.getX() + 1, player.getY() - 1);
                            lastShootTime = GetTickCount();
                        }
                        break;
                    }
                } else switch(code) {
                    case 'a': case 'A': player.moveLeft(); break;
                    case 'd': case 'D': player.moveRight(); break;
                    case 'w': case 'W': // Rapid shoot
                        if (GetTickCount() - lastShootTime > shootDelay) {
                            bullets.emplace_back(player.getX() + 1, player.getY() - 1);
                            lastShootTime = GetTickCount();
                        }
                        break;
                }
            }

            // Bullets update
            for (int i = bullets.size(); i--; ) {
                bullets[i].move();
                if (bullets[i].offScreen()) {
                    bullets[i] = move(bullets.back());
                    bullets.pop_back();
                    continue;
                }
                for (int j = 0; j < 2; ++j) {
                    if (enemies[j].active && enemies[j].hitBullet(bullets[i])) {
                        enemies[j].erase();
                        enemies[j].active = false;
                        bullets[i] = move(bullets.back());
                        bullets.pop_back();
                        score += 10;
                        break;
                    }
                }
            }

            // Enemies update
            for (int i = 0; i < 2; ++i) {
                if (!enemies[i].active) {
                    if (rand() % 30 == 0) enemies[i].reset();
                    continue;
                }
                enemies[i].move();
                if (enemies[i].offScreen()) {
                    enemies[i].erase();
                    enemies[i].active = false;
                    continue;
                }
                if (enemies[i].hitPlane(player)) {
                    system("cls");
                    cout << "GAME OVER\\nScore: " << score << "\\nHigh Score: " << highScore << endl;
                    if (score > highScore) {
                        highScore = score;
                        saveHighScore();
                    }
                    return;
                }
            }

            gotoxy(2, 2);
            cout << "Score: " << score << "    ";

            Sleep(speed);
            if (score > 0 && score % 100 == 0 && speed > 30) speed -= 5;
        }
    }
};

int main()
{
    Game game;
    game.start();
    return 0;
}

