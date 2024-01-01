#pragma once

#include <iterator>

class ClientCfg {
public:
	typedef std::size_t ClientCfgSizeType;

	ClientCfg(ClientCfgSizeType max_msg = 1024, ClientCfgSizeType timed_out = 60000,
	          ClientCfgSizeType max_username = 12)
		: m_max_msg(max_msg)
		, m_timed_out(timed_out)
		, m_max_username(max_username) {
	}

	[[nodiscard]] ClientCfgSizeType get_m_max_msg() const {
		return m_max_msg;
	}

	[[nodiscard]] ClientCfgSizeType get_m_timed_out() const {
		return m_timed_out;
	}

	[[nodiscard]] ClientCfgSizeType get_m_max_username() const {
		return m_max_username;
	}

private:
	ClientCfgSizeType m_max_msg = 1024;
	ClientCfgSizeType m_timed_out = 60000; // 1 минута
	ClientCfgSizeType m_max_username = 12;
};
