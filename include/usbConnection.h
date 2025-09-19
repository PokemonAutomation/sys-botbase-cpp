#pragma once

#include "defines.h"
#include "lockFreeQueue.h"
#include "connection.h"
#include <string>
#include <vector>
#include <memory>

namespace UsbConnection {
	class UsbConnection : public Connection::ConnectionHandler {
	public:
		UsbConnection() : ConnectionHandler() {
			m_error = false;
			m_stop = false;
			m_handler = std::make_unique<CommandHandler::Handler>();
		};

		~UsbConnection() override {
			m_error = true;
			notifyAll();

			m_persistentBuffer.clear();
			m_senderQueue.clear();
			m_commandQueue.clear();

			if (m_senderThread.joinable()) m_senderThread.join();
			if (m_commandThread.joinable()) m_commandThread.join();
			if (m_handler) m_handler.reset();
		};

	public:
		Result initialize(Result& res) override;
        void initializeThreads() override;
        void stopThreads() override;
		bool connect() override;
		void run() override;
		void disconnect() override;
		int receiveData(int sockfd = 0) override;
		int sendData(const char* data, size_t size, int sockfd = 0) override;

	private:
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
		std::atomic_bool m_stop{ false };
		std::unique_ptr<CommandHandler::Handler> m_handler;

		struct USBResponse {
			u64 size;
			void* data;
		};
	};
}
