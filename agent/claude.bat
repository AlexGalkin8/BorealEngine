@echo off
rem Starts Claude Code from the agent directory so that all agent
rem configuration stays inside it.
cd /d "%~dp0"
claude %*
