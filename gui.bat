@echo off
rem Start the EMTSim circuit editor in your web browser.
rem Usage: gui.bat   (then press Ctrl+C in this window to stop it)
setlocal
set "PY=%~dp0.venv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"
"%PY%" "%~dp0gui\server.py" %*
