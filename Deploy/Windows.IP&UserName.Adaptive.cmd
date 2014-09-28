::
::*************************************************************************************************
::*************************************************************************************************
::
::     Adaptive 自动配置脚本 by Wave V0.99.2013.09.10
::
::*************************************************************************************************
::*************************************************************************************************



::关闭回显
@echo off

::设置背景色和前景色以及标题
Color 2F
title                                                   计算机基础实验室  2013
echo.
echo				计算机自动维护   by 计算机基础实验室
echo.
echo.



::*************************************************************************************************
::
::     BatchGotAdmin 提升在Win7系统下的权限，未发现XP有不良反应
::
::*************************************************************************************************
REM  --> Check for permissions
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"

REM --> If error flag set, we do not have admin.
if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    goto UACPrompt
) else ( goto gotAdmin )

:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%temp%\getadmin.vbs"
    "%temp%\getadmin.vbs"
    exit /B
	
:gotAdmin
    if exist "%temp%\getadmin.vbs" ( del "%temp%\getadmin.vbs" )
    pushd "%CD%"
    CD /D "%~dp0"

::*********************************************提升权限********************************************



::*************************************************************************************************
::
::     定义变量和显示
::
::*************************************************************************************************

::设置Mask和DNS
set Mask=255.255.252.0
set DNS=202.115.128.33
set DNS2=8.8.8.8
::获取网卡地址
for /f "tokens=1 delims=\" %%i in ('getmac /v ^| find /i "本地连接"') do set mac=%%i
set mac=%mac:~28%
::set MAC=%MAC:~28% 
::查表，获取对应的机器名和IP地址
for /f "tokens=1,3,4,5,6" %%i in ('findstr "%mac%" "C:\Windows\System32\Adaptive\MACTable.txt"') do set "name=%%i"&set "IP=%%j"&set "Gateway=%%k"&set "ExamServer=%%l"&set "House=%%m"
::获取当前操作系统版本并设置相应变量
for /f "tokens=2 delims=[" %%i in ('ver') do set SysVer=%%i
::Windows XP系统变量设置
if "%SysVer:~3,3%"=="5.1" (
	set StartupPath="%HomePath%\「开始」菜单\程序\启动"
	set AllStartupPath="%ALLUSERSPROFILE%\「开始」菜单\程序\启动"
	set Sys="Windows XP"
)
::Windows 7系统变量设置
if "%SysVer:~3,3%"=="6.1" (
	set StartupPath="%HomePath%\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup"
	set AllStartupPath="%ALLUSERSPROFILE%\Microsoft\Windows\Start Menu\Programs\Startup"
	set Sys="Windows 7"
)

::*********************************************变量定义********************************************



::*************************************************************************************************
::
::     倒计时模块
::
::*************************************************************************************************

::可在31秒内取消自动配置操作
set Time=31
for /l %%a in (%Time%,-1,0)do (
	cls&echo.
	echo				计算机自动维护   by 计算机基础实验室
	echo.
	echo.
	echo.
	echo 当前系统为 %Sys%  版本号：%SysVer:~3,8%
	echo.
	echo 该电脑将被设置为以下信息，请确认
	echo.
	echo name=%name%,  IP=%IP%,  MAC=%mac%
	echo.
	echo      Gateway=%Gateway%,  ExamServer=%ExamServer%, House=%House%
	echo.	
	echo 自动配置%%a秒后即将开始，按任意字母键可退出
	choice/c abcdefghijklmnopqrstuvwxyz1 /t 1 /d 1 >nul||if not errorlevel 27 exit
)
cls
::以下显示代码仅为清屏后保留以上信息显示
echo.
echo				计算机自动维护   by 计算机基础实验室
echo.
echo.
echo.
echo 当前系统为 %Sys%  版本号：%SysVer:~3,8%
echo.
echo 该电脑将被设置为以下信息，请确认
echo.
echo name=%name%,  IP=%IP%,  MAC=%mac%
echo.
echo      Gateway=%Gateway%,  ExamServer=%ExamServer%, House=%House%
echo.

::*********************************************倒计时模块******************************************



::*************************************************************************************************
::
::     主要内容
::
::*************************************************************************************************

echo 正在配置网络环境，请稍候.....
echo.
::配置网络设置
netsh interface ip set address "本地连接" static %IP% %Mask% %Gateway% 1 >nul 2>nul
netsh interface ip set dns "本地连接" static %DNS% register=PRIMARY >nul 2>nul
netsh interface ip add dns "本地连接"  %DNS2% index=2 >nul 2>nul


::将默认引导系统改为Windos 7
if "%SysVer:~3,3%"=="5.1" (
	xcopy /Y %SystemRoot%\System32\Adaptive\DefaultWin7\burg.cfg C:\burg\burg.cfg /H /R
	echo.
	echo 将默认引导 Windows 7
	echo.
)

::修改用户名
wmic useraccount where name="%username%" call rename "%name%"
::修改用户名全名
net user %name% /fullname:""
::修改计算机名。也可以直接修改注册表，不过，我觉得不好
wmic computersystem where name="%computername%" call rename "%name%"



::**********************************************正文结束*******************************************
choice /t 3 /c yn /d y
::删除启动文件
cd %StartupPath%
del /q *.bat
del /q *.cmd
cd %AllStartupPath%
del /q *.bat
del /q *.cmd
echo.
echo 启动配置文件已删除

if "%House%"=="7201" (
	::映射网络驱动器K盘
	net use K: /delete /y >nul
	net use K: \\%ExamServer%\Ncre38 "" /user:%name%
	::打开K盘
	explorer K:\%name%
)

if "%House%"=="6A507" (
	::映射网络驱动器K盘
	net use K: /delete /y >nul
	net use K: \\%ExamServer%\Ncre38 "" /user:%name%
	::打开K盘
	explorer K:\%name%
)

Shutdown -r -t 18
echo.
echo 即将关闭计算机.....
echo.

pause