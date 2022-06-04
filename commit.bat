@echo off

NoteCollector.exe
git add *
git status

cmd /k my_script.bat