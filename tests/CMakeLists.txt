FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.14.0
)

FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

add_executable(tests
    test_parameter_view.cpp
    test_parameter_tree.cpp
    test_processor_chain.cpp
    test_source_mixer.cpp
    test_dsp.cpp
    test_looper_processor.cpp
)

target_link_libraries(tests PRIVATE
    ${PROJECT_NAME}_core
    Catch2::Catch2WithMain
)

catch_discover_tests(tests)