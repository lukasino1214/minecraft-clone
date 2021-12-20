set_allowedarchs("x64")
set_languages("c99", "c++17")

package("FastNoise2")
    add_urls("https://github.com/Auburn/FastNoise2.git")

    on_install(function (package)
        local configs = {}
        if package:config("shared") then
            configs.kind = "shared"
        end
        import("package.tools.cmake").install(package, configs)
    end)
package_end()

packages = {"FastNoise2", "glfw", "glad", "glm", "spdlog", "yaml-cpp", "entt", "stb"}
add_requires(packages)

target("deps")
    set_kind("static")
    add_files("deps/imgui/*.cpp")
    add_headerfiles("deps/imgui/*.h")
    add_packages("glfw", "glad")
target_end()

target("minecraft-clone")
    set_kind("binary")
    add_files("src/*.cpp", "src/*/*.cpp", "src/*/*/*.cpp", "src/*/*/*/*.cpp")
    add_headerfiles("src/*.h", "src/*/*.h", "src/*/*/*.h", "src/*/*/*/*.h")
    set_optimize("fastest")
    add_deps("deps")
    add_packages(packages)
target_end()