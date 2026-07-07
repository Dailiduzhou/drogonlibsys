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

add_requires("drogon", "jwt-cpp", "libxcrypt")
add_requires("yaml-cpp", { system = true })
add_requires("aws-sdk-cpp", { configs = { build_only = "s3" } })
add_requires("hiredis", { system = true })

target("drogonlibsys")
set_kind("binary")
add_files(path.join(backend_dir, "src/**.cc"))
add_includedirs(path.join(backend_dir, "include"), path.join(backend_dir, "src"))
add_packages("drogon", "jwt-cpp", "yaml-cpp", "aws-sdk-cpp", "hiredis", "libxcrypt", "sqlite3")
add_syslinks("curl", "ssl", "crypto", "pq", "sqlite3")
set_targetdir("build/bin")

target("tests")
set_kind("binary")
add_files(path.join(backend_dir, "tests/test_main.cpp"))
add_includedirs(path.join(backend_dir, "include"), path.join(backend_dir, "src"))
add_packages("drogon", "jwt-cpp", "yaml-cpp", "aws-sdk-cpp", "hiredis", "libxcrypt", "sqlite3")
add_syslinks("curl", "ssl", "crypto", "pq", "sqlite3")
set_targetdir("build/bin")
set_default(false)
