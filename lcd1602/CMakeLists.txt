cmake_minimum_required(VERSION 3.12)
project(lcd VERSION 1.0.0 LANGUAGES C)

# Настройка стандарта C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Поиск библиотеки SDL2
find_package(SDL3 REQUIRED)

# Добавление исполняемого файла
add_executable(lcd
    src/chars.c
    src/main.c
)

# Подключение SDL2 к целевому файлу
target_include_directories(lcd PRIVATE ${SDL2_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/include)
target_link_libraries(lcd 
    # ${SDL2_LIBRARIES}
    SDL3::SDL3
    )

# Для Windows дополнительные настройки
if(WIN32)
    target_link_libraries(lcd mingw32)
endif()

# Установка выходного каталога (опционально)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Добавление цели для запуска
add_custom_target(run
    COMMAND lcd
    DEPENDS lcd
    # WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)