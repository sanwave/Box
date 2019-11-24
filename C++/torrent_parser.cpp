
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include "stdint.h"
#include "assert.h"
#include "errno.h"

void replace(std::string &str, const std::string &old_value, const std::string &new_value)
{
    for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) 
    {
        if ((pos = str.find(old_value, pos)) != std::string::npos)
            str.replace(pos, old_value.length(), new_value);
        else
            break;
    }
}

int parse(int level, char *data, int len)
{
    assert(len>=0);
    if(len==0)
    {
        return 0;
    }
    int ret = -1;
    char indent[1024];
    memset(indent, 32, sizeof(indent));
    //std::cout<<std::string(indent, level*2);
    bool newline=false;
    switch (data[0])
    {
    case 'e':
        ret = 1;
        break;
        
    case 'i':
    {
        char *c=strchr(data+1, 'e');
        if(!c) break;
        int val = atoi(data+1);
        std::cout<<val;
        ret = c+1-data;
        break;
    }
    
    case 'l':
    {
        int n=0;
        std::cout<<"[";
        do
        {
			if(*(data+1+n)=='e')
			{
				++n;
				break;
			}
			if(ret>1)
			{
				std::cout<<", ";
			}
            ret = parse(level+1, data+1+n, len-1-n);			
            if(ret>0) n+=ret;
        }while(ret>0&&n+1<len);
        ret=1+n;
        std::cout<<"]";
        break;
    }
        
    case 'd':
    {
        int i=0;
        int n=0;
		if(level)
		{
			std::cout<<std::endl;
		}
        std::cout<<std::string(indent, (level)*2)<<"{"<<std::endl;
        do
        {
			++i;
			if(*(data+1+n)=='e')
			{
				std::cout<<std::endl<<std::string(indent, (level)*2)<<"}";
				++n;
				break;
			}
			if(i%2&&ret>0)
			{
				std::cout<<", "<<std::endl;
			}
            if(i%2)
			{
			    std::cout<<std::string(indent, (level+1)*2);	
			}
			else
            {
                std::cout<<": ";
            }
            ret = parse(level+1, data+1+n, len-1-n);            
            if(ret>0)
            {
                n+=ret;
            }
        }while(ret>0&&n+1<len);
        ret=1+n;
        break;
    }
        
    default:
    {
        char *c=strchr(data, ':');
        if(!isdigit(*data)||!c)break;
        int n = atoi(data);
        if(c-data+1+n>len)break;
        ret=c-data+1+n;
		bool print_as_pieces=false;
		if(n%20==0)
		{
			for(int i=0;i<n;i++)
			{
				if(!isprint(*(c+1+i)))
				{
					print_as_pieces=true;
					break;
				}
			}
		}
		if(print_as_pieces)
		{
			char buf[41];
			std::cout<<std::endl<<std::string(indent, level*2)<<"[";
			for(int i=0;i<n;i+=20)
			{
				if(i)
				{
					std::cout<<", ";
				}
				if(i%100==0)
				{
					std::cout<<std::endl<<std::string(indent, (level+1)*2);
				}
				for(int j=0; j<20; j++)
				{
					snprintf(buf+j*2, 3, "%02x", (unsigned char)*(c+1+i+j));
				}
				std::cout<<"\""<<std::string(buf,40)<<"\"";
			}
			std::cout<<std::endl<<std::string(indent, level*2)<<"]";			
		}
		else
		{
			std::string str=std::string(c+1, n);
			replace(str, "\"", "\\\"");
			std::cout<<"\""<<str<<"\"";			
		}        
        break;
    }
    }
    if(level<=0 || newline)
    {
        std::cout<<std::endl;
    }
    return ret;
}

int main(int argc, char **argv)
{
    if(argc<2)
    {
        std::cout<<"usage: bencode_parser [file]"<<std::endl;
        return 1;
    }
    const char *file=argv[1];
    int fd = open(file, O_RDONLY);
    if(fd==-1)
    {
        std::cout<<"failed to open(), file: "<<file<<", err: "<<errno<<std::endl;
        return 1;
    }
    char buffer[2*1024*1024];
    int readn = read(fd, buffer, sizeof(buffer));
    if(readn==-1)
    {
        std::cout<<"failed to read(), file: "<<file<<", err: "<<errno<<std::endl;
        return 1;
    }
    int n=0;
    int parsed=0;
    do
    {
        n=parse(0, buffer+parsed,readn-parsed);
        if(n>0)parsed+=n;
    }while(n>0);
    //std::cout<<"parse torrent file, file: "<<file<<", len: "<<readn<<", parsed: "<<parsed<<std::endl;
    return readn==parsed?0:1;
}
