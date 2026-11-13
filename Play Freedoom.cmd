@echo off
setlocal
set "doom_bundle=%~dp0playable"
if not exist "%doom_bundle%\engine\crispy-doom.exe" goto missing
if not exist "%doom_bundle%\game\freedoom-0.13.0\freedoom1.wad" goto missing
start "" /D "%doom_bundle%\engine" "%doom_bundle%\engine\crispy-doom.exe" -iwad "%doom_bundle%\game\freedoom-0.13.0\freedoom1.wad" -fullscreen
exit /b

:missing
echo Extract Crispy Doom and Freedoom into the playable folder first.
pause
exit /b 1
