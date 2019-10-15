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
		// 클라이언트 측 소켓 생성 및 정보를 담을 구조체 생성 및 값 할당
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
		int code = recv(clientSocket, cBuffer, PACKET_SIZE, 0); //소켓으로부터 온 정보를 받아주는 역할. 보내준 데이터가 없으면 대기
		if (code == 0 || code == -1)
		{
			userSockets.erase(clientSocket);
			break;
		}

		recvMutex.lock();

		cout << "User Message [" << clientSocket <<"]: " << cBuffer << endl;

		string msg = to_string(clientSocket) + " : " + cBuffer;

		int len = msg.length();
		// 서버에 연결된 모든 클라에 메시지 보냄
		for (auto sock : userSockets)
		{
			send(sock, msg.c_str(), len, 0);
		}

		recvMutex.unlock();
	}
}


int main()
{

	WSADATA wsaData; // 소켓의 초기화 정보를 저장할 구조체
	WSAStartup(MAKEWORD(2, 2), &wsaData); // 시작

	SOCKET hListen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //소켓 생성

	SOCKADDR_IN tListenAddr = {};
	tListenAddr.sin_family = AF_INET;
	tListenAddr.sin_port = htons(PORT);
	tListenAddr.sin_addr.s_addr = htonl(INADDR_ANY); //포트와 주소등 설정
	
	// 소켓에 위에 설정한 정보를 묶어주고, 소켓을 접속 대기 상태로 만듬
	bind(hListen, (SOCKADDR*)& tListenAddr, sizeof(tListenAddr));
	listen(hListen, SOMAXCONN);

	thread listenThread(listenWorker, ref(hListen));

	listenThread.join();
	
	cout << "# Server Close" << endl;

	closesocket(hListen);

	WSACleanup(); // 끝

	system("pause");
	return 0;
}