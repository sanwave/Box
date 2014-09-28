@ECHO OFF

echo 正在激活Office 2010,请不要关闭计算机
Start /Wait %SystemDrive%\Temp\FirstLogonRun\"Office 2010 Toolkit.exe" /Ez-Activator


echo 正在激活Windows 7，请不要关闭计算机
Start /Wait %SystemDrive%\Temp\FirstLogonRun\WindowsLoader\"Windows Loader.exe" /silent /norestart


echo 激活成功！计算机即将重新启动....

rd /s/q %SystemDrive%\Temp

Shutdown -r -t 10

pause
