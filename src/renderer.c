#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "renderer.h"
#include "resources.h"

static const char* vertexShaderSource = 
"#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in vec2 aTexCoord;\n"
"out vec2 TexCoord;\n"
"uniform mat4 model;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);\n"
"    TexCoord = aTexCoord;\n"
"}\n";

static const char* fragmentShaderSource = 
"#version 330 core\n"
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"uniform sampler2D texture1;\n"
"uniform vec4 color;\n"
"void main()\n"
"{\n"
"    vec4 texColor = texture(texture1, TexCoord);\n"
"    if(texColor.a < 0.1)\n"
"        discard;\n"
"    FragColor = texColor * color;\n"
"}\n";

static const char* bulletVertexShaderSource = 
"#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in vec2 aTexCoord;\n"
"layout (location = 2) in vec2 iPos;\n"
"layout (location = 3) in vec2 iSize;\n"
"layout (location = 4) in vec4 iColor;\n"
"out vec2 TexCoord;\n"
"out vec4 Color;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"    vec2 worldPos = iPos + aPos * iSize;\n"
"    gl_Position = projection * vec4(worldPos, 0.0, 1.0);\n"
"    TexCoord = aTexCoord;\n"
"    Color = iColor;\n"
"}\n";

static const char* bulletFragmentShaderSource = 
"#version 330 core\n"
"in vec2 TexCoord;\n"
"in vec4 Color;\n"
"out vec4 FragColor;\n"
"uniform sampler2D texture1;\n"
"void main()\n"
"{\n"
"    vec4 texColor = texture(texture1, TexCoord);\n"
"    if(texColor.a < 0.1)\n"
"        discard;\n"
"    FragColor = texColor * Color;\n"
"}\n";

typedef struct {
    GLuint shaderProgram;
    GLuint bulletShaderProgram;
    GLuint VAO, VBO, EBO;
    GLuint bulletInstanceVBO;
    GLuint textures[SPRITE_COUNT];
    GLint modelLoc;
    GLint projectionLoc;
    GLint colorLoc;
    GLint bulletProjectionLoc;
} Renderer;

typedef struct {
    float x, y;
    float w, h;
    float r, g, b, a;
} SpriteInstance;

static Renderer renderer;

static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
        return 0;
    }
    
    return shader;
}

static GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    
    if (vertexShader == 0 || fragmentShader == 0) {
        return 0;
    }
    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
        return 0;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

bool initRenderer() {
    renderer.shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    if (renderer.shaderProgram == 0) {
        return false;
    }

    renderer.bulletShaderProgram = createShaderProgram(bulletVertexShaderSource, bulletFragmentShaderSource);
    if (renderer.bulletShaderProgram == 0) {
        return false;
    }
    
    renderer.modelLoc = glGetUniformLocation(renderer.shaderProgram, "model");
    renderer.projectionLoc = glGetUniformLocation(renderer.shaderProgram, "projection");
    renderer.colorLoc = glGetUniformLocation(renderer.shaderProgram, "color");
    renderer.bulletProjectionLoc = glGetUniformLocation(renderer.bulletShaderProgram, "projection");
    
    float vertices[] = {
         0.5f,  0.5f,         1.0f, 0.0f,   
         0.5f, -0.5f,         1.0f, 1.0f,   
        -0.5f, -0.5f,         0.0f, 1.0f,   
        -0.5f,  0.5f,         0.0f, 0.0f    
    };
    
    unsigned int indices[] = {
        0, 1, 3,  
        1, 2, 3   
    };
    
    glGenVertexArrays(1, &renderer.VAO);
    glGenBuffers(1, &renderer.VBO);
    glGenBuffers(1, &renderer.EBO);
    glGenBuffers(1, &renderer.bulletInstanceVBO);
    glBindVertexArray(renderer.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, renderer.VBO);    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, renderer.bulletInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * MAX_BULLETS, NULL, GL_STREAM_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(0));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 4));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
    
    float projectionMatrix[16] = {
        2.0f/480.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f/320.0f, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    
    glUseProgram(renderer.shaderProgram);
    glUniformMatrix4fv(renderer.projectionLoc, 1, GL_FALSE, projectionMatrix);

    glUseProgram(renderer.bulletShaderProgram);
    glUniformMatrix4fv(renderer.bulletProjectionLoc, 1, GL_FALSE, projectionMatrix);

    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glUniform4fv(renderer.colorLoc, 1, color);
    
    return true;
}

void destroyRenderer() {
    glDeleteVertexArrays(1, &renderer.VAO);
    glDeleteBuffers(1, &renderer.VBO);
    glDeleteBuffers(1, &renderer.EBO);
    glDeleteBuffers(1, &renderer.bulletInstanceVBO);
    glDeleteProgram(renderer.shaderProgram);
    glDeleteProgram(renderer.bulletShaderProgram);
    glDeleteTextures(SPRITE_COUNT, renderer.textures);
}

void setTexture(SpriteType type, GLuint textureID) {
    renderer.textures[type] = textureID;
}

static void drawQuad(float x, float y, float width, float height, float r, float g, float b, float a) {
    float modelMatrix[16] = {
        width, 0.0f, 0.0f, 0.0f,
        0.0f, height, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x, y, 0.0f, 1.0f
    };
    
    glUniformMatrix4fv(renderer.modelLoc, 1, GL_FALSE, modelMatrix);
    
    float color[4] = {r, g, b, a};
    glUniform4fv(renderer.colorLoc, 1, color);
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void renderGame(GameState* gameState) {
    glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(renderer.shaderProgram);
    glBindVertexArray(renderer.VAO);
    
    float farScrollPos = fmodf(gameState->level.backgroundOffset, 256.0f);
    
    glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_BACKGROUND]);
    drawQuad(256.0f - farScrollPos, 160, 256, 320, 0.4f, 0.4f, 0.5f, 0.3f);
    drawQuad(512.0f - farScrollPos, 160, 256, 320, 0.4f, 0.4f, 0.5f, 0.3f);
    drawQuad(768.0f - farScrollPos, 160, 256, 320, 0.4f, 0.4f, 0.5f, 0.3f);
    drawQuad(0.0f   - farScrollPos, 160, 256, 320, 0.4f, 0.4f, 0.5f, 0.3f);

    float midScrollPos = fmodf(gameState->level.midgroundOffset, 256.0f);
    
    drawQuad(256.0f - midScrollPos, 160, 256, 320, 0.5f, 0.5f, 0.6f, 0.5f);
    drawQuad(512.0f - midScrollPos, 160, 256, 320, 0.5f, 0.5f, 0.6f, 0.5f);
    drawQuad(768.0f - midScrollPos, 160, 256, 320, 0.5f, 0.5f, 0.6f, 0.5f);
    drawQuad(0.0f   - midScrollPos, 160, 256, 320, 0.5f, 0.5f, 0.6f, 0.5f);
    
    float nearScrollPos = fmodf(gameState->level.foregroundOffset, 256.0f);
    
    drawQuad(256.0f - nearScrollPos, 160, 256, 320, 0.7f, 0.7f, 0.8f, 0.7f);
    drawQuad(512.0f - nearScrollPos, 160, 256, 320, 0.7f, 0.7f, 0.8f, 0.7f);
    drawQuad(768.0f - nearScrollPos, 160, 256, 320, 0.7f, 0.7f, 0.8f, 0.7f);
    drawQuad(0.0f   - nearScrollPos, 160, 256, 320, 0.7f, 0.7f, 0.8f, 0.7f);
    
    glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_PLAYER]);
    drawQuad(gameState->player.x, gameState->player.y, 
             gameState->player.width, gameState->player.height, 1.0f, 1.0f, 1.0f, 1.0f);
    
    static SpriteInstance bulletInstances[MAX_BULLETS];
    int bulletCount = 0;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (gameState->bullets[i].active) {
            SpriteInstance s;
            s.x = gameState->bullets[i].x;
            s.y = gameState->bullets[i].y;
            s.w = gameState->bullets[i].width;
            s.h = gameState->bullets[i].height;
            s.r = 1.0f; s.g = 1.0f; s.b = 0.5f; s.a = 1.0f;
            bulletInstances[bulletCount++] = s;
        }
    }
    if (bulletCount > 0) {
        glUseProgram(renderer.bulletShaderProgram);
        glBindVertexArray(renderer.VAO);
        glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_BULLET]);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.bulletInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(bulletCount * (int)sizeof(SpriteInstance)), bulletInstances);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, bulletCount);
        glUseProgram(renderer.shaderProgram);
        glBindVertexArray(renderer.VAO);
    }
    
    static SpriteInstance enemyBulletInstances[MAX_ENEMY_BULLETS];
    int enemyBulletCount = 0;
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (gameState->enemyBullets[i].active) {
            SpriteInstance s;
            s.x = gameState->enemyBullets[i].x;
            s.y = gameState->enemyBullets[i].y;
            s.w = gameState->enemyBullets[i].width;
            s.h = gameState->enemyBullets[i].height;
            s.r = 1.0f; s.g = 0.0f; s.b = 0.0f; s.a = 1.0f;
            enemyBulletInstances[enemyBulletCount++] = s;
        }
    }
    if (enemyBulletCount > 0) {
        glUseProgram(renderer.bulletShaderProgram);
        glBindVertexArray(renderer.VAO);
        glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_ENEMY_BULLET]);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.bulletInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(enemyBulletCount * (int)sizeof(SpriteInstance)), enemyBulletInstances);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, enemyBulletCount);
        glUseProgram(renderer.shaderProgram);
        glBindVertexArray(renderer.VAO);
    }
    
    static SpriteInstance enemiesSmall[MAX_ENEMIES];
    static SpriteInstance enemiesMedium[MAX_ENEMIES];
    static SpriteInstance enemiesLarge[MAX_ENEMIES];
    static SpriteInstance enemiesBoss[MAX_ENEMIES];
    int countSmall = 0, countMedium = 0, countLarge = 0, countBoss = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!gameState->enemies[i].active) continue;
        SpriteInstance s;
        s.x = gameState->enemies[i].x;
        s.y = gameState->enemies[i].y;
        s.w = gameState->enemies[i].width;
        s.h = gameState->enemies[i].height;
        s.r = 1.0f; s.g = 1.0f; s.b = 1.0f; s.a = 1.0f;
        switch (gameState->enemies[i].type) {
            case ENEMY_SMALL:
                enemiesSmall[countSmall++] = s;
                break;
            case ENEMY_MEDIUM:
                enemiesMedium[countMedium++] = s;
                break;
            case ENEMY_LARGE:
                enemiesLarge[countLarge++] = s;
                break;
            case ENEMY_BOSS:
                enemiesBoss[countBoss++] = s;
                break;
            default:
                enemiesSmall[countSmall++] = s;
                break;
        }
    }
    if (countSmall > 0) {
        glUseProgram(renderer.bulletShaderProgram);
        glBindVertexArray(renderer.VAO);
        glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_ENEMY_SMALL]);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.bulletInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(countSmall * (int)sizeof(SpriteInstance)), enemiesSmall);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, countSmall);
        glUseProgram(renderer.shaderProgram);
        glBindVertexArray(renderer.VAO);
    }
    if (countMedium > 0) {
        glUseProgram(renderer.bulletShaderProgram);
        glBindVertexArray(renderer.VAO);
        glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_ENEMY_MEDIUM]);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.bulletInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(countMedium * (int)sizeof(SpriteInstance)), enemiesMedium);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, countMedium);
        glUseProgram(renderer.shaderProgram);
        glBindVertexArray(renderer.VAO);
    }
    if (countLarge > 0) {
        glUseProgram(renderer.bulletShaderProgram);
        glBindVertexArray(renderer.VAO);
        glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_ENEMY_LARGE]);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.bulletInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(countLarge * (int)sizeof(SpriteInstance)), enemiesLarge);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, countLarge);
        glUseProgram(renderer.shaderProgram);
        glBindVertexArray(renderer.VAO);
    }
    if (countBoss > 0) {
        glUseProgram(renderer.bulletShaderProgram);
        glBindVertexArray(renderer.VAO);
        glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_ENEMY_BOSS]);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.bulletInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(countBoss * (int)sizeof(SpriteInstance)), enemiesBoss);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, countBoss);
        glUseProgram(renderer.shaderProgram);
        glBindVertexArray(renderer.VAO);
    }
    
    static SpriteInstance powerupsHealth[MAX_POWERUPS];
    static SpriteInstance powerupsRapid[MAX_POWERUPS];
    static SpriteInstance powerupsDouble[MAX_POWERUPS];
    int countHealth = 0, countRapid = 0, countDouble = 0;
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!gameState->powerups[i].active) continue;
        SpriteInstance s;
        s.x = gameState->powerups[i].x;
        s.y = gameState->powerups[i].y;
        s.w = gameState->powerups[i].width;
        s.h = gameState->powerups[i].height;
        switch (gameState->powerups[i].type) {
            case POWERUP_HEALTH:
                s.r = 0.0f; s.g = 1.0f; s.b = 0.0f; s.a = 1.0f;
                powerupsHealth[countHealth++] = s;
                break;
            case POWERUP_RAPID_FIRE:
                s.r = 1.0f; s.g = 1.0f; s.b = 0.0f; s.a = 1.0f;
                powerupsRapid[countRapid++] = s;
                break;
            case POWERUP_DOUBLE_BULLET:
                s.r = 0.0f; s.g = 0.5f; s.b = 1.0f; s.a = 1.0f;
                powerupsDouble[countDouble++] = s;
                break;
            default:
                s.r = 0.0f; s.g = 1.0f; s.b = 0.0f; s.a = 1.0f;
                powerupsHealth[countHealth++] = s;
                break;
        }
    }
    if (countHealth > 0) {
        glUseProgram(renderer.bulletShaderProgram);
        glBindVertexArray(renderer.VAO);
        glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_POWERUP_HEALTH]);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.bulletInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(countHealth * (int)sizeof(SpriteInstance)), powerupsHealth);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, countHealth);
        glUseProgram(renderer.shaderProgram);
        glBindVertexArray(renderer.VAO);
    }
    if (countRapid > 0) {
        glUseProgram(renderer.bulletShaderProgram);
        glBindVertexArray(renderer.VAO);
        glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_POWERUP_RAPID_FIRE]);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.bulletInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(countRapid * (int)sizeof(SpriteInstance)), powerupsRapid);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, countRapid);
        glUseProgram(renderer.shaderProgram);
        glBindVertexArray(renderer.VAO);
    }
    if (countDouble > 0) {
        glUseProgram(renderer.bulletShaderProgram);
        glBindVertexArray(renderer.VAO);
        glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_POWERUP_DOUBLE_BULLET]);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.bulletInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(countDouble * (int)sizeof(SpriteInstance)), powerupsDouble);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, countDouble);
        glUseProgram(renderer.shaderProgram);
        glBindVertexArray(renderer.VAO);
    }

    static SpriteInstance explosionInstances[MAX_EXPLOSIONS];
    int explosionCount = 0;
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!gameState->explosions[i].active) continue;
        SpriteInstance s;
        s.x = gameState->explosions[i].x;
        s.y = gameState->explosions[i].y;
        s.w = gameState->explosions[i].width;
        s.h = gameState->explosions[i].height;
        float alpha = gameState->explosions[i].currentLife / gameState->explosions[i].lifespan;
        s.r = 1.0f; s.g = 0.7f; s.b = 0.0f; s.a = alpha;
        explosionInstances[explosionCount++] = s;
    }
    if (explosionCount > 0) {
        glUseProgram(renderer.bulletShaderProgram);
        glBindVertexArray(renderer.VAO);
        glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_EXPLOSION]);
        glBindBuffer(GL_ARRAY_BUFFER, renderer.bulletInstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(explosionCount * (int)sizeof(SpriteInstance)), explosionInstances);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, explosionCount);
        glUseProgram(renderer.shaderProgram);
        glBindVertexArray(renderer.VAO);
    }

    if (!gameState->benchmarkMode) {
        glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_HUD_LIFE]);
        for (int i = 0; i < gameState->player.lives; i++) {
            drawQuad(20 + (i * 20), 20, 16, 16, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        if (gameState->player.isRapidFire) {
            glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_POWERUP_RAPID_FIRE]);
            drawQuad(430, 20, 16, 16, 1.0f, 1.0f, 0.0f, 1.0f);
        }

        if (gameState->player.isDoubleBullet) {
            glBindTexture(GL_TEXTURE_2D, renderer.textures[SPRITE_POWERUP_DOUBLE_BULLET]);
            drawQuad(450, 20, 16, 16, 0.0f, 0.5f, 1.0f, 1.0f);
        }
    }

    glBindVertexArray(0);
}

void renderGameOver(GameState* gameState) {
    glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    char scoreText[32];
    sprintf(scoreText, "FINAL SCORE: %d", gameState->player.score);
    
}
