@echo off
rem Папка этого .bat (с завершающим )
set "SCRIPT_DIR=%~dp0"

rem Корень BorealEngine — родитель папки scripts (без завершающего )
set "ROOT=%SCRIPT_DIR%.."

rem Преобразовать в абсолютный путь (без завершающего )
for %%I in ("%ROOT%") do set "ROOT=%%~fI"

rem Папка для .sln
set "BUILD=%ROOT%\engine"

if not exist "%BUILD%" mkdir "%BUILD%"

rem Запуск cmake (замените генератор/архитектуру при необходимости)
cmake -S "%ROOT%" -B "%BUILD%" -G "Visual Studio 17 2022" -A x64