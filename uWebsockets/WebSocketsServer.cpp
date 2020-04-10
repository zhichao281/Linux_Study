#include "WebSocketsServer.h"

#include "uWebsockets/src/Hub.h"
#include <openssl/sha.h>
#include <iostream>




CWebSocketsServer::CWebSocketsServer()
{
	m_extraHeaders = {};
	m_event = NULL;
	m_timerid = -1;
}


CWebSocketsServer::~CWebSocketsServer()
{
	StopTimeEvent();
	Stop();
}
void CWebSocketsServer::Start(int nPort)
{
	m_bRun.store(true);
	m_nPort = nPort;
	std::thread jthread([=]()
	{
		while (m_bRun.load())
		{		
			uWS::Hub hub;
			m_pus = &hub;
			hub.onConnection([&](uWS::WebSocket<uWS::SERVER> *ws, uWS::HttpRequest req)
			{
				std::string  strReq=req.getUrl().toString();
				if (m_event)
				{
					m_event->onConnection();
				}

			});
			hub.onError([&](int port)
			{
				if (m_event)
				{
					m_event->onError(port);
				}
			});
			hub.onError([&](void *user)
			{
				std::cout << "FAILURE: Connection failed! Timeout?" << std::endl;
				if (m_event)
				{
					m_event->onError(user);
				}
			});
			hub.onPing([&](uWS::WebSocket<uWS::SERVER>* ws, char* message, size_t length)
			{
				std::string strmsg(message, message + length);
				std::cout << "onPing" << std::endl;
				std::cout << strmsg << std::endl;
			}); 
			hub.onPong([&](uWS::WebSocket<uWS::SERVER>* ws, char* message, size_t length)
			{
					std::string strmsg(message, message + length);
					std::cout << "onPong" << std::endl;
					std::cout << strmsg << std::endl;
			});
			hub.onDisconnection([&](uWS::WebSocket<uWS::SERVER> *ws, int code, char *message, size_t length)
			{		
				if (m_event)
				{
					m_event->onDisconnection(code);
				}
			});

			hub.onMessage([&](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode)
			{			
				std::string strmsg(message, message + length);
				if (m_event)
				{
			
					m_event->OnMessage(strmsg, this);
				}
				std::cout << strmsg << std::endl;
			});
			if (hub.listen(m_nPort))
			{
				hub.run();
			}
			else
			{
				std::cout << "Failed to listen" << std::endl;
			}
		}
	});
	jthread.detach();
}

void CWebSocketsServer::sendTextMessage(std::string text, OpCode opCode)
{
	m_asyncMutex.lock();
	if (m_pus)
	{
		((uWS::Hub*)m_pus)->getDefaultGroup<uWS::SERVER>().broadcast(text.c_str(), text.length(), uWS::OpCode(opCode));
	}
	m_asyncMutex.unlock();

}

void CWebSocketsServer::Stop()
{
	m_asyncMutex.lock();
	m_bRun.store(false);
	if (m_pus)
	{
		((uWS::Hub*)m_pus)->getDefaultGroup<uWS::SERVER>().close();
		delete m_pus;
		m_pus = NULL;
	}
	m_asyncMutex.unlock();
}

void CWebSocketsServer::SetEvent(UWebSocketsServerEvent * event)
{
	m_event = event;
}


void setTimer()
{
	
}


void CWebSocketsServer::StartTimeEvent(int nMiliseconds)
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
	std::thread pongThread([=]()
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

	pongThread.detach();


#endif
}
void CWebSocketsServer::StopTimeEvent()
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
void CWebSocketsServer::onTimeEvent()
{
	m_asyncMutex.lock();
	if (m_pus)
	{
		if (m_event)
		{
			m_event->onTimeEvent();
		}
		((uWS::Hub*)m_pus)->getDefaultGroup<uWS::SERVER>().broadcast(0, 0, uWS::OpCode::PONG);
	}
	m_asyncMutex.unlock();
}
