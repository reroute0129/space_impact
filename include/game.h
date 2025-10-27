#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#define MAX_BULLETS 30
#define MAX_ENEMIES 30
#define MAX_ENEMY_BULLETS 30
#define MAX_POWERUPS 5
#define MAX_EXPLOSIONS 10

typedef enum {
    DIR_NONE,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_UP_LEFT,
    DIR_UP_RIGHT,
    DIR_DOWN_LEFT,
    DIR_DOWN_RIGHT
} Direction;

typedef enum {
    ENEMY_SMALL,    
    ENEMY_MEDIUM,   
    ENEMY_LARGE,    
    ENEMY_BOSS      
} EnemyType;

typedef enum {
    POWERUP_HEALTH,
    POWERUP_RAPID_FIRE,
    POWERUP_DOUBLE_BULLET
} PowerupType;

typedef struct {
    float x, y;
    float width, height;
    Direction direction;
    int lives;
    bool isRapidFire;
    bool isDoubleBullet;
    float powerupTimer;
    int score;
    float bulletCooldown;
} Player;

typedef struct {
    float x, y;
    float width, height;
    float speed;
    bool active;
} Bullet;

typedef struct {
    float x, y;
    float width, height;
    float speed;
    int health;
    EnemyType type;
    bool active;
    float bulletCooldown;
    float movementPattern;  
    int score;  
} Enemy;

typedef struct {
    float x, y;
    float width, height;
    PowerupType type;
    bool active;
    float speed;
} Powerup;

typedef struct {
    float x, y;
    float width, height;
    float lifespan;
    float currentLife;
    bool active;
} Explosion;

typedef struct {
    int number;
    float scrollSpeed;
    float enemySpawnRate;
    float backgroundOffset;
    float midgroundOffset;   
    float foregroundOffset;  
    bool bossSpawned;
    bool bossDefeated;
} Level;

typedef struct {
    Player player;
    Bullet bullets[MAX_BULLETS];
    Enemy enemies[MAX_ENEMIES];
    Bullet enemyBullets[MAX_ENEMY_BULLETS];
    Powerup powerups[MAX_POWERUPS];
    Explosion explosions[MAX_EXPLOSIONS];
    Level level;
    bool gameOver;
    bool paused;
    float enemySpawnTimer;
    float powerupSpawnTimer;
} GameState;

void initGame(GameState* gameState);
void updateGame(GameState* gameState, float deltaTime);
void fireBullet(GameState* gameState);
void spawnEnemy(GameState* gameState, EnemyType type);
void spawnBoss(GameState* gameState);
void spawnPowerup(GameState* gameState, float x, float y);
void handleCollisions(GameState* gameState);
void nextLevel(GameState* gameState);

#endif 
