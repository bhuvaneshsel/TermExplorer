cmake_minimum_required(VERSION 3.18)
project(TermExplorer VERSION 1.0)

include(FetchContent)
 
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v6.1.9  # Replace with a version, tag, or commit hash
)
 
FetchContent_MakeAvailable(ftxui)
 
add_executable(TermExplorer)
target_sources(TermExplorer PRIVATE
    main.cpp
    file_tree.cpp
    tui.cpp
)

target_link_libraries(TermExplorer
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)
