-- 图书管理系统 (Library Management System)
-- 基于 Drogon + Xmake 的 C++ 后端工程
--
-- 依赖来源:
--   xrepo (源码构建): drogon, jwt-cpp
--   系统 (pacman):    libpqxx, hiredis, libpq  (xrepo 的 libpq 构建受 Perl 模块问题阻塞)
--   MinIO:            暂为桩实现 (xrepo minio-cpp v0.4.0 配方 INIReader 链接缺失)

local backend_dir = os.scriptdir()

set_arch("x86_64")

-- Debug / Release 两种构建模式
add_rules("mode.debug", "mode.release", "mode.releasedbg")

-- 编译参数
set_warnings("all", "extra")
if is_mode("debug") then
    add_defines("DEBUG")
    set_symbols("debug")
    add_cxflags("-g", "-O0", {force = true})
    add_cxflags("-fsanitize=address,undefined", {force = true})
    add_ldflags("-fsanitize=address,undefined", {force = true})
elseif is_mode("release") then
    set_optimize("fastest")
    set_strip("all")
end

-- 第三方依赖
-- drogon / jwt-cpp: 从 xrepo 源码构建
add_requires("drogon", "jwt-cpp")
-- libpqxx / hiredis: 使用系统包 (pacman 安装, pkg-config 发现)
add_requires("libpqxx", "hiredis", {system = true})

target("drogonlibsys")
    set_kind("binary")
    add_files(path.join(backend_dir, "src/**.cc"))
    add_includedirs(path.join(backend_dir, "include"), path.join(backend_dir, "src"))
    add_packages("drogon", "jwt-cpp", "libpqxx", "hiredis")
    set_targetdir("build/bin")

target("tests")
    set_kind("binary")
    add_files(path.join(backend_dir, "tests/test_main.cpp"))
    add_includedirs(path.join(backend_dir, "include"), path.join(backend_dir, "src"))
    add_packages("drogon", "jwt-cpp", "libpqxx", "hiredis")
    set_targetdir("build/bin")
    set_default(false)
