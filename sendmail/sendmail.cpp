#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
using namespace std;

class SmtpMail
{
private:
	char SmtpSrvName[32];
	char Port[8];
	char UserName[32];
	char Password[32];
	char From[32];
	char To[32];
	char Subject[32];
	char Msg[64];
	void Base64(const char *Data, char *chuue);
	int Talk(SOCKET sockid, const char *OkCode, char *pSend);
public:
	SmtpMail(const char* s, const char* p, const char* u, const char* w,
		const char* f, const char* t, const char* j, const char* m)
	{
		strcpy(SmtpSrvName, s);
		strcpy(Port, p);
		strcpy(UserName, u);
		strcpy(Password, w);
		strcpy(From, f);
		strcpy(To, t);
		strcpy(Subject, j);
		strcpy(Msg, m);
	}
	int SendMail();
};
//---------------------------------------------------------------------
int SmtpMail::SendMail()
{
	const int buflen = 256;
	char buf[buflen];
	//(1)初始化网络环境
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
	{
		printf("WSAStartup() error : %d\n", GetLastError());
		return 1;
	}
	//(2)创建套接字
	SOCKET sockid;
	if ((sockid = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("socket() error : %d\n", GetLastError());
		WSACleanup();
		return 1;
	}
	//(3)得到SMTP服务器IP
	struct hostent *phostent = gethostbyname(SmtpSrvName);
	struct sockaddr_in addr;
	CopyMemory(&addr.sin_addr.S_un.S_addr,
		phostent->h_addr_list[0],
		sizeof(addr.sin_addr.S_un.S_addr));
	struct in_addr srvaddr;
	CopyMemory(&srvaddr, &addr.sin_addr.S_un.S_addr, sizeof(struct in_addr));
	printf("Smtp server name is %s\n", SmtpSrvName);
	printf("Smtp server ip is %s\n", inet_ntoa(srvaddr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(Port));
	ZeroMemory(&addr.sin_zero, 8);
	//(4)连接服务器
	if (connect(sockid, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		printf("connect() error : %d\n", GetLastError());
		goto STOP;
	}
	//(5)按照SMTP收发信息
	if (Talk(sockid, "220", "HELO asdf"))//192.168.1.2"))
	{
		goto STOP;
	}
	if (Talk(sockid, "250", "AUTH LOGIN"))
	{
		goto STOP;
	}
	ZeroMemory(buf, buflen);
	Base64(UserName, (char *)buf);
	if (Talk(sockid, "334", buf))
	{
		goto STOP;
	}
	ZeroMemory(buf, buflen);
	Base64(Password, buf);
	if (Talk(sockid, "334", buf))
	{
		goto STOP;
	}
	ZeroMemory(buf, buflen);
	sprintf_s(buf, "MAIL FROM:<%s>", From);
	if (Talk(sockid, "235", buf))
	{
		goto STOP;
	}
	ZeroMemory(buf, buflen);
	sprintf_s(buf, "RCPT TO:<%s>", To);
	if (Talk(sockid, "250", buf))
	{
		goto STOP;
	}
	if (Talk(sockid, "250", "DATA"))
	{
		goto STOP;
	}
	ZeroMemory(buf, buflen);
	sprintf_s(buf, "TO: %s\r\nFROM: %s\r\nSUBJECT: %s\r\n%s\r\n\r\n.",
		To, From, Subject, Msg);
	if (Talk(sockid, "354", buf))
	{
		goto STOP;
	}
	if (Talk(sockid, "250", "QUIT"))
	{
		goto STOP;
	}
	if (Talk(sockid, "221", ""))
	{
		goto STOP;
	}
	else
	{
		closesocket(sockid);
		WSACleanup();
		return 0;
	}
STOP://(6)关闭socket，释放网络资源
	closesocket(sockid);
	WSACleanup();
	return 1;
}
//---------------------------------------------------------------------
int SmtpMail::Talk(SOCKET sockid, const char *OkCode, char *pSend)
{
	const int buflen = 256;
	char buf[buflen];
	ZeroMemory(buf, buflen);
	//接收返回信息
	if (recv(sockid, buf, buflen, 0) == SOCKET_ERROR)
	{
		printf("recv() error : %d\n", GetLastError());
		return 1;
	}
	else
		printf("%s\n", buf);
	if (strstr(buf, OkCode) == NULL)
	{
		printf("Error: recv code != %s\n", OkCode);
	}
	//发送命令
	if (strlen(pSend))
	{
		ZeroMemory(buf, buflen);
		sprintf_s(buf, "%s\r\n", pSend);
		if (send(sockid, buf, strlen(buf), 0) == SOCKET_ERROR)
		{
			printf("send() error : %d\n", GetLastError());
			return 1;
		}
	}
	return 0;
}
//Base64编码，Data：未编码的二进制代码，chuue：编码过的Base64代码
void SmtpMail::Base64(const char* Data,  char *chuue)
{
	//编码表
	const char EncodeTable[] = 
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	string strEncode;
	int DataByte = strlen((char*)Data);
	unsigned char Tmp[4] = { 0 };
	int LineLength = 0;
	for (int i = 0; i<(int)(DataByte / 3); i++)
	{
		Tmp[1] = *Data++;
		Tmp[2] = *Data++;
		Tmp[3] = *Data++;
		strEncode += EncodeTable[Tmp[1] >> 2];
		strEncode += EncodeTable[((Tmp[1] << 4) | (Tmp[2] >> 4)) & 0x3F];
		strEncode += EncodeTable[((Tmp[2] << 2) | (Tmp[3] >> 6)) & 0x3F];
		strEncode += EncodeTable[Tmp[3] & 0x3F];
		if (LineLength += 4, LineLength == 76) 
{ strEncode += "\r\n"; LineLength = 0; }
	}
	//对剩余数据进行编码
	int Mod = DataByte % 3;
	if (Mod == 1)
	{
		Tmp[1] = *Data++;
		strEncode += EncodeTable[(Tmp[1] & 0xFC) >> 2];
		strEncode += EncodeTable[((Tmp[1] & 0x03) << 4)];
		strEncode += "==";
	}
	else if (Mod == 2)
	{
		Tmp[1] = *Data++;
		Tmp[2] = *Data++;
		strEncode += EncodeTable[(Tmp[1] & 0xFC) >> 2];
		strEncode += EncodeTable[((Tmp[1] & 0x03) << 4) | ((Tmp[2] & 0xF0) >> 4)];
		strEncode += EncodeTable[((Tmp[2] & 0x0F) << 2)];
		strEncode += "=";
	}

	strcpy(chuue, strEncode.c_str());
}

void main ()
{//  SmtpMail mail("mail.bupt.cn", "25", "abc123@bupt.cn", "password", 
//                        "abc123@bupt.cn", "xxxx@hotmail.com", "111", "111");
   SmtpMail mail("smtp.163.com", "25", "abc123", "password", "abc123@163.com", "abc456@163.com", "hello,小李", "来信收到，谢谢！");
      mail.SendMail();
}
