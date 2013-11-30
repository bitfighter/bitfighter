rem Building the notifier requires pyinstaller -- install with "pip install pyinstaller" 
@echo off
echo Building notifier executable...
pyinstaller --onefile --noconsole --icon=..\redship48.ico ..\bitfighter_notifier.py
echo Created .\dist\bitfighter_notifier.exe
echo Distribute this exe along with the redship48.ico icon file in the parent folder
echo NSIS will find the exe in the dist folder.
