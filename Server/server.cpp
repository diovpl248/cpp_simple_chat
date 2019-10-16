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
		// 클라이언트 측 소켓 생성 및 정보를 담을 구조체 생성 및 값 할당
		SOCKADDR_IN tCIntAddr = {};
		int iCIntSize = sizeof(tCIntAddr);
		SOCKET hClient = accept(listenSocket, (SOCKADDR*)& tCIntAddr, &iCIntSize); // 접속 요청이 올 때 까지 대기함

		
		userSocketsMutex.lock(); // 유저 소켓들을 추가하기전에 다른곳에서 사용중인지 체크
		userSockets.insert(hClient);
		userSocketsMutex.unlock();

		string msg = "# Server Connection \n # Your ID : " + to_string(hClient);
		send(hClient, msg.c_str(), msg.length(), 0); // 클라이언트에 메시지 전송

		thread t(recvWorker, ref(hClient)); // 클라의 메시지를 받는 쓰레드 생성
		t.detach();

		cout << "# " << hClient << " Client Connection" << endl;
	}
}


void recvWorker(SOCKET clientSocket)
{
	while (true)
	{
		char cBuffer[PACKET_SIZE] = {};
		int code = recv(clientSocket, cBuffer, PACKET_SIZE, 0); //소켓으로부터 온 정보를 받아주는 역할. 보내준 데이터가 없으면 대기
		if (code == 0 || code == -1) //접속이 종료됐는지 체크
		{
			cout << "# " << clientSocket << " Client Disconnection" << endl;
			userSockets.erase(clientSocket);
			break;
		}

		cout << "User Message [" << clientSocket <<"]: " << cBuffer << endl;

		string msg = to_string(clientSocket) + " : " + cBuffer;

		int len = msg.length();
		// 서버에 연결된 모든 클라에 메시지 보냄
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
		서버 동작 순서
		socket() -> bind() -> listen() -> accept() -> [접속되면] -> read() -> write() -> close

		클라 동작순서
		socket() -> connect() -> write() -> read() -> close()
	*/

	WSADATA wsaData; // 소켓의 초기화 정보를 저장할 구조체
	WSAStartup(MAKEWORD(2, 2), &wsaData); // 시작

	// 다른 컴퓨터로부터 들어오는 접속 승인 요청을 수락해주는 소켓
	SOCKET listenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //소켓 생성

	// 소켓의 구성요소 (포트, IP등)을 지정할 구조체
	SOCKADDR_IN listenAddr = {};
	listenAddr.sin_family = AF_INET;
	listenAddr.sin_port = htons(PORT); //htons: host to network short. 빅엔디안 방식
	listenAddr.sin_addr.s_addr = htonl(INADDR_ANY);  //INADDR_ANY: 현재 동작되는 컴퓨터 IP주소
	
	// 소켓에 위에 설정한 정보를 묶어주고, 소켓을 접속 대기 상태로 만듬
	bind(listenSocket, (SOCKADDR*)&listenAddr, sizeof(listenAddr)); //소켓에 주소정보를 연결
	listen(listenSocket, SOMAXCONN);// 소켓에 접속승인을 해줌.

	thread listenThread(listenWorker, ref(listenSocket));

	// thread 끝날때 까지 대기 
	listenThread.join();
	
	cout << "# Server Close" << endl;

	closesocket(listenSocket);

	WSACleanup(); // 끝

	system("pause");
	return 0;
}