FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(json)