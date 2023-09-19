
setlocal ENABLEDELAYEDEXPANSION
set str=%2
set str=%str:~0,-3%.html
echo %str%
tools\web\static-server %1 8123 %str%