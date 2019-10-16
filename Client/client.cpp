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
		int code = recv(serverSocket, cBuffer, PACKET_SIZE, 0); //�������κ��� �� ������ �޾��ִ� ����. ������ �����Ͱ� ������ ���
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
	WSADATA wsaData; // ������ �ʱ�ȭ ������ ������ ����ü
	WSAStartup(MAKEWORD(2, 2), &wsaData); // ����

	SOCKET hSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN tAddr = {};
	tAddr.sin_family = AF_INET;
	tAddr.sin_port = htons(PORT);
	tAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

	auto result = connect(hSocket, (SOCKADDR*)& tAddr, sizeof(tAddr));

	if (result == SOCKET_ERROR)
	{
		cout << "������ ������ �����߽��ϴ�." << endl;
		WSACleanup(); // ��

		system("pause");
		return 0;
	}


	thread listenThread(recvWorker, ref(hSocket));
	thread sendThread(sendWorker, ref(hSocket));
	sendThread.detach();

	listenThread.join();

	closesocket(hSocket);

	WSACleanup(); // ��

	system("pause");
	return 0;
}