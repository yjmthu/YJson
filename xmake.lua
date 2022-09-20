set_languages("cxx20")
add_defines("UNICODE", "_UNICODE")

if is_plat("windows") then
  add_cxflags("/utf-8")
end

add_includedirs("include")

target("yjson")
  set_kind("static")
  add_files("src/yjson.cpp")
target_end()

target("yjson_test")
  set_kind("binary")
  add_files("test/main.cpp")
  add_deps("yjson")
target_end()

