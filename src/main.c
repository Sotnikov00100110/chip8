#include <SDL2/SDL_events.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_quit.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <string.h>

#define MEMORY_POOL 4096
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT= 320;

char font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

typedef struct { 
    uint16_t stack[16];
    uint16_t sp;
    uint8_t display[64 * 32];
    uint8_t memory_stack[MEMORY_POOL];
    uint16_t PC;
    uint16_t I;
    uint8_t V[16];
    uint8_t DT;
    uint8_t ST;
    unsigned char key[16];
    int flag;
} Chip8;

Chip8 *initializeCH8();
void draw_chip8(SDL_Renderer *renderer, Chip8 *ch8);
void cycle(Chip8 *ch8);
void freeCh8(Chip8 *ch8); 
void loadROM(Chip8 *ch8, char *filename);
void draw_chip8(SDL_Renderer *render, Chip8 *ch8);
void ch8start(char *name_game);

Chip8 *initializeCH8() {
    Chip8 *ch8 = (Chip8 *)malloc(sizeof(Chip8));
    
    if (ch8 == NULL) return NULL;
    memset(ch8, 0, sizeof(Chip8));
    ch8->PC = 0x200;
    memcpy(&ch8->memory_stack[0], font, 80);
    
    return ch8;
}

void cycle(Chip8 *ch8) {
    uint16_t opcode = ch8->memory_stack[ch8->PC] << 8 | ch8->memory_stack[ch8->PC + 1];
    ch8->PC += 2;

    switch (opcode & 0xF000) {
        case 0x0000: { // stack operation 
            if (opcode == 0x00E0) {
                memset(ch8->display, 0, sizeof(ch8->display));
            } else if (opcode == 0x00EE) {
                ch8->sp--;
                ch8->PC = ch8->stack[ch8->sp];
            }       
            break;
        }
        case 0xE000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            if ((opcode & 0x00FF) == 0x9e) {
                if (ch8->key[ch8->V[x]]) ch8->PC += 2;
            } else if ((opcode & 0x00FF) == 0xA1) {
                if (!ch8->key[ch8->V[x]]) ch8->PC += 2;
            }
            break;
        }
        case 0x1000: { // jump to adress nnn 
            uint16_t nnn = (opcode & 0x0FFF);
            ch8->PC = nnn;
            break;
        }
        case 0x2000: { // increment to stack 
            ch8->stack[ch8->sp] = ch8->PC;
            ch8->sp++;
            uint16_t nnn = (opcode & 0x0FFF);
            ch8->PC = nnn;
            break;
        }
        case 0x3000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = (opcode & 0x00FF);
            if (ch8->V[x] == nn) ch8->PC += 2;
            break;
        }
        case 0x4000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = (opcode & 0x00FF);
            if (ch8->V[x] != nn) ch8->PC += 2;
            break;
        }
        case 0x5000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            if (ch8->V[x] == ch8->V[y]) ch8->PC += 2;
            break;
        }
        case 0x6000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = (opcode & 0x00FF);
            ch8->V[x] = nn;
            break;
        }
        case 0x7000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = (opcode & 0x00FF);
            ch8->V[x] += nn;
            break;
        }
        case 0x8000: { // math operation, bit and decimal operation 
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            switch (opcode & 0x000F) {
                case 0x0000: {
                    ch8->V[x] = ch8->V[y];
                    break;                
                }
                case 0x0001: {
                    ch8->V[x] |= ch8->V[y];
                    break;
                }
                case 0x0002: {
                    ch8->V[x] &= ch8->V[y];
                    break;
                }
                case 0x0003: {
                    ch8->V[x] ^= ch8->V[y];
                    break;
                }
                case 0x0004: {
                    uint16_t sum = ch8->V[x] + ch8->V[y];
                    ch8->V[0xF] = (sum > 255) ? 1 : 0;
                    ch8->V[x] = sum & 0xFF;
                    break;
                }
                case 0x0005: {
                    ch8->V[0xF] = (ch8->V[x] > ch8->V[y]) ? 1 : 0;
                    ch8->V[x] -= ch8->V[y];
                    break;
                }
                 case 0x0006: {
                    ch8->V[0xF] = ch8->V[x] & 0x1;
                    ch8->V[x] >>= 1;
                    break;
                }
                case 0x0007: {
                    ch8->V[0xF] = (ch8->V[y] > ch8->V[x]) ? 1 : 0;
                    ch8->V[x] = ch8->V[y] - ch8->V[x];
                    break;
                }
                case 0x000E: {
                    ch8->V[0xF] = (ch8->V[x] >> 7);
                    ch8->V[x] <<= 1;
                    break;
                }
            }
            break;
        }
        case 0x9000: { 
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            if (ch8->V[x] != ch8->V[y]) ch8->PC += 2;
            break;
        }
        case 0xA000: { // I = nnn adress 
            uint16_t nnn = (opcode & 0x0FFF);
            ch8->I = nnn;
            break;
        }
        case 0xB000: { // jump to nnn + V[0]
            uint16_t nnn = (opcode & 0x0FFF);
            ch8->PC = ch8->V[0] + nnn;
            break;
        }
        case 0xC000: { // random operation from 0 to 255 
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = (opcode & 0x00FF);
            ch8->V[x] = ((rand() & 255) + 0) & nn;
            break;    
        }
        case 0xD000: { // display operation 
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            uint8_t n = (opcode & 0x000F);
            
            uint8_t start_x = ch8->V[x] % 64;
            uint8_t start_y = ch8->V[y] % 32;
            
            ch8->V[0xF] = 0;

            for (int i = 0; i < n; i++) {
                
                uint8_t pixel = ch8->memory_stack[ch8->I + i];
                
                for (int j = 0; j < 8; j++) {
                    if ((pixel & (0x80 >> j)) != 0) {
                        int curr_y = (start_y + i) % 32;
                        int curr_x = (start_x + j) % 64;
                        int pixel_index =  curr_y * 64 + curr_x;

                        if (ch8->display[pixel_index] != 0) {
                            ch8->V[0xF] = 1;
                        }
                        
                        ch8->display[pixel_index] ^= 0xFF;
                    }
                }
            }

            ch8->flag = 1;
            break;    
        }
        case 0xF000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            switch (opcode & 0x00FF) {
                case 0x001E: {
                    uint8_t x = (opcode & 0x0F00) >> 8;
                    ch8->I += ch8->V[x];
                    break;
                }
                case 0x07: {
                    ch8->V[x] = ch8->DT;
                    break;
                }
                case 0x15: {
                    ch8->DT = ch8->V[x];
                    break;
                }
                case 0x18: {
                    ch8->ST = ch8->V[x];
                    break;
                }
                case 0x29: {
                    ch8->I = ch8->V[x] * 5;
                    break;
                }
                case 0x33: {
                    uint8_t val = ch8->V[x];
                    ch8->memory_stack[ch8->I] = val / 100;
                    ch8->memory_stack[ch8->I + 1] = (val / 10) % 10;
                    ch8->memory_stack[ch8->I + 2] = val % 10;
                    break;
                }
                case 0x55: {
                    for (int i = 0; i <= x; i++) {
                        ch8->memory_stack[ch8->I + i] = ch8->V[i];
                    }
                    break;
                }
                case 0x65: {
                    for (int i = 0; i <= x; i++) {
                        ch8->V[i] = ch8->memory_stack[ch8->I + i];
                    }
                    break;
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}

void freeCh8(Chip8 *ch8) {
    if (ch8 != NULL) {
        free(ch8);
    }
}

void loadROM(Chip8 *ch8, char *filename) {
    FILE *f = fopen(filename, "rb");
    
    if (!f) {
        fprintf(stderr, "Large file or cannot open\n");
        return;
    }
    fseek(f, 0, SEEK_END);

    long rom_size = ftell(f);
    rewind(f);

    if (rom_size > (MEMORY_POOL - 0x200)) {
        fprintf(stderr, "large rom\n");
        fclose(f);
    }

    fread(&ch8->memory_stack[0x200], rom_size, 1, f);
    fclose(f);
}

void draw_chip8(SDL_Renderer *render, Chip8 *ch8) {
    SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
    SDL_RenderClear(render);
    SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
    
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            if (ch8->display[y * 64 + x]) {
                SDL_Rect rect = { x * 10, y * 10, 10, 10 }; // Масштаб 10x
                SDL_RenderFillRect(render, &rect);
            }
        }
    }

    SDL_RenderPresent(render);
}

void ch8start(char *name_game) {
    Chip8 *ch8 = initializeCH8();
    loadROM(ch8, name_game);

    SDL_Window *window = SDL_CreateWindow(
        "Chip8_Test",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 320,
        0 
    );

    SDL_Renderer *render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Event event;
    int running = 1;
    uint32_t last_timer = SDL_GetTicks();
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                int state = (event.type == SDL_KEYDOWN) ? 1 : 0;
                switch (event.key.keysym.sym) {
                    case SDLK_1: ch8->key[0x1] = state; break;
                    case SDLK_2: ch8->key[0x2] = state; break;
                    case SDLK_3: ch8->key[0x3] = state; break;
                    case SDLK_4: ch8->key[0xC] = state; break;
                    case SDLK_q: ch8->key[0x4] = state; break;
                    case SDLK_w: ch8->key[0x5] = state; break;
                    case SDLK_e: ch8->key[0x6] = state; break;
                    case SDLK_r: ch8->key[0xD] = state; break;
                    case SDLK_a: ch8->key[0x7] = state; break;
                    case SDLK_s: ch8->key[0x8] = state; break;
                    case SDLK_d: ch8->key[0x9] = state; break;
                    case SDLK_f: ch8->key[0xE] = state; break;
                    case SDLK_z: ch8->key[0xA] = state; break;
                    case SDLK_x: ch8->key[0x0] = state; break;
                    case SDLK_c: ch8->key[0xB] = state; break;
                    case SDLK_v: ch8->key[0xF] = state; break;
                }
            }
        }

        cycle(ch8);
        
        if (ch8->flag) {
            draw_chip8(render, ch8);
            ch8->flag = 0;
        }
        
        if (SDL_GetTicks() - last_timer >= 16) {
            if (ch8->DT > 0) ch8->DT--;
            if (ch8->ST > 0) ch8->ST--;
            last_timer = SDL_GetTicks();
        }
        SDL_Delay(2);
    }

    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
    freeCh8(ch8);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path_to_rom>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    ch8start(argv[1]);
    SDL_Quit();
    return EXIT_SUCCESS;
}
