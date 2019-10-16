#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <string>

#pragma comment(lib, "ws2_32")

#define PORT 1234
#define PACKET_SIZE 1024
#define SERVER_IP "127.0.0.1"

using namespace std;

mutex recvMutex;

void recvWorker(SOCKET serverSocket)
{
	while (true)
	{
		char cBuffer[PACKET_SIZE] = {};
		int code = recv(serverSocket, cBuffer, PACKET_SIZE, 0); //소켓으로부터 온 정보를 받아주는 역할. 보내준 데이터가 없으면 대기
		if (code == 0 || code == -1)
		{
			break;
		}

		recvMutex.lock();

		cout << cBuffer << endl;
		int len = strlen(cBuffer);

		recvMutex.unlock();
	}
}

void sendWorker(SOCKET serverSocket)
{
	while (true)
	{
		string msg;
		getline(std::cin, msg);
		send(serverSocket, msg.c_str(), msg.length(), 0);
	}
}

int main()
{
	WSADATA wsaData; // 소켓의 초기화 정보를 저장할 구조체
	WSAStartup(MAKEWORD(2, 2), &wsaData); // 시작

	SOCKET hSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN tAddr = {};
	tAddr.sin_family = AF_INET;
	tAddr.sin_port = htons(PORT);
	tAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

	auto result = connect(hSocket, (SOCKADDR*)& tAddr, sizeof(tAddr));

	if (result == SOCKET_ERROR)
	{
		cout << "서버에 접속을 실패했습니다." << endl;
		WSACleanup(); // 끝

		system("pause");
		return 0;
	}


	thread listenThread(recvWorker, ref(hSocket));
	thread sendThread(sendWorker, ref(hSocket));
	sendThread.detach();

	listenThread.join();

	closesocket(hSocket);

	WSACleanup(); // 끝

	system("pause");
	return 0;
}