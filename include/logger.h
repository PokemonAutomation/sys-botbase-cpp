#pragma once

#include "defines.h"
#include "lockFreeQueue.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <chrono>
#include <string>
#include <switch.h>
#include <thread>
#include <atomic>
#include <mutex>

namespace SbbLog {
    using namespace LocklessQueue;

	class Logger {
	public:
		static Logger& instance() {
			static Logger loggerInstance;
			return loggerInstance;
		}

		void enableLogs(bool enable) {
            m_logsEnabled.store(enable, std::memory_order_release);
			if (enable) {
				log("Logging enabled.");
			} else {
				log("Logging disabled.");
			}
		}

		bool isLoggingEnabled() {
            return m_logsEnabled.load(std::memory_order_acquire);
		}

		void log(const std::string& message, const std::string& error = "", bool override = false) {
			if (!isLoggingEnabled() && error.empty() && !override) {
				return;
			}

			m_queue.push(std::move(LogMessage(std::move(message), std::move(error), getCurrentTimestamp())));
            m_cv.notify_one();
		}
	private:
		Logger() : m_queue(), m_running(false), m_logsEnabled(false) {
			m_thread = std::thread(&Logger::threadLoop, this);
		};

		~Logger() {
			m_running.store(false, std::memory_order_release);
			if (m_thread.joinable()) {
				m_thread.join();
			}
		};

		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;

		struct LogMessage {
			LogMessage(const std::string& msg, const std::string& err, const uint64_t& ts)
                : message(std::move(msg)), error(std::move(err)), timestamp(ts) {}
			
			LogMessage() = default;

			std::string message;
			std::string error;
			uint64_t timestamp;
		};

        size_t m_maxLogSize = 1024 * 1024 * 8;
		LockFreeQueue<LogMessage, 1024> m_queue;
		std::atomic_bool m_running { false };
		std::atomic_bool m_logsEnabled { false };
        std::thread m_thread;
        std::mutex m_mutex;
		std::condition_variable m_cv;
	private:
		size_t getFileSize(const std::string& filename) {
			struct stat stat_buf;
			int rc = stat(filename.c_str(), &stat_buf);
			return rc == 0 ? stat_buf.st_size : 0;
		}

		uint64_t getCurrentTimestamp() {
			u64 now_sec = 0;
			Result res = timeGetCurrentTime(TimeType_UserSystemClock, &now_sec);
			if (R_FAILED(res)) {
				log("Failed to get current time", std::to_string(R_DESCRIPTION(res)));
				now_sec = static_cast<u64>(std::time(nullptr));
			}

			auto now = std::chrono::system_clock::now();
			auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;
			return now_sec * 1000000 + now_us;
		}

		void threadLoop() {
			try {
				const std::string filename = "sdmc:/atmosphere/contents/430000000000000B/log.txt";
				m_running.store(true, std::memory_order_release);

				std::unique_lock<std::mutex> lock(m_mutex);
				while (m_running.load(std::memory_order_acquire)) {
					m_cv.wait(lock, [this] { return !m_queue.empty() || !m_running.load(std::memory_order_acquire); });
					if (!m_running.load(std::memory_order_acquire)) {
						break;
					}

					if (getFileSize(filename) >= m_maxLogSize) {
						std::ofstream clear(filename, std::ios::trunc);
						clear.close();
					}

					LogMessage message;
					std::ofstream logFile(filename, std::ios::app);
					while (m_queue.pop(message)) {
						if (logFile.is_open()) {
							time_t seconds = static_cast<time_t>(message.timestamp / 1000000);
							uint32_t microseconds = static_cast<uint32_t>(message.timestamp % 1000000);

							time_t now_time_t = static_cast<std::time_t>(seconds);
							tm* localTime = std::localtime(&now_time_t);

							std::ostringstream oss;
							oss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
							oss << "." << std::setfill('0') << std::setw(6) << microseconds;

							std::string msg = "[" + oss.str() + "] " + message.message;
							if (!message.error.empty()) {
								msg += " Error: " + message.error;
							}

							logFile << msg << std::endl;
						}
					}

					logFile.close();
				}
			} catch (...) {
				return;
			}
		}
	};
}
