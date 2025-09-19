#include "defines.h"
#include "commandHandler.h"
#include "logger.h"
#include "util.h"
#include <algorithm>
#include <cstring>

namespace CommandHandler {
	using namespace SbbLog;
	using namespace Util;
	using namespace ModuleBase;
	using namespace MemoryCommands;

	/**
	 * @brief Handles a command by name and parameters, dispatching to the appropriate handler.
	 * @param The command name.
	 * @param The command parameters.
	 * @return The result buffer.
	 */
	std::vector<char> Handler::HandleCommand(const std::string& cmd, const std::vector<std::string>& params) {
		std::vector<char> buffer;
		if (cmd.empty()) {
			Logger::instance().log("HandleCommand() cmd empty.");
			return buffer;
		}

		std::string log = "HandleCommand cmd: " + cmd;
		if (!params.empty()) {
			log += ". Parameters: ";
			for (size_t i = 0; i < params.size(); ++i) {
				log += "[" + std::to_string(i) + "]: " + params[i];
				if (i < params.size() - 1) {
					log += ", ";
				}
			}
        }

        Logger::instance().log(log);
		u64 pid = 0;
		Result rc = pmdmntGetApplicationProcessId(&pid);
		if (R_SUCCEEDED(rc) && (m_metaData.pid == 0 || m_metaData.pid != pid)) {
			m_metaData.pid = pid;
			initMetaData();
        }

		auto it = Handler::m_cmd.find(cmd);
		if (it != Handler::m_cmd.end()) {
			it->second(params, buffer);
		} else {
			Logger::instance().log("HandleCommand() cmd not found (" + cmd + ").");
		}

		return buffer;
	}

#pragma region Vision
	/**
	 * @brief Handle the "peek" command.
	 * @param [offset, size].
	 * @param Output buffer for result.
	 */
	void Handler::peek_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() != 2) {
			return;
		}

		u64 offset = Utils::parseStringToInt(params.front());
		u64 size = Utils::parseStringToInt(params[1]);
		peek(m_metaData.heap_base + offset, size, buffer);
	}

	/**
	 * @brief Handle the "peekMulti" command.
	 * @param [offset1, size1, offset2, size2, ...].
	 * @param Output buffer for result.
	 */
	void Handler::peekMulti_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() < 2) {
			return;
		}

		u64 itemCount = (params.size()) / 2;
		auto offsets = std::vector<u64>(itemCount);
		auto sizes = std::vector<u64>(itemCount);
		for (u64 i = 0; i < itemCount; ++i) {
			offsets[i] = m_metaData.heap_base + Utils::parseStringToInt(params[(i * 2)]);
			sizes[i] = Utils::parseStringToInt(params[(i * 2) + 1]);
		}

		peekMulti(offsets, sizes, buffer);
	}

	/**
	 * @brief Handle the "peekAbsolute" command.
	 * @param [offset, size].
	 * @param Output buffer for result.
	 */
	void Handler::peekAbsolute_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() != 2) {
			return;
		}

		u64 offset = Utils::parseStringToInt(params[0]);
		u64 size = Utils::parseStringToInt(params[1]);
		peek(offset, size, buffer);
	}

	/**
	 * @brief Handle the "peekAbsoluteMulti" command.
	 * @param [offset1, size1, offset2, size2, ...].
	 * @param Output buffer for result.
	 */
	void Handler::peekAbsoluteMulti_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() < 2) {
			return;
		}

		u64 itemCount = (params.size()) / 2;
		auto offsets = std::vector<u64>(itemCount);
		auto sizes = std::vector<u64>(itemCount);
		for (u64 i = 0; i < itemCount; ++i) {
			offsets[i] = Utils::parseStringToInt(params[(i * 2)]);
			sizes[i] = Utils::parseStringToInt(params[(i * 2) + 1]);
		}

		peekMulti(offsets, sizes, buffer);
	}

	/**
	 * @brief Handle the "peekMain" command.
	 * @param [offset, size].
	 * @param Output buffer for result.
	 */
	void Handler::peekMain_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() != 2) {
			return;
		}

		u64 offset = Utils::parseStringToInt(params[0]);
		u64 size = Utils::parseStringToInt(params[1]);
		peek(m_metaData.main_nso_base + offset, size, buffer);
	}

	/**
	 * @brief Handle the "peekMainMulti" command.
	 * @param [offset1, size1, offset2, size2, ...].
	 * @param Output buffer for result.
	 */
	void Handler::peekMainMulti_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() < 2) {
			return;
		}

		size_t itemCount = (params.size()) / 2;
		auto offsets = std::vector<u64>(itemCount);
		auto sizes = std::vector<u64>(itemCount);
		for (size_t i = 0; i < itemCount; i++) {
			offsets[i] = m_metaData.main_nso_base + Utils::parseStringToInt(params[(i * 2)]);
			sizes[i] = Utils::parseStringToInt(params[(i * 2) + 1]);
		}

		peekMulti(offsets, sizes, buffer);
	}

	/**
	 * @brief Handle the "poke" command.
	 * @param [offset, data].
	 */
	void Handler::poke_cmd(const std::vector<std::string>& params) {
		if (params.size() != 2) {
			return;
		}

		u64 offset = Utils::parseStringToInt(params[0]);
		std::vector<char> buffer = Utils::parseStringToByteBuffer(params[1]);
		poke(m_metaData.heap_base + offset, buffer.size(), buffer);
	}

	/**
	 * @brief Handle the "pokeAbsolute" command.
	 * @param [offset, data].
	 */
	void Handler::pokeAbsolute_cmd(const std::vector<std::string>& params) {
		if (params.size() != 2) {
			return;
		}

		u64 offset = Utils::parseStringToInt(params[0]);
		std::vector<char> buffer = Utils::parseStringToByteBuffer(params[1]);
		poke(offset, buffer.size(), buffer);
	}

	/**
	 * @brief Handle the "pokeMain" command.
	 * @param [offset, data].
	 */
	void Handler::pokeMain_cmd(const std::vector<std::string>& params) {
		if (params.size() != 2) {
			return;
		}

		u64 offset = Utils::parseStringToInt(params[0]);
		std::vector<char> buffer = Utils::parseStringToByteBuffer(params[1]);
		poke(m_metaData.main_nso_base + offset, buffer.size(), buffer);
	}

	/**
	 * @brief Handle the "pointerAll" command.
	 * @param [mainJump, jump1, jump2, ..., finalJump].
	 * @param Output buffer for result.
	 */
	void Handler::pointerAll_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() < 2) {
			return;
		}

		std::vector<std::string> mod = params;
		s64 finalJump = Utils::parseStringToSignedLong(mod.back());
		mod.pop_back();

		s64 mainJump = Utils::parseStringToSignedLong(mod.front());
		mod.erase(mod.begin());

		int count = mod.size();
		std::vector<s64> jumps(count);
		for (int i = 0; i < count; i++) {
			jumps[i] = Utils::parseStringToSignedLong(mod[i]);
		}

		u64 val = followMainPointer(mainJump, jumps, buffer);
		if (val != 0) {
			val += finalJump;
			std::memcpy(buffer.data(), &val, sizeof(val));
		} else {
			Logger::instance().log("pointerAll_cmd() val is 0, not adding final jump.");
		}

		if (g_enableBackwardsCompat && !Utils::isUSB()) {
			Utils::hexify(buffer);
		}
	}

	/**
	 * @brief Handle the "pointerRelative" command.
	 * @param [mainJump, jump1, jump2, ..., finalJump].
	 * @param Output buffer for result.
	 */
	void Handler::pointerRelative_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() < 2) {
			return;
		}

		std::vector<std::string> mod = params;
		s64 finalJump = Utils::parseStringToSignedLong(mod.back());
		mod.pop_back();

		s64 mainJump = Utils::parseStringToSignedLong(mod.front());
		mod.erase(mod.begin());

		int count = mod.size();
		std::vector<s64> jumps(count);
		for (int i = 0; i < count; i++) {
			jumps[i] = Utils::parseStringToSignedLong(mod[i]);
		}

		u64 val = followMainPointer(mainJump, jumps, buffer);
		if (val != 0) {
			val += finalJump;
			val -= m_metaData.heap_base;
			std::memcpy(buffer.data(), &val, sizeof(val));
		} else {
			Logger::instance().log("pointerRelative_cmd() val is 0, not adding final jump.");
		}

		if (g_enableBackwardsCompat && !Utils::isUSB()) {
			Utils::hexify(buffer);
		}
	}

	/**
	 * @brief Handle the "pointerPeek" command.
	 * @param [size, mainJump, jump1, ..., finalJump].
	 * @param Output buffer for result.
	 */
	void Handler::pointerPeek_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() < 3) {
			return;
		}

		std::vector<std::string> mod = params;
		s64 finalJump = Utils::parseStringToSignedLong(mod.back());
		mod.pop_back();

		u64 size = Utils::parseStringToSignedLong(mod.front());
		mod.erase(mod.begin());

		s64 mainJump = Utils::parseStringToSignedLong(mod.front());
		mod.erase(mod.begin());

		int count = mod.size();
		std::vector<s64> jumps(count);
		for (int i = 0; i < count; i++) {
			jumps[i] = Utils::parseStringToSignedLong(mod[i]);
		}

		u64 val = followMainPointer(mainJump, jumps, buffer);
		val += finalJump;
		std::memcpy(buffer.data(), &val, sizeof(val));
		peek(val, size, buffer);
	}

	/**
	 * @brief Handle the "pointerPeekMulti" command.
	 * @param Multiple pointer expressions separated by "*".
	 * @param Output buffer for result.
	 */
	void Handler::pointerPeekMulti_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() < 3) {
			return;
		}

		std::vector<u64> offsets;
		std::vector<u64> sizes;
		std::vector<std::vector<std::string>> groups;
		std::vector<std::string> currentGroup;

		for (const auto& param : params) {
			if (param == "*") {
				if (!currentGroup.empty()) {
					groups.push_back(currentGroup);
					currentGroup.clear();
				}
			} else {
				currentGroup.push_back(param);
			}
		}

		if (!currentGroup.empty()) {
			groups.push_back(currentGroup);
		}

		for (const auto& group : groups) {
			if (group.size() < 4) {
				continue;
			}

			std::vector<std::string> mod = group;
			s64 finalJump = Utils::parseStringToSignedLong(mod.back());
			mod.pop_back();

			s64 size = Utils::parseStringToSignedLong(mod.front());
			mod.erase(mod.begin());

			s64 mainJump = Utils::parseStringToSignedLong(mod.front());
			mod.erase(mod.begin());

			int count = mod.size();
			std::vector<s64> jumps(count);
			for (int i = 0; i < count; i++) {
				jumps[i] = Utils::parseStringToSignedLong(mod[i]);
			}

			u64 val = followMainPointer(mainJump, jumps, buffer);
			val += finalJump;

			offsets.push_back(val);
			sizes.push_back(size);
		}

		if (!offsets.empty() && !sizes.empty()) {
			peekMulti(offsets, sizes, buffer);
		}
	}

	/**
	 * @brief Handle the "pointerPoke" command.
	 * @param Command parameters: [data, mainJump, jump1, ..., finalJump].
	 */
	void Handler::pointerPoke_cmd(const std::vector<std::string>& params) {
		if (params.size() < 3) {
			return;
		}

		std::vector<std::string> mod = params;
		s64 finalJump = Utils::parseStringToSignedLong(mod.back());
		mod.pop_back();

		std::vector<char> data = Utils::parseStringToByteBuffer(mod.front());
		mod.erase(mod.begin());

		s64 mainJump = Utils::parseStringToSignedLong(mod.front());
		mod.erase(mod.begin());

		int count = mod.size();
		std::vector<s64> jumps(count);
		for (int i = 0; i < count; i++) {
			jumps[i] = Utils::parseStringToSignedLong(mod[i]);
		}

		std::vector<char> buffer;
		u64 val = followMainPointer(mainJump, jumps, buffer);
		val += finalJump;
		poke(val, data.size(), data);
	}
#pragma endregion Various memory read/write commands.
#pragma region Controller
	/**
	 * @brief Handle the "click" command.
	 * @param Command parameters: [buttonName].
	 */
	void Handler::click_cmd(const std::vector<std::string>& params) {
		if (params.size() != 1) {
			return;
		}

		click((HidNpadButton)parseStringToButton(params.front()));
	}

	/**
	 * @brief Handle the "press" command.
	 * @param Command parameters: [buttonName].
	 */
	void Handler::press_cmd(const std::vector<std::string>& params) {
		if (params.size() != 1) {
			return;
		}

		HidNpadButton key = (HidNpadButton)parseStringToButton(params.front());
		press(key);
	}

	/**
	 * @brief Handle the "release" command.
	 * @param Command parameters: [buttonName].
	 */
	void Handler::release_cmd(const std::vector<std::string>& params) {
		if (params.size() != 1) {
			return;
		}

		HidNpadButton key = (HidNpadButton)parseStringToButton(params.front());
		release(key);
	}

	/**
	 * @brief Handle the "setStick" command.
	 * @param Command parameters: [stickName, dx, dy].
	 */
	void Handler::setStick_cmd(const std::vector<std::string>& params) {
		if (params.size() != 3) {
			return;
		}

		int stick = parseStringToStick(params.front());
		if (stick == -1) {
			return;
		}

		int dxVal = std::stoull(params[1], NULL, 0);
		if (dxVal > JOYSTICK_MAX) {
			dxVal = JOYSTICK_MAX;
		} else if (dxVal < JOYSTICK_MIN) {
			dxVal = JOYSTICK_MIN;
		}


		int dyVal = std::stoull(params[2], NULL, 0);
		if (dyVal > JOYSTICK_MAX) {
			dyVal = JOYSTICK_MAX;
		} else if (dyVal < JOYSTICK_MIN) {
			dyVal = JOYSTICK_MIN;
		}

		setStickState((Joystick)stick, dxVal, dyVal);
	}

	/**
	 * @brief Handle the "touch" command.
	 * @param Command parameters: [x1, y1, x2, y2, ...].
	 */
	void Handler::touch_cmd(const std::vector<std::string>& params) {
		if (params.size() < 2) {
			return;
		}

		u32 count = params.size() / 2;
		std::vector<HidTouchState> state(count);
		u32 j = 0;
		for (u32 i = 0; i < count; i++) {
			state[i].diameter_x = state[i].diameter_y = m_fingerDiameter;
			state[i].x = (u32)Utils::parseStringToInt(params[j++]);
			state[i].y = (u32)Utils::parseStringToInt(params[j++]);
		}

		touch(state, count, m_pollRate * 1e+6L, false);
	}

	/**
	 * @brief Handle the "touchHold" command.
	 * @param Command parameters: [x, y, timeMs].
	 */
	void Handler::touchHold_cmd(const std::vector<std::string>& params) {
		if (params.size() < 3) {
			return;
		}

		std::vector<HidTouchState> state(1);
		state[0].diameter_x = state[0].diameter_y = m_fingerDiameter;
		state[0].x = (u32)Utils::parseStringToInt(params[0]);
		state[0].y = (u32)Utils::parseStringToInt(params[1]);
		u64 time = Utils::parseStringToInt(params[2]);
		touch(state, 1, time * 1e+6L, false);
	}

	/**
	 * @brief Handle the "touchDraw" command.
	 * @param Command parameters: [x1, y1, x2, y2, ...].
	 */
	void Handler::touchDraw_cmd(const std::vector<std::string>& params) {
		if (params.size() < 2) {
			return;
		}

		u32 count = params.size() / 2;
		std::vector<HidTouchState> state(count);
		u32 j = 0;
		for (u32 i = 0; i < count; i++) {
			state[i].diameter_x = state[i].diameter_y = m_fingerDiameter;
			state[i].x = (u32)Utils::parseStringToInt(params[j++]);
			state[i].y = (u32)Utils::parseStringToInt(params[j++]);
		}

		touch(state, count, m_pollRate * 1e+6L * 2, true);
	}

	/**
	 * @brief Handle the "key" command.
	 * @param [key1, key2, ...].
	 */
	void Handler::key_cmd(const std::vector<std::string>& params) {
		if (params.size() < 1) {
			return;
		}

		u64 count = params.size();
		std::vector<HiddbgKeyboardAutoPilotState> keystates(count);
		for (u64 i = 0; i < count; i++) {
			u8 key = (u8)Utils::parseStringToInt(params[i]);
			if (key >= HidKeyboardKey_A && key <= HidKeyboardKey_RightGui) {
				keystates[i].keys[key / 64] = 1UL << key;
				keystates[i].modifiers = 1024UL; //numlock
			}
		}

		key(keystates, count);
	}

	/**
	 * @brief Handle the "keyMod" command.
	 * @param [key1, mod1, key2, mod2, ...].
	 */
	void Handler::keyMod_cmd(const std::vector<std::string>& params) {
		if (params.size() < 2) {
			return;
		}

		u64 count = params.size() / 2;
		std::vector<HiddbgKeyboardAutoPilotState> keystates(count);
		int j = 0;
		for (u64 i = 0; i < count; i++) {
			u8 key = (u8)Utils::parseStringToInt(params[j++]);
			if (key >= HidKeyboardKey_A && key <= HidKeyboardKey_RightGui) {
				keystates[i].keys[key / 64] = 1UL << key;
				keystates[i].modifiers = BIT((u8)Utils::parseStringToInt(params[j++]));
			}
		}

		key(keystates, count);
	}

	/**
	 * @brief Handle the "keyMulti" command.
	 * @param [key1, key2, ...].
	 */
	void Handler::keyMulti_cmd(const std::vector<std::string>& params) {
		if (params.size() < 1) {
			return;
		}

		u64 count = params.size();
		std::vector<HiddbgKeyboardAutoPilotState> keystates(count);
		for (u64 i = 0; i < count; i++) {
			u8 key = (u8)Utils::parseStringToInt(params[i]);
			if (key >= HidKeyboardKey_A && key <= HidKeyboardKey_RightGui) {
				keystates[0].keys[key / 64] |= 1UL << key;
			}
		}

		key(keystates, count);
	}

	/**
	 * @brief Handle the "detachController" command.
	 */
	void Handler::detachController_cmd() {
		detachController();
	}

	/**
	 * @brief Returns whether the controller thread is running.
	 * @return True if running, false otherwise.
	 */
	bool Handler::getIsRunningPA() {
		return m_ccThreadRunning.load();
	}
#pragma endregion Various controller commands.
#pragma region Base
	/**
	 * @brief Handle the "game" command.
	 * @param [subcommand] (name, author, rating, version, icon).
	 * @param Output buffer for result.
	 */
	void Handler::game_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() != 1) {
			return;
		}

		auto it = BaseCommands::m_game.find(params.front());
		if (it != BaseCommands::m_game.end()) {
			it->second(buffer);
		} else {
			Logger::instance().log("game_cmd() subcommand not found.");
		}
	}

	/**
	 * @brief Handle the "getTitleID" command.
	 * @param Output buffer for result.
	 */
	void Handler::getTitleID_cmd(std::vector<char>& buffer) {
		initMetaData();
		buffer.resize(sizeof(m_metaData.titleID));
		std::copy(reinterpret_cast<const char*>(&m_metaData.titleID),
			reinterpret_cast<const char*>(&m_metaData.titleID) + sizeof(m_metaData.titleID),
			buffer.begin());

		if (g_enableBackwardsCompat && !Utils::isUSB()) {
			Utils::hexifyString(buffer);
		}
	}

	/**
	 * @brief Handle the "getBuildID" command.
	 * @param Output buffer for result.
	 */
	void Handler::getBuildID_cmd(std::vector<char>& buffer) {
		initMetaData();
		buffer.resize(sizeof(m_metaData.buildID));
		std::copy(reinterpret_cast<const char*>(&m_metaData.buildID),
			reinterpret_cast<const char*>(&m_metaData.buildID) + sizeof(m_metaData.buildID),
			buffer.begin());

		if (g_enableBackwardsCompat && !Utils::isUSB()) {
			Utils::hexifyString(buffer);
		}
	}

	/**
	 * @brief Handle the "getTitleVersion" command.
	 * @param Output buffer for result.
	 */
	void Handler::getTitleVersion_cmd(std::vector<char>& buffer) {
		initMetaData();
		buffer.resize(sizeof(m_metaData.titleVersion));
		std::copy(reinterpret_cast<const char*>(&m_metaData.titleVersion),
			reinterpret_cast<const char*>(&m_metaData.titleVersion) + sizeof(m_metaData.titleVersion),
			buffer.begin());

		if (g_enableBackwardsCompat && !Utils::isUSB()) {
			Utils::hexifyString(buffer);
		}
	}

	/**
	 * @brief Handle the "getSystemLanguage" command.
	 * @param Output buffer for result.
	 */
	void Handler::getSystemLanguage_cmd(std::vector<char>& buffer) {
		setInitialize();
		u64 languageCode = 0;
		SetLanguage language = SetLanguage_ENUS;
		setGetSystemLanguage(&languageCode);
		setMakeLanguage(languageCode, &language);
		setExit();

		buffer.resize(sizeof(language));
		std::copy(reinterpret_cast<const char*>(&language),
			reinterpret_cast<const char*>(&language) + sizeof(language),
			buffer.begin());

		if (g_enableBackwardsCompat && !Utils::isUSB()) {
			Utils::hexifyString(buffer);
		}
	}

	/**
	 * @brief Handle the "isProgramRunning" command.
	 * @param [programID].
	 * @param Output buffer for result.
	 */
	void Handler::isProgramRunning_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() != 1) {
			return;
		}

		u64 programID = Utils::parseStringToInt(params.front());
		bool isRunning = getIsProgramOpen(programID);

		buffer.resize(sizeof(isRunning));
		std::copy(reinterpret_cast<const char*>(&isRunning),
			reinterpret_cast<const char*>(&isRunning) + sizeof(isRunning),
			buffer.begin());

		if (g_enableBackwardsCompat && !Utils::isUSB()) {
			Utils::hexifyString(buffer);
		}
	}

	/**
	 * @brief Handle the "pixelPeek" command.
	 * @param Output buffer for result.
	 */
	void Handler::pixelPeek_cmd(std::vector<char>& buffer) {
		try {
			u64 outSize = 0;
			buffer.resize(0x80000);

			Result rc = capsscCaptureJpegScreenShot(&outSize, (void*)buffer.data(), buffer.size(), ViLayerStack_Screenshot, 1e+9L);
			if (R_FAILED(rc)) {
				Logger::instance().log("Failed to capture screenshot.", std::to_string(R_DESCRIPTION(rc)));
			}

			buffer.resize(outSize);
			if (g_enableBackwardsCompat && !Utils::isUSB()) {
				Utils::hexify(buffer);
            }
		} catch (const std::bad_alloc& e) {
			Logger::instance().log("std::bad_alloc caught in pixelPeek_cmd().", std::string(e.what()));
			throw;
		}
	}

	/**
	 * @brief Handle the "screenOn" command.
	 */
	void Handler::screenOn_cmd() {
		setScreen(ViPowerState_On);
	}

	/**
	 * @brief Handle the "screenOff" command.
	 */
	void Handler::screenOff_cmd() {
		setScreen(ViPowerState_Off);
	}

	/**
	 * @brief Handle the "getMainNsoBase" command.
	 * @param Output buffer for result.
	 */
	void Handler::getMainNsoBase_cmd(std::vector<char>& buffer) {
		initMetaData();
		buffer.resize(sizeof(m_metaData.main_nso_base));
		std::copy(reinterpret_cast<const char*>(&m_metaData.main_nso_base),
			reinterpret_cast<const char*>(&m_metaData.main_nso_base) + sizeof(m_metaData.main_nso_base),
			buffer.begin());

		if (g_enableBackwardsCompat && !Utils::isUSB()) {
			Utils::hexifyString(buffer);
		}
	}

	/**
	 * @brief Handle the "getHeapBase" command.
	 * @param Output buffer for result.
	 */
	void Handler::getHeapBase_cmd(std::vector<char>& buffer) {
		initMetaData();
		buffer.resize(sizeof(m_metaData.heap_base));
		std::copy(reinterpret_cast<const char*>(&m_metaData.heap_base),
			reinterpret_cast<const char*>(&m_metaData.heap_base) + sizeof(m_metaData.heap_base),
			buffer.begin());

		if (g_enableBackwardsCompat && !Utils::isUSB()) {
			Utils::hexifyString(buffer);
		}
	}

	/**
	 * @brief Handle the "charge" command.
	 * @param Output buffer for result.
	 */
	void Handler::charge_cmd(std::vector<char>& buffer) {
		Result rc = psmInitialize();
		if (R_FAILED(rc)) {
			Logger::instance().log("charge_cmd() psmInitialize() failed.", std::to_string(R_DESCRIPTION(rc)));
			return;
		}

		u32 charge;
		rc = psmGetBatteryChargePercentage(&charge);
		psmExit();
		if (R_FAILED(rc)) {
			Logger::instance().log("charge_cmd() psmGetBatteryChargePercentage() failed.", std::to_string(R_DESCRIPTION(rc)));
			return;
		}

		buffer.resize(sizeof(charge));
		std::copy(reinterpret_cast<const char*>(&charge),
			reinterpret_cast<const char*>(&charge) + sizeof(charge),
			buffer.begin());

		if (g_enableBackwardsCompat && !Utils::isUSB()) {
			Utils::hexifyString(buffer);
		}
	}
#pragma endregion Various base libnx commands.
#pragma region Misc
	/**
	 * @brief Handle the "getVersion" command.
	 * @param Output buffer for result.
	 */
	void Handler::getVersion_cmd(std::vector<char>& buffer) {
		auto sbb = getSbbVersion();
		buffer.insert(buffer.begin(), sbb.begin(), sbb.end());
	}

	/**
	 * @brief Handle the "configure" command.
	 * @param [name, value].
	 */
	void Handler::configure_cmd(const std::vector<std::string>& params) {
		if (params.size() != 2) {
			return;
		}

		if (params.front() == "controllerType") {
			setControllerType(params);
			return;
		}

		auto it = BaseCommands::m_configure.find(params.front());
		if (it != BaseCommands::m_configure.end()) {
			it->second(params);
		} else {
			Logger::instance().log("configure_cmd() subfunction not found.");
		}
	}

	/**
	 * @brief Returns whether PA controller commands are enabled.
	 * @return True if enabled, false otherwise.
	 */
	bool Handler::getIsEnabledPA() {
		return BaseCommands::getIsEnabledPA();
	}

	/**
	 * @brief Handle the "ping" command.
	 * @param [value].
	 * @param Output buffer for result.
	 */
	void Handler::ping_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() != 1) {
			return;
		}

		std::string value;
		try {
			value = std::to_string(Utils::parseStringToInt(params[0]));
		} catch (...) {
			Logger::instance().log("ping_cmd() failed to parse value.");
			value = std::to_string(0);
		}

		buffer.insert(buffer.begin(), value.begin(), value.end());
	}
#pragma endregion Miscellaneous commands that get/set parameters.
#pragma region Time
	/**
	 * @brief Handle the "getSwitchTime" command.
	 * @param Output buffer for result.
	 */
	void Handler::getSwitchTime_cmd(std::vector<char>& buffer) {
		getSwitchTime(buffer);
	}

	/**
	 * @brief Handle the "setSwitchTime" command.
	 * @param [time].
	 * @param Output buffer for result.
	 */
	void Handler::setSwitchTime_cmd(const std::vector<std::string>& params, std::vector<char>& buffer) {
		if (params.size() != 1) {
			return;
		}

		setSwitchTime(params, buffer);
	}

	/**
	 * @brief Handle the "resetSwitchTime" command.
	 * @param Output buffer for result.
	 */
	void Handler::resetSwitchTime_cmd(std::vector<char>& buffer) {
		resetSwitchTime(buffer);
	}
#pragma endregion Time commands.
}
