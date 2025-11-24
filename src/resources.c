#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <GL/glew.h>

#include "resources.h"
#include "renderer.h"
#include "rng.h"

static GLuint createTextureFromPixelData(unsigned char* pixels, int width, int height, int channels) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
    
    return textureID;
}

static GLuint createPlayerTexture() {
    unsigned char playerPixels[16 * 8 * 4] = {0};
    
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 16; x++) {
            int index = (y * 16 + x) * 4;
            playerPixels[index + 3] = 0;
        }
    }
    
    for (int y = 3; y < 5; y++) {
        for (int x = 0; x < 2; x++) {
            int index = (y * 16 + x) * 4;
            playerPixels[index + 0] = 200; // R
            playerPixels[index + 1] = 100; // G
            playerPixels[index + 2] = 0;   // B
            playerPixels[index + 3] = 255; // A
        }
    }
    
    for (int y = 2; y < 6; y++) {
        for (int x = 2; x < 12; x++) {
            int index = (y * 16 + x) * 4;
            playerPixels[index + 0] = 200; // R
            playerPixels[index + 1] = 200; // G
            playerPixels[index + 2] = 200; // B
            playerPixels[index + 3] = 255; // A
        }
    }
    
    for (int y = 3; y < 5; y++) {
        for (int x = 12; x < 16; x++) {
            int index = (y * 16 + x) * 4;
            playerPixels[index + 0] = 220; // R
            playerPixels[index + 1] = 220; // G
            playerPixels[index + 2] = 220; // B
            playerPixels[index + 3] = 255; // A
        }
    }
    
    return createTextureFromPixelData(playerPixels, 16, 8, 4);
}

static GLuint createEnemySmallTexture() {
    unsigned char pixels[16 * 12 * 4] = {0};
    
    for (int y = 2; y < 10; y++) {
        for (int x = 2; x < 14; x++) {
            int index = (y * 16 + x) * 4;
            pixels[index + 0] = 200; // R
            pixels[index + 1] = 50;  // G
            pixels[index + 2] = 50;  // B
            pixels[index + 3] = 255; // A
        }
    }
    
    return createTextureFromPixelData(pixels, 16, 12, 4);
}

static GLuint createEnemyMediumTexture() {
    unsigned char pixels[24 * 16 * 4] = {0};
    
    for (int y = 2; y < 14; y++) {
        for (int x = 4; x < 20; x++) {
            int index = (y * 24 + x) * 4;
            pixels[index + 0] = 150; // R
            pixels[index + 1] = 150; // G
            pixels[index + 2] = 50;  // B
            pixels[index + 3] = 255; // A
        }
    }
    
    for (int y = 5; y < 11; y++) {
        for (int x = 8; x < 16; x++) {
            int index = (y * 24 + x) * 4;
            pixels[index + 0] = 200; // R
            pixels[index + 1] = 100; // G
            pixels[index + 2] = 0;   // B
            pixels[index + 3] = 255; // A
        }
    }
    
    return createTextureFromPixelData(pixels, 24, 16, 4);
}

static GLuint createEnemyLargeTexture() {
    unsigned char pixels[32 * 24 * 4] = {0};
    
    for (int y = 4; y < 20; y++) {
        for (int x = 4; x < 28; x++) {
            int index = (y * 32 + x) * 4;
            pixels[index + 0] = 50;  // R
            pixels[index + 1] = 50;  // G
            pixels[index + 2] = 150; // B
            pixels[index + 3] = 255; // A
        }
    }
    
    for (int y = 8; y < 16; y++) {
        for (int x = 10; x < 18; x++) {
            int index = (y * 32 + x) * 4;
            pixels[index + 0] = 200; // R
            pixels[index + 1] = 0;   // G
            pixels[index + 2] = 0;   // B
            pixels[index + 3] = 255; // A
        }
    }
    
    return createTextureFromPixelData(pixels, 32, 24, 4);
}

static GLuint createEnemyBossTexture() {
    unsigned char pixels[64 * 48 * 4] = {0};
    
    for (int y = 8; y < 40; y++) {
        for (int x = 8; x < 56; x++) {
            int index = (y * 64 + x) * 4;
            pixels[index + 0] = 100; // R
            pixels[index + 1] = 20;  // G
            pixels[index + 2] = 100; // B
            pixels[index + 3] = 255; // A
        }
    }
    
    for (int y = 12; y < 36; y++) {
        for (int x = 4; x < 8; x++) {
            int index = (y * 64 + x) * 4;
            pixels[index + 0] = 150; // R
            pixels[index + 1] = 50;  // G
            pixels[index + 2] = 150; // B
            pixels[index + 3] = 255; // A
        }
    }
    
    for (int y = 16; y < 32; y++) {
        for (int x = 20; x < 36; x++) {
            int index = (y * 64 + x) * 4;
            pixels[index + 0] = 200; // R
            pixels[index + 1] = 50;  // G
            pixels[index + 2] = 0;   // B
            pixels[index + 3] = 255; // A
        }
    }
    
    return createTextureFromPixelData(pixels, 64, 48, 4);
}

static GLuint createBulletTexture() {
    unsigned char pixels[8 * 4 * 4] = {0};
    
    for (int y = 1; y < 3; y++) {
        for (int x = 0; x < 8; x++) {
            int index = (y * 8 + x) * 4;
            pixels[index + 0] = 200; // R
            pixels[index + 1] = 200; // G
            pixels[index + 2] = 100; // B
            pixels[index + 3] = 255; // A
        }
    }
    
    return createTextureFromPixelData(pixels, 8, 4, 4);
}

static GLuint createEnemyBulletTexture() {
    unsigned char pixels[8 * 4 * 4] = {0};
    
    for (int y = 1; y < 3; y++) {
        for (int x = 0; x < 8; x++) {
            int index = (y * 8 + x) * 4;
            pixels[index + 0] = 200; // R
            pixels[index + 1] = 50;  // G
            pixels[index + 2] = 50;  // B
            pixels[index + 3] = 255; // A
        }
    }
    
    return createTextureFromPixelData(pixels, 8, 4, 4);
}

static GLuint createPowerupHealthTexture() {
    unsigned char pixels[16 * 16 * 4] = {0};
    
    for (int y = 4; y < 12; y++) {
        for (int x = 6; x < 10; x++) {
            int index = (y * 16 + x) * 4;
            pixels[index + 0] = 50;  // R
            pixels[index + 1] = 200; // G
            pixels[index + 2] = 50;  // B
            pixels[index + 3] = 255; // A
        }
    }
    
    for (int y = 6; y < 10; y++) {
        for (int x = 4; x < 12; x++) {
            int index = (y * 16 + x) * 4;
            pixels[index + 0] = 50;  // R
            pixels[index + 1] = 200; // G
            pixels[index + 2] = 50;  // B
            pixels[index + 3] = 255; // A
        }
    }
    
    return createTextureFromPixelData(pixels, 16, 16, 4);
}

static GLuint createPowerupRapidFireTexture() {
    unsigned char pixels[16 * 16 * 4] = {0};
    
    for (int y = 2; y < 14; y++) {
        for (int x = 7; x < 9; x++) {
            int index = (y * 16 + x) * 4;
            pixels[index + 0] = 200; // R
            pixels[index + 1] = 200; // G
            pixels[index + 2] = 0;   // B
            pixels[index + 3] = 255; // A
        }
    }
    
    for (int y = 8; y < 10; y++) {
        for (int x = 4; x < 12; x++) {
            int index = (y * 16 + x) * 4;
            pixels[index + 0] = 200; // R
            pixels[index + 1] = 200; // G
            pixels[index + 2] = 0;   // B
            pixels[index + 3] = 255; // A
        }
    }
    
    return createTextureFromPixelData(pixels, 16, 16, 4);
}

static GLuint createPowerupDoubleBulletTexture() {
    unsigned char pixels[16 * 16 * 4] = {0};
    
    for (int y = 4; y < 8; y++) {
        for (int x = 4; x < 8; x++) {
            int index = (y * 16 + x) * 4;
            pixels[index + 0] = 50;  // R
            pixels[index + 1] = 100; // G
            pixels[index + 2] = 200; // B
            pixels[index + 3] = 255; // A
        }
    }
    
    for (int y = 8; y < 12; y++) {
        for (int x = 8; x < 12; x++) {
            int index = (y * 16 + x) * 4;
            pixels[index + 0] = 50;  // R
            pixels[index + 1] = 100; // G
            pixels[index + 2] = 200; // B
            pixels[index + 3] = 255; // A
        }
    }
    
    return createTextureFromPixelData(pixels, 16, 16, 4);
}

static GLuint createExplosionTexture() {
    unsigned char pixels[32 * 32 * 4] = {0};
    
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            int dx = x - 16;
            int dy = y - 16;
            float dist = sqrt(dx*dx + dy*dy);
            
            if (dist < 14) {
                int index = (y * 32 + x) * 4;
                pixels[index + 0] = 255; // R
                pixels[index + 1] = (unsigned char)(200 * (1.0f - dist / 14.0f)); // G
                pixels[index + 2] = 0;   // B
                pixels[index + 3] = 255; // A
            }
        }
    }
    
    return createTextureFromPixelData(pixels, 32, 32, 4);
}

    static GLuint createBackgroundTexture() {
        unsigned char pixels[256 * 256 * 4] = {0};
        
        for (int i = 0; i < 200; i++) {
            int x = (int)(rng_u32() % 256u);
            int y = (int)(rng_u32() % 256u);
            int brightness = (int)(rng_u32() % 155u) + 100;
            
            int index = (y * 256 + x) * 4;
            pixels[index + 0] = brightness;
            pixels[index + 1] = brightness;
            pixels[index + 2] = brightness;
            pixels[index + 3] = 255;
        }
        
        return createTextureFromPixelData(pixels, 256, 256, 4);
    }

static GLuint createHudLifeTexture() {
    unsigned char pixels[16 * 16 * 4] = {0};
    
    for (int y = 4; y < 12; y++) {
        for (int x = 4; x < 12; x++) {
            int index = (y * 16 + x) * 4;
            pixels[index + 0] = 200; // R
            pixels[index + 1] = 50;  // G
            pixels[index + 2] = 50;  // B
            pixels[index + 3] = 255; // A
        }
    }
    
    return createTextureFromPixelData(pixels, 16, 16, 4);
}

extern void setTexture(SpriteType type, GLuint textureID);

bool loadResources() {
    
    GLuint playerTexture = createPlayerTexture();
    setTexture(SPRITE_PLAYER, playerTexture);
    
    GLuint enemySmallTexture = createEnemySmallTexture();
    setTexture(SPRITE_ENEMY_SMALL, enemySmallTexture);
    
    GLuint enemyMediumTexture = createEnemyMediumTexture();
    setTexture(SPRITE_ENEMY_MEDIUM, enemyMediumTexture);
    
    GLuint enemyLargeTexture = createEnemyLargeTexture();
    setTexture(SPRITE_ENEMY_LARGE, enemyLargeTexture);
    
    GLuint enemyBossTexture = createEnemyBossTexture();
    setTexture(SPRITE_ENEMY_BOSS, enemyBossTexture);
    
    GLuint bulletTexture = createBulletTexture();
    setTexture(SPRITE_BULLET, bulletTexture);
    
    GLuint enemyBulletTexture = createEnemyBulletTexture();
    setTexture(SPRITE_ENEMY_BULLET, enemyBulletTexture);
    
    GLuint powerupHealthTexture = createPowerupHealthTexture();
    setTexture(SPRITE_POWERUP_HEALTH, powerupHealthTexture);
    
    GLuint powerupRapidFireTexture = createPowerupRapidFireTexture();
    setTexture(SPRITE_POWERUP_RAPID_FIRE, powerupRapidFireTexture);
    
    GLuint powerupDoubleBulletTexture = createPowerupDoubleBulletTexture();
    setTexture(SPRITE_POWERUP_DOUBLE_BULLET, powerupDoubleBulletTexture);
    
    GLuint explosionTexture = createExplosionTexture();
    setTexture(SPRITE_EXPLOSION, explosionTexture);
    
    GLuint backgroundTexture = createBackgroundTexture();
    setTexture(SPRITE_BACKGROUND, backgroundTexture);
    
    GLuint hudLifeTexture = createHudLifeTexture();
    setTexture(SPRITE_HUD_LIFE, hudLifeTexture);
    
    return true;
}

void unloadResources() {
}
