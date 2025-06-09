cmake_minimum_required(VERSION 3.16)

project(Server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ASIO_USE_CPM "Download Asio with CPM instead of FetchContent" OFF)
option(
  ASIO_CPM_FETCHCONTENT_COMPAT
  "Should asio be declared with FetchContent functions to be compatible. This doesn't not allow CPM cache to work."
  ON
)
include(FetchContent)
FetchContent_Declare(
  jwt
  GIT_REPOSITORY "https://github.com/Thalhammer/jwt-cpp"
  GIT_TAG        "master"
)
set(BUILD_TESTS OFF CACHE BOOL "Build unit tests" FORCE)
FetchContent_Declare(rconpp
  GIT_REPOSITORY https://github.com/Jaskowicz1/rconpp
  GIT_TAG        main
)
FetchContent_MakeAvailable(jwt rconpp)
find_package(nlohmann_json REQUIRED)
find_package(sioclient REQUIRED)
find_package(CURL REQUIRED)

add_executable(Server Bot.hpp HardCode.hpp main.cpp FileSaver.hpp HTTPClient.hpp Config.hpp)

target_link_libraries(Server PRIVATE nlohmann_json)
target_link_libraries(Server PRIVATE jwt-cpp::jwt-cpp)
target_link_libraries(Server PRIVATE sioclient)
target_link_libraries(Server PRIVATE CURL::libcurl)
target_link_libraries(Server PRIVATE rconpp)
# tgbot
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
include_directories(/usr/local/include ${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIR})
if (CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    add_definitions(-DHAVE_CURL)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE /usr/local/lib/libTgBot.a ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES})

include(GNUInstallDirs)
install(TARGETS Server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
