#pragma once

#include <string>
#include <boost/json.hpp>
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

// uid???? say uid rn 1x1 go uidless go skeet shoutbox rn i roll go
namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using boost::json::value;
using boost::json::object;
using json = nlohmann::json;

namespace Minecraft
{
	class CMinecraftServer
	{
	public:
		CMinecraftServer(std::string szIp, uint16_t iPort = 25565)
			: m_szIp(std::move(szIp)),
			m_iPort(iPort)
		{
			Ping();
		}

		void Ping()
		{
			m_bOnline = false;
			m_szLastError.clear();

			try
			{
				asio::io_context IOContext;
				tcp::socket Socket(IOContext);
				tcp::resolver Resolver(IOContext);

				auto Endpoints = Resolver.resolve(m_szIp, std::to_string(m_iPort));

				Connect(IOContext, Socket, Endpoints, std::chrono::seconds(2));
				SendHandshakeAndStatusRequest(Socket);
				m_szRawJson = ReceiveStatusResponse(IOContext, Socket, std::chrono::seconds(2));
				m_vecPlayers = ParseOnlinePlayers(m_szRawJson);
				m_bOnline = true;
			}
			catch (const std::exception& Exception)
			{
				m_szLastError = Exception.what();
				m_bOnline = false;
			}
		}

	public:
		std::string m_szRawJson;
		std::vector<std::string> m_vecPlayers;
		std::string m_szLastError;
		bool m_bOnline = false;

	private:
		static constexpr uint32_t kMaxStatusPacketBytes = 1u * 1024u * 1024u; // 1 MiB

		template <typename Endpoints>
		void Connect(asio::io_context& IOContext, tcp::socket& Socket, const Endpoints& Endpoints_,
			std::chrono::steady_clock::duration Timeout)
		{
			boost::system::error_code ConnectError = asio::error::would_block;

			asio::async_connect(Socket, Endpoints_,
				[&](const boost::system::error_code& ec, const tcp::endpoint&)
				{
					ConnectError = ec;
				});

			RunWithTimeout(IOContext, Socket, Timeout, ConnectError);

			if (ConnectError)
				throw boost::system::system_error(ConnectError, "connect");
		}

		void RunWithTimeout(asio::io_context& IOContext, tcp::socket& Socket,
			std::chrono::steady_clock::duration Timeout, boost::system::error_code& WatchedError)
		{
			asio::steady_timer Timer(IOContext);
			Timer.expires_after(Timeout);
			Timer.async_wait([&](const boost::system::error_code& ec)
				{
					if (!ec)
					{
						boost::system::error_code Ignored;
						Socket.close(Ignored);
					}
				});

			IOContext.restart();
			while (IOContext.run_one())
			{
				if (WatchedError != asio::error::would_block)
				{
					Timer.cancel();
					break;
				}
			}

			if (WatchedError == asio::error::would_block)
				WatchedError = asio::error::timed_out;
		}

		void SendHandshakeAndStatusRequest(tcp::socket& Socket)
		{
			std::vector<uint8_t> buffer;

			AppendPacket(buffer, 0x00, BuildHandshakePayload());
			AppendPacket(buffer, 0x00, {});

			asio::write(Socket, asio::buffer(buffer));
		}

		std::vector<uint8_t> BuildHandshakePayload() const
		{
			std::vector<uint8_t> payload;

			auto ver = EncodeVarInt(static_cast<uint32_t>(-1));
			payload.insert(payload.end(), ver.begin(), ver.end());

			auto addr = std::vector<uint8_t>(m_szIp.begin(), m_szIp.end());
			auto addrLen = EncodeVarInt(static_cast<uint32_t>(addr.size()));
			payload.insert(payload.end(), addrLen.begin(), addrLen.end());
			payload.insert(payload.end(), addr.begin(), addr.end());

			payload.push_back(static_cast<uint8_t>((m_iPort >> 8) & 0xFF));
			payload.push_back(static_cast<uint8_t>(m_iPort & 0xFF));

			auto state = EncodeVarInt(1);
			payload.insert(payload.end(), state.begin(), state.end());

			return payload;
		}

		static void AppendPacket(std::vector<uint8_t>& buffer, uint8_t iPacketId,
			const std::vector<uint8_t>& vecPayload)
		{
			std::vector<uint8_t> packet;
			auto idBytes = EncodeVarInt(iPacketId);
			packet.insert(packet.end(), idBytes.begin(), idBytes.end());
			packet.insert(packet.end(), vecPayload.begin(), vecPayload.end());

			auto lenBytes = EncodeVarInt(static_cast<uint32_t>(packet.size()));
			buffer.insert(buffer.end(), lenBytes.begin(), lenBytes.end());
			buffer.insert(buffer.end(), packet.begin(), packet.end());
		}

		std::string ReceiveStatusResponse(asio::io_context& IOContext, tcp::socket& Socket,
			std::chrono::steady_clock::duration Timeout)
		{
			std::vector<uint8_t> buffer(4096);
			size_t iTotalRead = 0;

			auto fnReadMore = [&](size_t iMinBytes) -> void
				{
					while (iTotalRead < iMinBytes)
					{
						if (iTotalRead == buffer.size())
							buffer.resize(buffer.size() * 2);

						boost::system::error_code ReadError = asio::error::would_block;
						size_t iBytesRead = 0;

						Socket.async_read_some(asio::buffer(buffer.data() + iTotalRead, buffer.size() - iTotalRead),
							[&](const boost::system::error_code& ec, size_t iBytes)
							{
								ReadError = ec;
								iBytesRead = iBytes;
							});

						RunWithTimeout(IOContext, Socket, Timeout, ReadError);

						if (ReadError && ReadError != asio::error::eof)
							throw boost::system::system_error(ReadError, "read");

						if (iBytesRead == 0)
							throw std::runtime_error("connection closed before status response was complete");

						iTotalRead += iBytesRead;
					}
				};

			fnReadMore(15);

			size_t iIndex = 0;
			uint32_t iPacketLen = DecodeVarInt(buffer, iIndex);
			uint32_t iPacketId = DecodeVarInt(buffer, iIndex);

			if (iPacketId != 0x00)
				throw std::runtime_error("unexpected packet id for status response");

			uint32_t iStringLen = DecodeVarInt(buffer, iIndex);

			if (iStringLen == 0 || iStringLen > kMaxStatusPacketBytes)
				throw std::runtime_error("status JSON length is zero or implausibly large");

			size_t iStringEnd = iIndex + iStringLen;
			fnReadMore(iStringEnd);

			return std::string(buffer.begin() + iIndex, buffer.begin() + iStringEnd);
		}

		static std::vector<std::string> ParseOnlinePlayers(const std::string& szJson)
		{
			auto fnIsGoodPlayerName = [](const std::string& szName) -> bool
				{
					for (unsigned char c : szName)
						if (!std::isalnum(c) && c != '_')
							return false;

					return !szName.empty();
				};

			std::vector<std::string> vecNames;
			json Data = json::parse(szJson, nullptr, /*allow_exceptions=*/false);

			if (Data.is_discarded())
				throw std::runtime_error("status response was not valid JSON");

			if (Data.contains("players") && Data["players"].contains("sample"))
			{
				for (const auto& Player : Data["players"]["sample"])
				{
					if (!Player.contains("name") || !Player["name"].is_string())
						continue;

					std::string szName = Player["name"].get<std::string>();
					if (fnIsGoodPlayerName(szName))
						vecNames.push_back(std::move(szName));
				}
			}

			return vecNames;
		}

		static std::vector<uint8_t> EncodeVarInt(uint32_t iValue)
		{
			std::vector<uint8_t> out;
			do
			{
				uint8_t iByte = iValue & 0x7F;
				iValue >>= 7;
				if (iValue != 0)
					iByte |= 0x80;
				out.push_back(iByte);
			} while (iValue != 0);

			return out;
		}

		static uint32_t DecodeVarInt(const std::vector<uint8_t>& buffer, size_t& iIndex)
		{
			uint32_t iValue = 0;

			for (uint8_t iShift = 0; iShift < 35; iShift += 7)
			{
				if (iIndex >= buffer.size())
					throw std::runtime_error("varint ran past end of buffer");

				uint8_t iByte = buffer[iIndex++];
				iValue |= static_cast<uint32_t>(iByte & 0x7F) << iShift;

				if ((iByte & 0x80) == 0)
					return iValue;
			}

			throw std::runtime_error("varint is too long / malformed");
		}

	private:
		std::string m_szIp;
		uint16_t m_iPort;
	};
}