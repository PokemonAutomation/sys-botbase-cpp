#pragma once

#include "defines.h"
#include "lockFreeQueue.h"
#include "connection.h"
#include <string>
#include <vector>
#include <memory>

namespace SocketConnection {
	class SocketConnection : public Connection::ConnectionHandler {
	public:
		SocketConnection() : ConnectionHandler(), m_tcp(), m_senderQueue(), m_commandQueue() {
			m_error = false;
            m_stop = false;
			m_handler = std::make_unique<CommandHandler::Handler>();
		};

		~SocketConnection() override {
            disconnect();
            stopThreads();
		};

	public:
		Result initialize(Result& res) override;
		void initializeThreads() override;
		void stopThreads() override;
		bool connect() override;
		void run() override;
		void disconnect() override;
		int receiveData(int sockfd = 0) override;
		int sendData(const char* data, size_t data_size, int sockfd) override;

	private:
		struct TcpConnection {
			int serverFd = -1;
			int clientFd = -1;
			const int port = 6000;
		};

		TcpConnection m_tcp;

		int setupServerSocket();
		void closeSocket();
		void notifyAll() {
			m_commandCv.notify_all();
            m_senderCv.notify_all();
			if (m_handler) m_handler->cqNotifyAll();
		}

		bool getThreadsInitialized() const {
			return m_senderInitialized.load(std::memory_order_relaxed)
				   && m_commandInitialized.load(std::memory_order_relaxed);
        }

		std::string m_persistentBuffer;
		std::atomic_bool m_senderInitialized { false };
		std::atomic_bool m_commandInitialized { false };

		std::thread m_senderThread;
		LocklessQueue::LockFreeQueue<std::vector<char>> m_senderQueue;
		std::mutex m_senderMutex;
		std::condition_variable m_senderCv;

		std::thread m_commandThread;
		LocklessQueue::LockFreeQueue<std::string> m_commandQueue;
		std::mutex m_commandMutex;
		std::condition_variable m_commandCv;

		std::atomic_bool m_error { false };
		std::atomic_bool m_stop { false };
		std::unique_ptr<CommandHandler::Handler> m_handler;
	};
}
