cmake_minimum_required(VERSION 3.14)
project(ABSchat)

# C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем rconpp из subdirectory
add_subdirectory(external/rconpp-1.0.0)
add_subdirectory(external/tg-bot-cpp)

# Указываем директории для include
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/external/rconpp-1.0.0/include
    ${PROJECT_SOURCE_DIR}/external/tg-bot-cpp/include
)

# Добавляем основной исполняемый файл
add_executable(ABSchat
    main.cpp
    Config.hpp HardCode.hpp LogOutput.hpp Service.hpp
    ClientBrocker.hpp ClientSubscriber.hpp MinecraftChatSubscriber.hpp TgBotSubscriber.hpp
    ChatsList.hpp
)

# Линкуем с rconpp
target_link_libraries(ABSchat rconpp)
