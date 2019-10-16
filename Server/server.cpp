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

mutex userSocketsMutex;

set<SOCKET> userSockets;

void recvWorker(SOCKET clientSocket);

void listenWorker(SOCKET listenSocket)
{
	while (true)
	{
		// Ŭ���̾�Ʈ �� ���� ���� �� ������ ���� ����ü ���� �� �� �Ҵ�
		SOCKADDR_IN tCIntAddr = {};
		int iCIntSize = sizeof(tCIntAddr);
		SOCKET hClient = accept(listenSocket, (SOCKADDR*)& tCIntAddr, &iCIntSize); // ���� ��û�� �� �� ���� �����

		
		userSocketsMutex.lock(); // ���� ���ϵ��� �߰��ϱ����� �ٸ������� ��������� üũ
		userSockets.insert(hClient);
		userSocketsMutex.unlock();

		string msg = "# Server Connection \n # Your ID : " + to_string(hClient);
		send(hClient, msg.c_str(), msg.length(), 0); // Ŭ���̾�Ʈ�� �޽��� ����

		thread t(recvWorker, ref(hClient)); // Ŭ���� �޽����� �޴� ������ ����
		t.detach();

		cout << "# " << hClient << " Client Connection" << endl;
	}
}


void recvWorker(SOCKET clientSocket)
{
	while (true)
	{
		char cBuffer[PACKET_SIZE] = {};
		int code = recv(clientSocket, cBuffer, PACKET_SIZE, 0); //�������κ��� �� ������ �޾��ִ� ����. ������ �����Ͱ� ������ ���
		if (code == 0 || code == -1) //������ ����ƴ��� üũ
		{
			cout << "# " << clientSocket << " Client Disconnection" << endl;
			userSockets.erase(clientSocket);
			break;
		}

		cout << "User Message [" << clientSocket <<"]: " << cBuffer << endl;

		string msg = to_string(clientSocket) + " : " + cBuffer;

		int len = msg.length();
		// ������ ����� ��� Ŭ�� �޽��� ����
		userSocketsMutex.lock();
		for (auto sock : userSockets)
		{
			send(sock, msg.c_str(), len, 0);
		}
		userSocketsMutex.unlock();
	}
}


int main()
{
	/*
		���� ���� ����
		socket() -> bind() -> listen() -> accept() -> [���ӵǸ�] -> read() -> write() -> close

		Ŭ�� ���ۼ���
		socket() -> connect() -> write() -> read() -> close()
	*/

	WSADATA wsaData; // ������ �ʱ�ȭ ������ ������ ����ü
	WSAStartup(MAKEWORD(2, 2), &wsaData); // ����

	// �ٸ� ��ǻ�ͷκ��� ������ ���� ���� ��û�� �������ִ� ����
	SOCKET listenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //���� ����

	// ������ ������� (��Ʈ, IP��)�� ������ ����ü
	SOCKADDR_IN listenAddr = {};
	listenAddr.sin_family = AF_INET;
	listenAddr.sin_port = htons(PORT); //htons: host to network short. �򿣵�� ���
	listenAddr.sin_addr.s_addr = htonl(INADDR_ANY);  //INADDR_ANY: ���� ���۵Ǵ� ��ǻ�� IP�ּ�
	
	// ���Ͽ� ���� ������ ������ �����ְ�, ������ ���� ��� ���·� ����
	bind(listenSocket, (SOCKADDR*)&listenAddr, sizeof(listenAddr)); //���Ͽ� �ּ������� ����
	listen(listenSocket, SOMAXCONN);// ���Ͽ� ���ӽ����� ����.

	thread listenThread(listenWorker, ref(listenSocket));

	// thread ������ ���� ��� 
	listenThread.join();
	
	cout << "# Server Close" << endl;

	closesocket(listenSocket);

	WSACleanup(); // ��

	system("pause");
	return 0;
}