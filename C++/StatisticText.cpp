/*********************************************************************************************
*Name:0621String.cpp
*Author:San
*E-Mail:sxwangbo@live.com
*Date:2011.6.20
*Description:通过输入多行文本，以空行结束，改程序实现统计所有文本中的狭义字符数（不包括分隔符），
*单词数，行数。（本程序使用字符数组 char[]）。
*********************************************************************************************/

#include<iostream>
#include<string>
#include<cstring>
using namespace std;

//计算单行文本狭义字符数、单词数并累加到总字符数和单词数中
bool rowCount(int &Chars,int &Words)
{
	string str;            //用于存储单行文本
	getline(cin,str);    //获得整行内容，包括空格
	int rowWords=0;        //用于存储单行中单词数

	if((str[0]>='a'&&str[0]<='z'||str[0]>='A'&&str[0]<='Z'||str[0]>='0'&&str[0]<='9')&&str.length()==1) 
	{
		str.append(".");
	} //识别并规范只有一个狭义字符（不包括分隔符）的文本行

	for(int i=0;i<str.length();i++) 
	{
		if((!(str[i]>='a'&&str[i]<='z'||str[i]>='A'&&str[i]<='Z'||str[i]>='0'&&str[i]<='9'))) 
		{
			i==0&&--Chars+5||rowWords++;
		} //判断并计算单行文本中单词数：若某行第一个字符为分隔符，则减去其在字符数中的占位

		else if(i==str.length()-1) 
		{
			str.append(".");
		} //识别并规范末尾无分隔符的文本行
	}

	Chars+=str.length()-rowWords;
	Words+=rowWords;
	//返回对该文本行是否为空行的判断值：若为空行，返回True
	return str.empty();
}

bool rowCount2(int &Chars,int &Words)
{
	char sptr[37001];     //用于存储单行文本
	cin.getline(sptr,37000);      //获得整行内容，包括空格（最多支持37000个字符）
	int rowWords=0,length=-1;      //分别用于存储单行中单词数和文本长度		
	if((sptr[0]>='a'&&sptr[0]<='z'||sptr[0]>='A'&&sptr[0]<='Z'||sptr[0]>='0'&&sptr[0]<='9')&&sptr[1]==NULL)
		strcat(sptr,".");                  //识别并规范只有一个狭义字符（不包括分隔符）的文本行	
	do
	{
		length++;
	}while(sptr[length]!=NULL);              //获取sptr的内容长度

	for(int i=0;sptr[i]!=NULL&&length!=0;i++)
	{         
		if(!(sptr[i]>='a'&&sptr[i]<='z'||sptr[i]>='A'&&sptr[i]<='Z'||sptr[i]>='0'&&sptr[i]<='9'))
		{
			if(i==0)
				Chars--;
			else 
				rowWords++;                   //判断并计算单行文本中单词数：若某行第一个字符为分隔符，则减去其在字符数中的占位
		}
		else if(i==length-1)
			strcat(sptr,".");             //识别并规范末尾无分隔符的文本行
	}
	if(sptr[length]!=NULL)
		length++;	         //使length的值与sptr的内容长度保持一致

	Chars+=length-rowWords;
	Words+=rowWords;
	return sptr[0]==NULL;           //返回对该文本行是否为空行的判断值：若为空行，返回True
}

void main(){
	int Chars=0,Words=0,Rows=0;              //分别用于存储所有文本行的狭义字符数（不包括分隔符），单词数，行数
	cout<<"\n请输入多行文本，以空行结束：\n";
	while(1){
		if(rowCount(Chars,Words)) 
			break;
		Rows++;             //循环调用rowCount函数，并通过该函数返回值判断是否结束
	}
	cout<<"字符数："<<Chars<<endl
		<<"单词数："<<Words<<endl
		<<"行数："<<Rows<<endl;		
}
