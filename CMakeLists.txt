cmake_minimum_required(VERSION 3.16)

string(TIMESTAMP PLP_VER_DAY "%m%d" UTC)
string(TIMESTAMP PLP_VER_YEAR "%Y" UTC)
project(palette-picker
    VERSION 0.2.${PLP_VER_YEAR}.${PLP_VER_DAY}
    DESCRIPTION "Palette picker for a specific fighting game.")
set(PLP_VERSION ${PROJECT_VERSION})
string(REPLACE . , PLP_VERSION_COMMA ${PLP_VERSION})

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BASE_INSTALL_PATH palette-picker)

add_subdirectory(${PROJECT_SOURCE_DIR}/thirdparty/SokuLib)
add_library(palette-picker MODULE
  src/main.hpp
  src/main.cpp
  src/menu.hpp
  src/menu.cpp
  src/palette-list.hpp
  src/palette-list.cpp
  src/reader.cpp
)
target_link_libraries(palette-picker SokuLib)
set_target_properties(palette-picker PROPERTIES PREFIX "")

# file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/locale)
# install(TARGETS th123intl LIBRARY DESTINATION ${BASE_INSTALL_PATH})
# install(FILES
#     ${CMAKE_CURRENT_SOURCE_DIR}/pack/MonoSpatialModSWR.ttf
#     ${CMAKE_CURRENT_SOURCE_DIR}/pack/simhei.ttf
#     DESTINATION ${BASE_INSTALL_PATH})
# install(FILES
#     ${CMAKE_CURRENT_SOURCE_DIR}/locale/en.json
#     ${CMAKE_CURRENT_SOURCE_DIR}/locale/pt-BR.json
#     ${CMAKE_CURRENT_SOURCE_DIR}/locale/jp.json
#     ${CMAKE_CURRENT_SOURCE_DIR}/locale/zh.json
#     DESTINATION ${BASE_INSTALL_PATH}/locale)
