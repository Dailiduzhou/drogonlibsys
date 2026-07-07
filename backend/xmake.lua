local backend_dir = os.scriptdir()

set_arch("x86_64")

-- Debug / Release 两种构建模式
add_rules("mode.debug", "mode.release", "mode.releasedbg")

-- 编译参数
set_warnings("all", "extra")
if is_mode("debug") then
	add_defines("DEBUG")
	set_symbols("debug")
	add_cxflags("-g", "-O0", { force = true })
	add_cxflags("-fsanitize=address,undefined", { force = true })
	add_ldflags("-fsanitize=address,undefined", { force = true })
elseif is_mode("release") then
	set_optimize("fastest")
	set_strip("all")
end

add_requires("drogon", { system = true })
add_requires("jwt-cpp", "libxcrypt")
add_requires("yaml-cpp", { system = true })
add_requires("aws-sdk-cpp", { configs = { build_only = "s3" } })
add_requires("hiredis", { system = true })

-- 纯 C 子模块: 密码哈希 (bcrypt via libxcrypt), 与 Drogon 解耦
-- 用 clang 作为 C 编译器, 静态链接进 drogonlibsys
target("libsys_c")
set_kind("static")
set_languages("c11")
add_files(path.join(backend_dir, "csrc/*.c"))
add_includedirs(path.join(backend_dir, "csrc/include"))
add_packages("libxcrypt")
set_targetdir("build/bin")

target("drogonlibsys")
set_kind("binary")
add_files(path.join(backend_dir, "src/**.cc"))
add_includedirs(
	path.join(backend_dir, "include"),
	path.join(backend_dir, "src"),
	path.join(backend_dir, "csrc/include")
)
add_deps("libsys_c")
add_packages("drogon", "jwt-cpp", "yaml-cpp", "aws-sdk-cpp", "hiredis", "libxcrypt")
add_syslinks("curl", "ssl", "crypto", "pq", "sqlite3")
set_targetdir("build/bin")

target("tests")
set_kind("binary")
-- 仅当 tests 目录存在时才加入 (Docker .dockerignore 会排除 tests/)
if os.isdir(path.join(backend_dir, "tests")) then
	add_files(path.join(backend_dir, "tests/*.cpp"))
end
add_includedirs(
	path.join(backend_dir, "include"),
	path.join(backend_dir, "src"),
	path.join(backend_dir, "csrc/include")
)
add_deps("libsys_c")
add_packages("drogon", "jwt-cpp", "yaml-cpp", "aws-sdk-cpp", "hiredis", "libxcrypt")
add_syslinks("curl", "ssl", "crypto", "pq", "sqlite3")
set_targetdir("build/bin")
set_default(false)
