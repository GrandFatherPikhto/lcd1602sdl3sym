#include <SDL3/SDL.h>
#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_thread.h>

#include <stdio.h>

typedef struct {
    int quit;
    SDL_Mutex *mutex;
    SDL_Condition  *cond;
    int event_available;
} SharedState;

static int input_thread(void *arg) {
    SDL_Window *window = NULL;
    SDL_Event e;
    SharedState *state = (SharedState *)arg;

    // Не создаём окно здесь; предполагаем, что окно создано в главной нити.
    // В этом примере поток просто обрабатывает ввод и уведомляет главный поток.

    printf("Input thread started\n");

    while (1) {
        // Блокируем до появления события ввода
        if (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_USER + 0) {
                // пользовательское событие для выхода
                break;
            }
            // Пересылка события в главный поток
            SDL_Event gated;

            gated.user.code = 0;
            gated.user.data1 = (void *)"input_event";
            if (e.key.key == SDLK_Q)
            {
                printf("Event Q\n");
                gated.type = SDL_EVENT_QUIT;
            } else 
            {
                gated.type = SDL_EVENT_USER + 1; // обозначим как уведомление
            }

            // Если нужно, можно передать оригинальное событие тоже
            SDL_PushEvent(&gated); // или можно использовать SDL_QueueEvent если доступно
        }
        // Небольшая задержка, чтобы не расходовать CPU
        SDL_Delay(10);
    }

    printf("Input thread exiting\n");
    return 0;
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == false) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("SDL Threaded Input",
                                       640, 480,
                                       0);
    if (!win) {
        fprintf(stderr, "CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Локальные средства синхронизации можно добавить по мере необходимости
    SharedState state = {0};
    state.mutex = SDL_CreateMutex();
    state.cond  = NULL; // можно добавить условную переменную
    state.event_available = 0;

    // Создаём поток обработки ввода
    SDL_Thread *thread = SDL_CreateThread(input_thread, "InputThread", (void *)&state);
    if (!thread) {
        fprintf(stderr, "CreateThread failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    // Основной цикл: обрабатываем события, включая уведомления из потока ввода
    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                qprintf("e: %d\n", e.type);
                running = 0;
            } else if (e.type == SDL_EVENT_USER + 1) {
                // обработка уведомления об вводе
                printf("Main thread: input event received\n");
            } else {
                // обработка прочих событий окна
            }
        }

        // Здесь можно делать рендеринг или логику
        SDL_Delay(16);
    }

    // Завершение
    // посылаем сигнал завершения в поток
    SDL_Event quit;
    quit.type = SDL_EVENT_USER + 0;
    SDL_PushEvent(&quit);

    SDL_WaitThread(thread, NULL);
    SDL_DestroyWindow(win);
    SDL_DestroyMutex(state.mutex);
    SDL_Quit();
    return 0;
}