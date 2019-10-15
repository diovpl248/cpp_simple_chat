#include <WinSock2.h>
#include <iostream>
#include <set>
#include <thread>
#include <mutex>
#include <string>

#pragma comment(lib, "ws2_32")

#define PORT 1234
#define PACKET_SIZE 1024

using namespace std;

mutex listenMutex;
mutex recvMutex;

set<SOCKET> userSockets;

void recvWorker(SOCKET clientSocket);

void listenWorker(SOCKET listenSocket)
{
	while (true)
	{
		// Ŭ���̾�Ʈ �� ���� ���� �� ������ ���� ����ü ���� �� �� �Ҵ�
		SOCKADDR_IN tCIntAddr = {};
		int iCIntSize = sizeof(tCIntAddr);
		SOCKET hClient = accept(listenSocket, (SOCKADDR*)& tCIntAddr, &iCIntSize);

		listenMutex.lock();

		userSockets.insert(hClient);

		string msg = "# Server Connection \n # Your ID : " + to_string(hClient);
		send(hClient, msg.c_str(), msg.length(), 0);

		thread t(recvWorker, ref(hClient));
		t.detach();

		cout << "# " << hClient << " Client Connection" << endl;

		listenMutex.unlock();
	}
}


void recvWorker(SOCKET clientSocket)
{
	while (true)
	{
		char cBuffer[PACKET_SIZE] = {};
		int code = recv(clientSocket, cBuffer, PACKET_SIZE, 0); //�������κ��� �� ������ �޾��ִ� ����. ������ �����Ͱ� ������ ���
		if (code == 0 || code == -1)
		{
			userSockets.erase(clientSocket);
			break;
		}

		recvMutex.lock();

		cout << "User Message [" << clientSocket <<"]: " << cBuffer << endl;

		string msg = to_string(clientSocket) + " : " + cBuffer;

		int len = msg.length();
		// ������ ����� ��� Ŭ�� �޽��� ����
		for (auto sock : userSockets)
		{
			send(sock, msg.c_str(), len, 0);
		}

		recvMutex.unlock();
	}
}


int main()
{

	WSADATA wsaData; // ������ �ʱ�ȭ ������ ������ ����ü
	WSAStartup(MAKEWORD(2, 2), &wsaData); // ����

	SOCKET hListen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //���� ����

	SOCKADDR_IN tListenAddr = {};
	tListenAddr.sin_family = AF_INET;
	tListenAddr.sin_port = htons(PORT);
	tListenAddr.sin_addr.s_addr = htonl(INADDR_ANY); //��Ʈ�� �ּҵ� ����
	
	// ���Ͽ� ���� ������ ������ �����ְ�, ������ ���� ��� ���·� ����
	bind(hListen, (SOCKADDR*)& tListenAddr, sizeof(tListenAddr));
	listen(hListen, SOMAXCONN);

	thread listenThread(listenWorker, ref(hListen));

	listenThread.join();
	
	cout << "# Server Close" << endl;

	closesocket(hListen);

	WSACleanup(); // ��

	system("pause");
	return 0;
}