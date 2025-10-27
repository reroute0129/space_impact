#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "game.h"
#include "renderer.h"
#include "resources.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1200

GameState gameState;
GLFWwindow* window;

static bool keyUpPressed = false;
static bool keyDownPressed = false;
static bool keyLeftPressed = false;
static bool keyRightPressed = false;

void updatePlayerDirection(GameState* gameState) {
    if (keyUpPressed && keyLeftPressed) {
        gameState->player.direction = DIR_UP_LEFT;
    } else if (keyUpPressed && keyRightPressed) {
        gameState->player.direction = DIR_UP_RIGHT;
    } else if (keyDownPressed && keyLeftPressed) {
        gameState->player.direction = DIR_DOWN_LEFT;
    } else if (keyDownPressed && keyRightPressed) {
        gameState->player.direction = DIR_DOWN_RIGHT;
    } else if (keyUpPressed) {
        gameState->player.direction = DIR_UP;
    } else if (keyDownPressed) {
        gameState->player.direction = DIR_DOWN;
    } else if (keyLeftPressed) {
        gameState->player.direction = DIR_LEFT;
    } else if (keyRightPressed) {
        gameState->player.direction = DIR_RIGHT;
    } else {
        gameState->player.direction = DIR_NONE;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_UP:
                keyUpPressed = true;
                break;
            case GLFW_KEY_DOWN:
                keyDownPressed = true;
                break;
            case GLFW_KEY_RIGHT:
                keyRightPressed = true;
                break;
            case GLFW_KEY_LEFT:
                keyLeftPressed = true;
                break;
            case GLFW_KEY_SPACE:
                fireBullet(&gameState);
                break;
            case GLFW_KEY_ESCAPE:
                gameState.gameOver = true;
                break;
            case GLFW_KEY_ENTER:
                if (gameState.gameOver) {
                    initGame(&gameState);
                }
                break;
        }
    } else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_UP:
                keyUpPressed = false;
                break;
            case GLFW_KEY_DOWN:
                keyDownPressed = false;
                break;
            case GLFW_KEY_RIGHT:
                keyRightPressed = false;
                break;
            case GLFW_KEY_LEFT:
                keyLeftPressed = false;
                break;
        }
    }
    
    updatePlayerDirection(&gameState);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

bool initOpenGL() {
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Space Impact", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        printf("Failed to initialize GLEW\n");
        return false;
    }

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

#define MAX_BENCH_FRAMES 300000

static void print_usage(const char* prog) {
    printf("Usage: %s [--benchmark] [--duration SEC] [--warmup SEC] [--density 0-100]\n", prog);
}

static int cmp_desc_double(const void* a, const void* b) {
    double da = *(const double*)a, db = *(const double*)b;
    if (da < db) return 1;
    if (da > db) return -1;
    return 0;
}

int main(int argc, char** argv) {
    bool optBenchmark = false;
    double optDuration = 10.0;
    double optWarmup = 1.0;
    int optDensity = 100;

    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "--benchmark") == 0) {
            optBenchmark = true;
        } else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
            optDuration = atof(argv[++i]);
        } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            optWarmup = atof(argv[++i]);
        } else if (strcmp(argv[i], "--density") == 0 && i + 1 < argc) {
            optDensity = atoi(argv[++i]);
        } else {
            printf("Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    srand((unsigned)time(NULL));

    if (!initOpenGL()) {
        return -1;
    }

    glfwSwapInterval(0);

    if (!initRenderer()) {
        glfwTerminate();
        return -1;
    }

    if (!loadResources()) {
        destroyRenderer();
        glfwTerminate();
        return -1;
    }

    initGame(&gameState);

    if (!optBenchmark) {
        double lastTime = glfwGetTime();
        double deltaTime = 0.0;
        double frameTime = 1.0 / 60.0;

        while (!glfwWindowShouldClose(window) && !gameState.gameOver) {
            double currentTime = glfwGetTime();
            deltaTime += currentTime - lastTime;
            lastTime = currentTime;

            glfwPollEvents();

            while (deltaTime >= frameTime) {
                updateGame(&gameState, frameTime);
                deltaTime -= frameTime;
            }

            renderGame(&gameState);
            glfwSwapBuffers(window);
        }

        if (gameState.gameOver) {
            renderGameOver(&gameState);
            glfwSwapBuffers(window);

            while (!glfwWindowShouldClose(window) && gameState.gameOver) {
                glfwPollEvents();
            }
        }

        destroyRenderer();
        glfwTerminate();
        return 0;
    }

    if (optDensity < 0) optDensity = 0;
    if (optDensity > 100) optDensity = 100;
    prepareBenchmarkScene(&gameState, optDensity);

    static double frameDurations[MAX_BENCH_FRAMES];
    int framesCollected = 0;
    double sumDur = 0.0, minDur = 1e9, maxDur = 0.0;

    const double fixedDt = 1.0 / 60.0;
    double lastTime = glfwGetTime();
    double accumulator = 0.0;

    const double benchStart = lastTime;
    const double warmupEnd = benchStart + ((optWarmup > 0.0) ? optWarmup : 0.0);
    const double benchEnd = warmupEnd + ((optDuration > 0.0) ? optDuration : 0.0);
    double lastSwapTs = lastTime;

    printf("[Benchmark] density=%d, warmup=%.2fs, duration=%.2fs\n",
           optDensity, optWarmup, optDuration);
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        accumulator += now - lastTime;
        lastTime = now;

        glfwPollEvents();

        while (accumulator >= fixedDt) {
            updateGame(&gameState, (float)fixedDt);
            accumulator -= fixedDt;
        }

        renderGame(&gameState);
        glfwSwapBuffers(window);

        double afterSwap = glfwGetTime();
        double frameDur = afterSwap - lastSwapTs;
        lastSwapTs = afterSwap;

        if (afterSwap >= warmupEnd && afterSwap <= benchEnd) {
            if (framesCollected < MAX_BENCH_FRAMES) {
                frameDurations[framesCollected++] = frameDur;
            }
            sumDur += frameDur;
            if (frameDur < minDur) minDur = frameDur;
            if (frameDur > maxDur) maxDur = frameDur;
        }

        if (afterSwap >= benchEnd) break;
    }

    double elapsed = sumDur;
    double avgFps = (elapsed > 0.0) ? ((double)framesCollected / elapsed) : 0.0;
    double minFps = (maxDur > 0.0) ? (1.0 / maxDur) : 0.0;
    double maxFps = (minDur > 0.0) ? (1.0 / minDur) : 0.0;

    double p1LowFps = 0.0;
    if (framesCollected > 0) {
        qsort(frameDurations, framesCollected, sizeof(double), cmp_desc_double);
        int n = (int)ceil(framesCollected * 0.01);
        if (n < 1) n = 1;
        double sumSlow = 0.0;
        for (int i = 0; i < n && i < framesCollected; i++) sumSlow += frameDurations[i];
        double avgSlowDur = sumSlow / (double)n;
        if (avgSlowDur > 0.0) p1LowFps = 1.0 / avgSlowDur;
    }

    printf("\nBenchmark results\n");
    printf("Frames: %d\n", framesCollected);
    printf("Measured time: %.3f s\n", elapsed);
    printf("Avg FPS: %.2f\n", avgFps);
    printf("1%% low FPS: %.2f\n", p1LowFps);
    printf("Min FPS: %.2f\n", minFps);
    printf("Max FPS: %.2f\n", maxFps);

    destroyRenderer();
    glfwTerminate();
    return 0;
}
