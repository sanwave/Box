
#SingleInstance force
#NoEnv
#IfWinActive, ahk_class Warcraft III
SendMode Input
SetMouseDelay, -1，-1
SetKeyDelay, -1，-1
Gosub, D_Window      ;进入主界面子程序
Gosub, D_Tray ;进入托盘子程序
;以下为显血部分;
Loop
{
	IfWinActive, Warcraft III ahk_class Warcraft III
	{
		If (m_bIn == 0)
		{
			If (AllyHB == 1)
			{
				Send, {tab}
				Sleep 200
				Send, {[ Down}
			}
			If (EnemyHB == 1)
			{
				Send, {tab}
				Sleep 200
				Send, {] Down}
			}
			m_bIn := 1
		}
	}
	Else
	{
		If (m_bIn == 1)
		m_bIn := 0
	}
	Sleep 200
}
;============================;
D_Window: ;主界面
{
	Menu, SetMenu, Add, 保存设置 (&S), D_Define
	Menu, SetMenu, Add
	Menu, SetMenu, Add, 退出程序 (&X), D_AppExit
	Menu, HelpMenu, Add, 关于 (&A), D_HelpAbout
	Menu, MyMenu, Add, 程序设置 (&D), :SetMenu
	Menu, MyMenu, Add, 帮助 (&H), :HelpMenu
	Gui, Menu, MyMenu
	Gui, Add, GroupBox, x16 y7 w180 h130 , 物品栏
	Gui, Add, GroupBox, x16 y147 w180 h50 , 血条栏
	Gui, Add, Text, x26 y32 w90 h30 , 物品栏1：
	Gui, Add, Text, x26 y92 w90 h30 , 物品栏2：
	Gui, Add, CheckBox, Checked x26 y167 w80 h20 vAllyHB, 友方血条
	Gui, Add, CheckBox, Checked x106 y167 w80 h20 vEnemyHB, 敌方血条
	Gui, Add, Hotkey, x86 y27 w15 h20 vItem7, Q
	Gui, Add, Hotkey, x86 y87 w15 h20 vItem8, ~
	Gui, Show, x301 y147 h233 w217, MicrOperation
	Gosub, D_Define ;进入定义子程序
	Return
}
;============================;
D_Tray:
{
	Menu, Tray, NoStandard
	Menu, Tray, Add, 设置
	Menu, Tray, Add, 退出
	Menu, Tray, Default, 设置
	Menu, Tray, Click, 1
	Menu, Tray, Tip, MicrOperation
	Menu, Tray, Icon, , , 1
	Return
}
;============================;
D_Define:
{ ;下面两句是为了取消先前定义的热键，要是没有后果很严重，很多键乱了套。当然只定义一次是没问题的，要是定义后再改就会出现问题。
	if Item7
	HotKey, ~%Item7%, D_Item7,Off
	if Item8
	HotKey, ~%Item8%, D_Item8,Off
	Gui, Submit ;托盘程序，并使用户输入信息与相关变量关联
	if Item7
	HotKey, ~%Item7%, D_Item7,On
	if Item8
	HotKey, ~%Item8%, D_Item8,On
	Return
}
;============================;
D_Item7:
{
	Send, {Numpad7}{BS} ;这里出现一个退格键足矣，相信不会有人定义组合键的，dota讲的是速度和操作，按键越简单越好
	return
}
;============================;
D_Item8:
{
	Send, {Numpad8}{BS}
	return
}
;================================;
设置:
{
	Gui, Show, , MicrOperation
	Return
}
;============================;
D_HelpAbout:
{
	Msgbox , 0, MicrOperation,AutoHotkey版权所有
	Return
}
;============================;
D_AppExit:
GuiClose:
退出:
{
	ExitApp ;退出程序
	Return
}
GuiSize:
{
	If (A_EventInfo==1) ;这里只是针对窗口最小化事件处理，就是将程序托盘并关联变量
	Gosub, D_Define
}
;============================;
LWin:: ;屏蔽左Windows键
Return