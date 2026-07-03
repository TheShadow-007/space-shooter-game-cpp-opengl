// Place this at the top
#include <windows.h>
#include <mmsystem.h>
#include "glut.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

// --- Structures ---
struct Bullet {
    float x, y;
};

struct Enemy {
    float x, y, speed;
};

struct Asteroid {
    float x, y, speed;
};

struct Star {
    float x, y, speed;
};

struct Boss {
    float x, y;
    int health;
    bool active;
};

struct Shockwave {
    float x, y;
    float radius;
    bool active;
};

// --- Globals ---
float shipX = 0.0f, shipY = -0.8f, shipSpeed = 0.02f;
bool booster = false, paused = false, gameOver = false;

std::vector<Bullet> bullets, bossBullets;
std::vector<Enemy> enemies;
std::vector<Asteroid> asteroids;
std::vector<Star> stars;
std::vector<Shockwave> shockwaves;

Boss boss = { 0.0f, 1.2f, 50, false };

int score = 0, level = 1, enemiesDestroyed = 0;

// --- Utility Functions ---
void playSound(const char* file, bool async = true) {
    PlaySoundA(file, NULL, SND_FILENAME | (async ? SND_ASYNC : SND_SYNC));
}

void drawText(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

bool checkCollision(Bullet b, float x, float y, float size) {
    return fabs(b.x - x) < size && fabs(b.y - y) < size;
}

bool checkShipCollision(float x, float y, float size = 0.07f) {
    return fabs(shipX - x) < size && fabs(shipY - y) < size;
}

void drawShockwave(const Shockwave& s) {
    if (!s.active) return;
    glColor3f(0.5f, 0.8f, 1.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; i += 10) {
        float angle = i * 3.14f / 180.0f;
        glVertex2f(s.x + s.radius * cos(angle), s.y + s.radius * sin(angle));
    }
    glEnd();
}

// --- Draw Entities (drawShip, drawBullet, etc. are unchanged from your code) ---
void drawShip() {
    glColor3f(0.0f, 0.5f, 1.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(shipX, shipY);
    glVertex2f(shipX - 0.05f, shipY - 0.1f);
    glVertex2f(shipX + 0.05f, shipY - 0.1f);
    glEnd();
    if (booster) {
        float flameLength = 0.18f + (rand() % 5) * 0.005f;
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.5f, 0.0f);
        glVertex2f(shipX, shipY - 0.1f);
        glColor3f(1.0f, 1.0f, 0.0f);
        glVertex2f(shipX - 0.02f, shipY - flameLength);
        glVertex2f(shipX + 0.02f, shipY - flameLength);
        glEnd();
    }
}

void drawBullet(Bullet b) {
    glColor3f(1, 1, 0);
    glBegin(GL_QUADS);
    glVertex2f(b.x - 0.005f, b.y);
    glVertex2f(b.x + 0.005f, b.y);
    glVertex2f(b.x + 0.005f, b.y + 0.05f);
    glVertex2f(b.x - 0.005f, b.y + 0.05f);
    glEnd();
}

void drawBossBullet(Bullet b) {
    glColor3f(1, 0, 0);
    glBegin(GL_QUADS);
    glVertex2f(b.x - 0.005f, b.y);
    glVertex2f(b.x + 0.005f, b.y);
    glVertex2f(b.x + 0.005f, b.y - 0.05f);
    glVertex2f(b.x - 0.005f, b.y - 0.05f);
    glEnd();
}

void drawEnemy(Enemy e) {
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(e.x, e.y);
    glVertex2f(e.x - 0.05f, e.y + 0.08f);
    glVertex2f(e.x + 0.05f, e.y + 0.08f);
    glEnd();
}

void drawAsteroid(Asteroid a) {
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i += 20) {
        float rad = i * 3.14f / 180.0f;
        glVertex2f(a.x + 0.035f * cos(rad), a.y + 0.035f * sin(rad));
    }
    glEnd();
}

void drawStar(Star s) {
    glColor3f(1, 1, 1);
    glPointSize(1);
    glBegin(GL_POINTS);
    glVertex2f(s.x, s.y);
    glEnd();
}

void drawBoss() {
    if (!boss.active) return;
    glColor3f(0.6f, 0.0f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(boss.x - 0.2f, boss.y);
    glVertex2f(boss.x + 0.2f, boss.y);
    glVertex2f(boss.x + 0.2f, boss.y - 0.1f);
    glVertex2f(boss.x - 0.2f, boss.y - 0.1f);
    glEnd();
    glColor3f(1, 0, 0);
    glBegin(GL_QUADS);
    glVertex2f(-0.9f, 0.95f);
    glVertex2f(-0.9f + (boss.health / 100.0f) * 1.8f, 0.95f);
    glVertex2f(-0.9f + (boss.health / 100.0f) * 1.8f, 0.92f);
    glVertex2f(-0.9f, 0.92f);
    glEnd();
}

// --- Display ---
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    for (Star& s : stars) drawStar(s);
    if (gameOver) {
        drawText(-0.2f, 0.0f, "GAME OVER");
        drawText(-0.35f, -0.1f, "Press R to Restart or ESC to Exit");
        glutSwapBuffers(); return;
    }
    if (paused) {
        drawText(-0.1f, 0.0f, "PAUSED");
        glutSwapBuffers(); return;
    }

    drawShip();
    for (Bullet& b : bullets) drawBullet(b);
    for (Bullet& b : bossBullets) drawBossBullet(b);
    for (Enemy& e : enemies) drawEnemy(e);
    for (Asteroid& a : asteroids) drawAsteroid(a);
    if (boss.active) drawBoss();
    for (Shockwave& s : shockwaves) drawShockwave(s);

    drawText(-0.95f, 0.9f, "Score: " + std::to_string(score) + " Level: " + std::to_string(level));
    glutSwapBuffers();
}

// --- Update Logic ---
void update(int value) {
    if (!paused && !gameOver) {
        for (Bullet& b : bullets) b.y += 0.05f;
        for (Bullet& b : bossBullets) b.y -= 0.03f;
        for (Enemy& e : enemies) e.y -= e.speed;
        for (Asteroid& a : asteroids) a.y -= a.speed;
        for (Star& s : stars) {
            s.y -= s.speed;
            if (s.y < -1.0f) {
                s.y = 1.0f;
                s.x = (rand() % 200 - 100) / 100.0f;
            }
        }
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](Bullet b) { return b.y > 1.0f; }), bullets.end());
        bossBullets.erase(std::remove_if(bossBullets.begin(), bossBullets.end(), [](Bullet b) { return b.y < -1.0f; }), bossBullets.end());

        for (auto b = bullets.begin(); b != bullets.end();) {
            bool hit = false;
            for (auto e = enemies.begin(); e != enemies.end();) {
                if (checkCollision(*b, e->x, e->y, 0.05f)) {.
                    playSound("explosion.wav");
                    b = bullets.erase(b);
                    e = enemies.erase(e);
                    score += 10;
                    enemiesDestroyed++;
                    hit = true;
                    break;
                } else ++e;
            }
            if (!hit && boss.active && checkCollision(*b, boss.x, boss.y - 0.05f, 0.25f)) {
                boss.health -= 2;
                b = bullets.erase(b);
                if (boss.health <= 0) {
                    boss.active = false;
                    score += 100;
                }
                hit = true;
            }
            if (!hit) ++b;
        }

        for (Bullet& b : bossBullets)
            if (checkShipCollision(b.x, b.y)) gameOver = true;

        if (enemiesDestroyed >= level * 5 && !boss.active) {
            level++;
            enemiesDestroyed = 0;
            if (level % 3 == 0) {
                boss.active = true;
                boss.health = 50 + level * 5;
                boss.y = 1.0f;
            }
        }

        if (boss.active && boss.y > 0.7f) boss.y -= 0.01f;
        if (rand() % 20 < level && !boss.active)
            enemies.push_back({ (float)(rand() % 200 - 100) / 100, 1.0f, 0.01f + level * 0.002f });
        if (rand() % 50 < level)
            asteroids.push_back({ (float)(rand() % 200 - 100) / 100, 1.0f, 0.01f });

        for (Enemy& e : enemies) if (checkShipCollision(e.x, e.y)) gameOver = true;
        for (Asteroid& a : asteroids) if (checkShipCollision(a.x, a.y)) gameOver = true;
        if (boss.active && checkShipCollision(boss.x, boss.y, 0.25f)) gameOver = true;

        if (boss.active && rand() % 40 == 0)
            bossBullets.push_back({ boss.x, boss.y - 0.1f });

        for (auto& s : shockwaves) {
            if (s.active) {
                s.radius += 0.02f;
                if (s.radius > 0.3f) s.active = false;
            }
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// --- Keyboard ---
void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0);
    if (key == ' ') {
        bullets.push_back({ shipX, shipY });
        playSound("laser.wav");
    }
    if (key == 'p' || key == 'P') paused = !paused;
    if (key == 'w') booster = true;
    if (key == 'a') shipX -= booster ? shipSpeed * 2 : shipSpeed;
    if (key == 'd') shipX += booster ? shipSpeed * 2 : shipSpeed;
    if (key == 'r' || key == 'R') {
        if (gameOver) {
            gameOver = false; paused = false;
            score = 0; level = 1; enemiesDestroyed = 0;
            shipX = 0.0f;
            bullets.clear(); bossBullets.clear();
            enemies.clear(); asteroids.clear(); shockwaves.clear();
            boss.active = false;
        }
    }
    if (key == 'm' || key == 'M') {
        for (Bullet& b : bullets) {
            shockwaves.push_back({ b.x, b.y, 0.05f, true });
            for (auto it = enemies.begin(); it != enemies.end();) {
                float dx = it->x - b.x, dy = it->y - b.y;
                if (sqrt(dx * dx + dy * dy) < 0.2f) {
                    it = enemies.erase(it);
                    score += 10;
                    enemiesDestroyed++;
                } else ++it;
            }
        }
        bullets.clear();
        playSound("blast.wav");
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    if (key == 'w') booster = false;
}

// --- Init & Main ---
void init() {
    glClearColor(0, 0, 0, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);
    for (int i = 0; i < 100; ++i)
        stars.push_back({ (rand() % 200 - 100) / 100.0f, (rand() % 200 - 100) / 100.0f, 0.002f + (rand() % 10) / 5000.0f });
    playSound("bgm.wav");
}

int main(int argc, char** argv) {
    srand((unsigned int)time(0));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Space Shooter Game");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutTimerFunc(0, update, 0);
    glutMainLoop();
    return 0;
}