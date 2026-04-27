@echo off
echo Compiling Expression Tree Builder...
gcc main.c -lgdi32 -mwindows -o ExpressionTreeBuilder.exe
if %ERRORLEVEL% EQU 0 (
    echo Build successful! Run ExpressionTreeBuilder.exe to start.
) else (
    echo Build failed. Please check for errors.
)
pause
