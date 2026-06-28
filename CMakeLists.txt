cmake_minimum_required(VERSION 3.20)
project(PCRetro C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Release 默认配置
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 严格警告
if(MSVC)
    add_compile_options(/W4 /utf-8 /D_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(_WIN32_WINNT=0x0601 WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()

# Windows 库
list(APPEND PLATFORM_LIBS
    user32 kernel32 advapi32 shell32 ole32 gdi32 comctl32
    winhttp crypt32 secur32 bcrypt ws2_32 oleaut32
)

# 找到 SQLite3
find_package(SQLite3 REQUIRED)

# ---------- 公共静态库 ----------
set(PCRETRO_CORE_SOURCES
    src/common.c
    src/storage/db.c
    src/storage/repo.c
    src/collectors/app_usage.c
    src/collectors/web_history.c
    src/collectors/file_ops.c
    src/collectors/runner.c
    src/query/time_ranges.c
    src/query/search.c
    src/ai/ollama.c
    src/ai/summarizer.c
    src/control/blacklist.c
    src/control/time_limits.c
    src/control/enforcer.c
    src/security/password.c
    src/security/lockout.c
    src/security/hotkey.c
    src/guard/acl.c
    src/guard/auto_start.c
    src/guard/installer.c
)
add_library(pcretro_core STATIC ${PCRETRO_CORE_SOURCES})
target_include_directories(pcretro_core PUBLIC src)
target_link_libraries(pcretro_core PUBLIC SQLite::SQLite3 ${PLATFORM_LIBS})

# ---------- UI 主程序 ----------
add_executable(PCRetroUI
    src/main.c
    src/ui/main_window.c
    src/ui/login.c
    src/ui/event_view.c
    src/ui/stats_view.c
    src/ui/ai_view.c
    src/ui/control_view.c
    src/ui/settings_view.c
    src/ui/tray.c
    src/ui/boot_notice.c
    src/ui/dialogs.c
)
target_link_libraries(PCRetroUI PRIVATE pcretro_core)
set_target_properties(PCRetroUI PROPERTIES WIN32_EXECUTABLE TRUE)

# ---------- Watcher 服务 ----------
add_executable(PCRetroWatcher
    src/guard/watcher_service.c
)
target_link_libraries(PCRetroWatcher PRIVATE pcretro_core)
set_target_properties(PCRetroWatcher PROPERTIES WIN32_EXECUTABLE TRUE)

# ---------- 安装器（控制台：sc 注册服务等） ----------
add_executable(PCRetroInstaller
    src/installer/install_wizard.c
)
target_link_libraries(PCRetroInstaller PRIVATE pcretro_core)

# ---------- 卸载器 ----------
add_executable(PCRetroUninstaller
    src/installer/uninstall.c
)
target_link_libraries(PCRetroUninstaller PRIVATE pcretro_core)
# 卸载器保留控制台，便于显示提示

# ---------- 测试 ----------
enable_testing()
add_executable(test_time_ranges tests/test_time_ranges.c)
target_link_libraries(test_time_ranges PRIVATE pcretro_core)
add_test(NAME test_time_ranges COMMAND test_time_ranges)

add_executable(test_stats tests/test_stats.c)
target_link_libraries(test_stats PRIVATE pcretro_core)
add_test(NAME test_stats COMMAND test_stats)

add_executable(test_password tests/test_password.c)
target_link_libraries(test_password PRIVATE pcretro_core)
add_test(NAME test_password COMMAND test_password)

# ---------- 安装 ----------
install(TARGETS PCRetroUI PCRetroWatcher PCRetroInstaller PCRetroUninstaller
        RUNTIME DESTINATION bin)
install(FILES installer/DISCLAIMER.txt DESTINATION .)
install(FILES installer/wix/Product.wxs DESTINATION .)

# ---------- CPack (MSI) ----------
set(CPACK_GENERATOR WIX)
set(CPACK_WIX_PRODUCT_GUID "4F8A9D2C-5B7E-4A3D-9F1E-7C8B2A6D4E5F")
set(CPACK_WIX_UPGRADE_GUID "8A2C5B9D-1E3F-4D7A-B6C9-2E4F8A1B5C7D")
set(CPACK_PACKAGE_NAME "PCRetro")
set(CPACK_PACKAGE_VENDOR "PCRetro Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "家长对孩子电脑使用进行合法、透明管理的软件")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_WIX_PRODUCT_NAME "电脑回溯器（家长管控版）")
set(CPACK_WIX_LICENSE_RTF "${CMAKE_SOURCE_DIR}/installer/DISCLAIMER.txt")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/installer/DISCLAIMER.txt")
include(CPack)
