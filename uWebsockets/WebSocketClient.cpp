#include "WebSocketClient.h"
#include "uWebsockets/src/Hub.h"
#include <openssl/sha.h>
#include <iostream>

#include <thread>
CWebSocketClient::CWebSocketClient()
{
	m_extraHeaders = {};
	m_event = NULL;
	m_timerid = -1;
	m_pus = nullptr;
}
CWebSocketClient::~CWebSocketClient()
{
	StopTimeEvent();
	Stop();	
}
void testConnections()
{
	uWS::Hub h;

	h.onError([](void *user) {
		switch ((long)user) {
		case 1:
			std::cout << "Client emitted error on invalid URI" << std::endl;
			break;
		case 2:
			std::cout << "Client emitted error on resolve failure" << std::endl;
			break;
		case 3:
			std::cout << "Client emitted error on connection timeout (non-SSL)" << std::endl;
			break;
		case 5:
			std::cout << "Client emitted error on connection timeout (SSL)" << std::endl;
			break;
		case 6:
			std::cout << "Client emitted error on HTTP response without upgrade (non-SSL)" << std::endl;
			break;
		case 7:
			std::cout << "Client emitted error on HTTP response without upgrade (SSL)" << std::endl;
			break;
		case 10:
			std::cout << "Client emitted error on poll error" << std::endl;
			break;
		case 11:
		{
			static int protocolErrorCount = 0;
			protocolErrorCount++;
			std::cout << "Client emitted error on invalid protocol" << std::endl;
			if (protocolErrorCount > 1) {
				std::cout << "FAILURE:  " << protocolErrorCount << " errors emitted for one connection!" << std::endl;
				exit(-1);
			}
			break;
		}

		default:
			std::cout << "FAILURE: " << user << " should not emit error!" << std::endl;
			exit(-1);
		}
	});

	h.onConnection([](uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req) {

	});

	h.onDisconnection([](uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, size_t length) {

	});

}

void CWebSocketClient::Start(std::string url)
{

	m_url = url;
	m_bRun.store(true);

	std::thread jthread([=]()
	{
		while (m_bRun.load())
		{
			CWebSocketClient* pthis = this;
			uWS::Hub h;
			m_pus = &h;
			UWebSocketsClientEvent* pevent = m_event;

			h.onError([&](void *user)
			{
				std::cout << "FAILURE: Connection failed! Timeout?" << std::endl;
				if (m_event)
				{
					m_event->onError(user);
				}
			});
			h.onPing([&](uWS::WebSocket<uWS::SERVER>* ws, char* message, size_t length)
				{
					std::string strmsg(message, message + length);
					std::cout << "onPing" << std::endl;
					std::cout << strmsg << std::endl;
				});
			h.onPong([&](uWS::WebSocket<uWS::SERVER>* ws, char* message, size_t length)
				{
					std::string strmsg(message, message + length);
					std::cout << "onPong" << std::endl;
					std::cout << strmsg << std::endl;
				});
			h.onMessage([&](uWS::WebSocket<uWS::CLIENT> *ws, char *message, size_t length, uWS::OpCode opCode) {
				if (m_event)
				{
					std::string strmsg(message, message + length);
					m_event->OnMessage(strmsg, pthis);
				}
			});
			int length = 0;
			h.onConnection([&h, &pthis, &pevent](uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req)
			{
				bool bSuccess = true;
				/*			switch ((long)ws->getUserData())
							{
							case 8:
								std::cout << "Client established a remote connection over non-SSL" << std::endl;
								bSuccess = false;
								ws->close(1000);
								break;
							case 9:
								std::cout << "Client established a remote connection over SSL" << std::endl;
								bSuccess = false;

								ws->close(1000);
								break;
							default:
								std::cout << "FAILURE: " << ws->getUserData() << " should not connect!" << std::endl;
								bSuccess = false;
								break;
							}*/
				if (pevent)
				{
					pevent->onConnection(bSuccess);
				}
			});
			h.onDisconnection([&h, &pthis, &pevent](uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, size_t length)
			{
				std::cout << "Client got disconnected with data: " << ws->getUserData() << ", code: " << code << ", message: <" << std::string(message, length) << ">" << std::endl;
				h.getDefaultGroup<uWS::CLIENT>().close();
				pthis->m_pus = NULL;			
				if (pevent&&code != 1000)
				{
					pevent->onDisconnection();
				}


			});
			h.connect(m_url, this, m_extraHeaders, 3000);
			h.run();
			std::cout << "Falling through testConnections" << std::endl;
	
			if (m_bRun.load())
			{
		

#ifdef _WINDOWS_
				Sleep(300);
#else
				sleep(300);//linux�µĺ���
#endif


			}
		}

	});
	if (m_bRun.load())
	{

#ifdef _WINDOWS_
		Sleep(1);
#else
		sleep(1);//linux�µĺ���
#endif

	}
	jthread.detach();
}

void CWebSocketClient::StartTimeEvent(int nMiliseconds)
{

#ifdef _WINDOWS_
	m_timerid = timeSetEvent(nMiliseconds, 1,
		[](UINT wTimerID, UINT msg, DWORD_PTR dwUser, DWORD_PTR dwl, DWORD_PTR dw2)
		{
			CWebSocketsServer* p = (CWebSocketsServer*)dwUser;
			p->onTimeEvent();
			return;
		}, (DWORD_PTR)this, TIME_PERIODIC);
#else
	m_bTime.store(true);
	std::thread pingThread([=]()
		{
			int seconds = 0;
			while (m_bTime.load())
			{
				struct timeval temp;
				temp.tv_sec = seconds;
				temp.tv_usec = nMiliseconds;
				select(0, NULL, NULL, NULL, &temp);
				onTimeEvent();
				continue;;
			}

		});

	pingThread.detach();
#endif
}
void CWebSocketClient::StopTimeEvent()
{

#ifdef _WINDOWS_
	if (m_timerid != -1)
	{
		timeKillEvent(m_timerid);
}
#else
	m_bTime.store(false);

#endif	
}
void CWebSocketClient::onTimeEvent()
{
	m_asyncMutex.lock();
	if (m_pus)
	{
		if (m_event)
		{
			m_event->onTimeEvent();
		}
		((uWS::Hub*)m_pus)->getDefaultGroup<uWS::CLIENT>().broadcast(0, 0, uWS::OpCode::PING);
	}
	m_asyncMutex.unlock();
}


void CWebSocketClient::sendTextMessage(std::string text, OpCode type)
{
	printf(text.c_str());
	printf("\r\n");
	m_asyncMutex.lock();
	if (m_pus)
	{
		((uWS::Hub*)m_pus)->getDefaultGroup<uWS::CLIENT>().broadcast(text.c_str(), text.length(), uWS::OpCode(type));
	}
	m_asyncMutex.unlock();
}
void CWebSocketClient::SetEvent(UWebSocketsClientEvent* pevent)
{
	m_event = pevent;
}
void CWebSocketClient::Stop()
{
	m_asyncMutex.lock();
	m_bRun.store(false);
#ifdef _WINDOWS_
	Sleep(300);
#else
	sleep(300);//linux�µĺ���
#endif

	if (m_pus)
	{
		((uWS::Hub*)m_pus)->getDefaultGroup<uWS::CLIENT>().close();
		m_pus = NULL;
	}
	m_asyncMutex.unlock();
}
void CWebSocketClient::SetExtraHeaders(std::map<std::string, std::string>& extraHeaders)
{
	m_extraHeaders = extraHeaders;
}