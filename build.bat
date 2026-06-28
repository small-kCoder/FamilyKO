@echo off
REM 电脑回溯器（C 版本）一键构建脚本
REM 依赖：CMake 3.20+, Visual Studio 2019/2022（带 C 编译器），可选 WiX 3.x
REM 输出：build\Release\PCRetroUI.exe 等，MSI 在 dist\ 下

setlocal

set ROOT=%~dp0
cd /d "%ROOT%"

REM 1. 配置
if not exist build mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..
if errorlevel 1 (
    echo [ERROR] CMake 配置失败
    exit /b 1
)

REM 2. 编译
cmake --build . --config Release --parallel
if errorlevel 1 (
    echo [ERROR] 编译失败
    exit /b 1
)

REM 3. 收集产物到 dist
if not exist ..\dist mkdir ..\dist
copy /Y "Release\PCRetroUI.exe"          ..\dist\
copy /Y "Release\PCRetroWatcher.exe"     ..\dist\
copy /Y "Release\PCRetroInstaller.exe"   ..\dist\
copy /Y "Release\PCRetroUninstaller.exe" ..\dist\
copy /Y "..\installer\DISCLAIMER.txt"    ..\dist\

REM 4. 跑测试
echo.
echo ===== 运行单元测试 =====
ctest -C Release --output-on-failure
if errorlevel 1 (
    echo [WARN] 部分测试失败
)

REM 5. 打 MSI（需安装 WiX 3.x）
echo.
echo ===== 打包 MSI =====
if exist "C:\wix\candle.exe" (
    set WIX="C:\wix"
) else if exist "%WIX_HOME%\candle.exe" (
    set WIX=%WIX_HOME%
) else (
    echo [WARN] 未找到 WiX 工具链，跳过 MSI 打包
    echo        请安装 WiX 3.14+: https://wixtoolset.org/releases/
    goto :end
)

mkdir ..\dist\msi_obj 2>nul
%WIX%\candle.exe -dPCRetroSrc="..\dist" -out "..\dist\msi_obj\\" "..\installer\wix\Product.wxs" "..\installer\wix\UI.wxs"
%WIX%\light.exe -ext WixUIExtension -cultures:zh-CN -out "..\dist\PCRetro-1.0.0.msi" "..\dist\msi_obj\Product.wixobj"
if errorlevel 1 (
    echo [ERROR] MSI 打包失败
    exit /b 1
)
echo MSI 已生成：..\dist\PCRetro-1.0.0.msi

:end
echo.
echo ===== 完成 =====
echo 可执行文件：%ROOT%dist\
endlocal
