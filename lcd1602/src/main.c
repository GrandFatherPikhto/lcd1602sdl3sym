#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "chars.h"

// Размеры символа и дисплея
#define CHAR_WIDTH 5
#define CHAR_HEIGHT 8
#define PIXEL_SCALE 4
#define LCD_WIDTH 16
#define LCD_HEIGHT 2
#define DISPLAY_WIDTH (LCD_WIDTH * (CHAR_WIDTH + 1) * PIXEL_SCALE)
#define DISPLAY_HEIGHT (LCD_HEIGHT * (CHAR_HEIGHT + 1) * PIXEL_SCALE)

// Цвета
#define BACKGROUND_COLOR 0x30, 0x30, 0x40, 0xFF
#define PIXEL_OFF_COLOR 0x60, 0x60, 0x70, 0xFF
#define PIXEL_ON_COLOR 0xE0, 0xE0, 0xF0, 0xFF
#define TEXT_COLOR 0xFF, 0xFF, 0xFF, 0xFF

// Структура для эмуляции состояния LCD1602
typedef struct {
    uint8_t ddram[80];  // Display Data RAM (80 байт)
    uint8_t cgram[64];  // Character Generator RAM (8 символов × 8 байт)
    uint8_t cursor_x;
    uint8_t cursor_y;
    bool display_on;
    bool cursor_on;
    bool blink_on;
    bool backlight;
} lcd1602_state_t;

// Глобальные переменные
static lcd1602_state_t lcd_state;
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static bool running = true;

// Инициализация состояния LCD
void lcd_init(void) {
    memset(&lcd_state, 0, sizeof(lcd_state));
    lcd_state.display_on = true;
    lcd_state.backlight = true;
    
    // Заполняем DDRAM пробелами
    for (int i = 0; i < 80; i++) {
        lcd_state.ddram[i] = ' ';
    }
}

// Функция записи данных в LCD
void lcd_write_data(uint8_t data) {
    if (lcd_state.cursor_y < LCD_HEIGHT && lcd_state.cursor_x < LCD_WIDTH) {
        int index = lcd_state.cursor_y * 40 + lcd_state.cursor_x;
        if (index < 80) {
            lcd_state.ddram[index] = data;
        }
    }
    
    lcd_state.cursor_x++;
    if (lcd_state.cursor_x >= LCD_WIDTH) {
        lcd_state.cursor_x = 0;
        lcd_state.cursor_y++;
        if (lcd_state.cursor_y >= LCD_HEIGHT) {
            lcd_state.cursor_y = 0;
        }
    }
}

// Функция записи команды в LCD
void lcd_write_command(uint8_t cmd) {
    // Обработка команд очистки дисплея и возврата домой
    if (cmd == 0x01) {
        // Clear display
        for (int i = 0; i < 80; i++) {
            lcd_state.ddram[i] = ' ';
        }
        lcd_state.cursor_x = 0;
        lcd_state.cursor_y = 0;
    } else if (cmd == 0x02) {
        // Return home
        lcd_state.cursor_x = 0;
        lcd_state.cursor_y = 0;
    }
    // Здесь можно добавить обработку других команд
}

// Отрисовка одного символа
void draw_char(uint8_t char_code, int x_pos, int y_pos) {
    uint8_t char_data[8];
    
    // Получаем данные символа из CGRAM или используем встроенный шрифт
    if (char_code < 8) {
        // Символ из CGRAM
        memcpy(char_data, &lcd_state.cgram[char_code * 8], 8);
    } else {
        // Встроенный символ - упрощенная реализация
        // В реальном эмуляторе здесь должен быть полный набор символов HD44780
        const uint8_t *data = get_char_data(char_code);
        memcpy(char_data, data, 8);
        for (int i = 0; i < 8; i++) {
            char_data[i] = data[i]; // Простая заглушка
        }
    }
    
    // Отрисовка пикселей символа
    SDL_FRect pixel_rect;
    pixel_rect.w = PIXEL_SCALE;
    pixel_rect.h = PIXEL_SCALE;
    
    for (int y = 0; y < CHAR_HEIGHT; y++) {
        for (int x = 0; x < CHAR_WIDTH; x++) {
            pixel_rect.x = (x_pos * (CHAR_WIDTH + 1) + x) * PIXEL_SCALE;
            pixel_rect.y = (y_pos * (CHAR_HEIGHT + 1) + y) * PIXEL_SCALE;
            
            // Проверяем бит в данных символа
            bool pixel_on = (char_data[y] & (1 << (CHAR_WIDTH - 1 - x))) != 0;
            
            if (pixel_on && lcd_state.display_on) {
                SDL_SetRenderDrawColor(renderer, PIXEL_ON_COLOR);
            } else {
                SDL_SetRenderDrawColor(renderer, PIXEL_OFF_COLOR);
            }
            SDL_RenderFillRect(renderer, &pixel_rect);
        }
    }
}

// Отрисовка всего дисплея
void render_display(void) {
    // Очистка экрана
    SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR);
    SDL_RenderClear(renderer);
    
    // Отрисовка всех символов
    for (int y = 0; y < LCD_HEIGHT; y++) {
        for (int x = 0; x < LCD_WIDTH; x++) {
            int index = y * 40 + x;
            if (index < 80) {
                draw_char(lcd_state.ddram[index], x, y);
            }
        }
    }
    
    // Отрисовка курсора
    if (lcd_state.cursor_on && lcd_state.display_on) {
        SDL_FRect cursor_rect = {
            lcd_state.cursor_x * (CHAR_WIDTH + 1) * PIXEL_SCALE,
            (lcd_state.cursor_y * (CHAR_HEIGHT + 1) + CHAR_HEIGHT) * PIXEL_SCALE,
            CHAR_WIDTH * PIXEL_SCALE,
            PIXEL_SCALE
        };
        SDL_SetRenderDrawColor(renderer, PIXEL_ON_COLOR);
        SDL_RenderFillRect(renderer, &cursor_rect);
    }
    
    // Обновление рендерера
    SDL_RenderPresent(renderer);
}

// Обработка входных событий
void handle_events(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
                
            case SDL_EVENT_KEY_DOWN:
                switch (event.key.key) {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    case SDLK_C:
                        if (event.key.mod & SDL_KMOD_CTRL) {
                            lcd_write_command(0x01); // Clear display
                        }
                        break;
                    case SDLK_H:
                        if (event.key.mod & SDL_KMOD_CTRL) {
                            lcd_write_command(0x02); // Return home
                        }
                        break;
                    // Добавьте другие клавиши для управления эмулятором
                }
                break;
                
            case SDL_EVENT_TEXT_INPUT:
                // Ввод текста непосредственно в LCD
                for (int i = 0; event.text.text[i] != '\0'; i++) {
                    lcd_write_data(event.text.text[i]);
                }
                break;
        }
    }
}

// Основная функция
int main(int argc, char* argv[]) {
    // Инициализация SDL
    if (SDL_Init(SDL_INIT_VIDEO) != true) {
        fprintf(stderr, "Ошибка инициализации SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    // Создание окна
    window = SDL_CreateWindow("LCD1602", DISPLAY_WIDTH, DISPLAY_HEIGHT, 0);
    if (!window) {
        fprintf(stderr, "Ошибка создания окна: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Создание рендерера
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        fprintf(stderr, "Ошибка создания рендерера: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Включение текстового ввода
    SDL_StartTextInput(window);
    
    // Инициализация LCD
    lcd_init();
    
    // Добавляем тестовый текст
    const char* test_text = "Hello, SDL3! LCD1602";
    for (int i = 0; test_text[i] != '\0' && i < LCD_WIDTH * LCD_HEIGHT; i++) {
        lcd_write_data(test_text[i]);
    }
    
    // Основной цикл
    while (running) {
        handle_events();
        render_display();
        SDL_Delay(16); // ~60 FPS
    }
    
    // Очистка ресурсов
    SDL_StopTextInput(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}