set_allowedarchs("x64")
set_languages("c99", "c++17")

packages = {"glfw", "glad", "glm", "spdlog", "yaml-cpp", "entt", "stb"}

add_requires(packages)

target("deps")
    set_kind("static")
    add_files("deps/*/*.cpp")
    add_headerfiles("deps/*/*.h")
    add_packages("glfw", "glad")

target("minecraft-clone")
    set_kind("binary")
    add_files("src/*.cpp", "src/*/*.cpp", "src/*/*/*.cpp")
    add_headerfiles("src/*.h", "src/*/*.h", "src/*/*/*.h")
    set_optimize("fastest")
    add_deps("deps")
    add_packages(packages)