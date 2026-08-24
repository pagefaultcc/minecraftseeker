#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using json = nlohmann::json;

namespace Minecraft
{
    class CMinecraftServer : public std::enable_shared_from_this<CMinecraftServer>
    {
    public:
        using FnCallback = std::function<void(std::shared_ptr<CMinecraftServer>)>;

        static std::shared_ptr<CMinecraftServer> Create(std::string szIp, uint16_t iPort = 25565)
        {
            return std::shared_ptr<CMinecraftServer>(new CMinecraftServer(std::move(szIp), iPort));
        }

        void Ping(FnCallback fnCallback,
                  std::chrono::steady_clock::duration Timeout = std::chrono::seconds(2))
        {
            m_fnCallback = std::move(fnCallback);
            m_bOnline = false;
            m_szLastError.clear();
            m_bCompleted = false;

            auto Self = shared_from_this();
            ArmTimeout(Timeout);

            m_Resolver.async_resolve(
                m_szIp,
                std::to_string(m_iPort),
                [this, Self](const boost::system::error_code& ec,
                             tcp::resolver::results_type Endpoints)
                {
                    if (ec)
                        return Fail(ec.message());

                    asio::async_connect(
                        m_Socket,
                        Endpoints,
                        [this, Self](const boost::system::error_code& ec,
                                     const tcp::endpoint&)
                        {
                            if (ec)
                                return Fail(ec.message());

                            SendHandshakeAndStatusRequest();
                        });
                });
        }

        void Run() { m_IOContext.run(); }

    public:
        std::string m_szRawJson;
        std::vector<std::string> m_vecPlayers;
        std::string m_szLastError;
        bool m_bOnline = false;
        std::string m_szIp;
        uint16_t m_iPort = 25565;

    private:
        explicit CMinecraftServer(std::string szIp, uint16_t iPort)
            : m_IOContext(),
              m_Resolver(m_IOContext),
              m_Socket(m_IOContext),
              m_Timer(m_IOContext),
              m_szIp(std::move(szIp)),
              m_iPort(iPort)
        {}

        static constexpr uint32_t kMaxStatusPacketBytes = 1u * 1024u * 1024u;

        std::atomic<bool> m_bCompleted{false};

        void ArmTimeout(std::chrono::steady_clock::duration Timeout)
        {
            auto Self = shared_from_this();
            m_Timer.expires_after(Timeout);
            m_Timer.async_wait([this, Self](const boost::system::error_code& ec)
            {
                if (!ec)
                {
                    boost::system::error_code Ignored;
                    m_Resolver.cancel();
                    m_Socket.close(Ignored);
                }
            });
        }

        void Fail(const std::string& szError)
        {
            bool expected = false;
            if (!m_bCompleted.compare_exchange_strong(expected, true))
                return;

            m_Timer.cancel();
            boost::system::error_code Ignored;
            m_Resolver.cancel();
            m_Socket.close(Ignored);

            m_bOnline = false;
            m_szLastError = szError;

            if (m_fnCallback)
                m_fnCallback(shared_from_this());
        }

        void Succeed()
        {
            bool expected = false;
            if (!m_bCompleted.compare_exchange_strong(expected, true))
                return;

            m_Timer.cancel();
            boost::system::error_code Ignored;
            m_Resolver.cancel();
            m_Socket.close(Ignored);

            m_bOnline = true;

            if (m_fnCallback)
                m_fnCallback(shared_from_this());
        }

        void SendHandshakeAndStatusRequest()
        {
            auto pBuffer = std::make_shared<std::vector<uint8_t>>();
            AppendPacket(*pBuffer, 0x00, BuildHandshakePayload());
            AppendPacket(*pBuffer, 0x00, {});

            auto Self = shared_from_this();
            asio::async_write(m_Socket, asio::buffer(*pBuffer),
                [this, Self, pBuffer](const boost::system::error_code& ec, size_t)
                {
                    if (ec)
                        return Fail(ec.message());

                    m_RecvBuffer.assign(4096, 0);
                    m_iTotalRead = 0;
                    ReadMore(15, [this, Self]() { OnHeaderRead(); });
                });
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

        void ReadMore(size_t iMinBytes, std::function<void()> fnContinuation)
        {
            if (m_iTotalRead >= iMinBytes)
                return fnContinuation();

            if (m_iTotalRead == m_RecvBuffer.size())
                m_RecvBuffer.resize(m_RecvBuffer.size() * 2);

            auto Self = shared_from_this();
            m_Socket.async_read_some(
                asio::buffer(m_RecvBuffer.data() + m_iTotalRead,
                             m_RecvBuffer.size() - m_iTotalRead),
                [this, Self, iMinBytes, fnContinuation](const boost::system::error_code& ec, size_t iBytes)
                {
                    if (ec && ec != asio::error::eof)
                        return Fail(ec.message());

                    if (iBytes == 0)
                        return Fail("connection closed before status response was complete");

                    m_iTotalRead += iBytes;
                    ReadMore(iMinBytes, fnContinuation);
                });
        }

        void OnHeaderRead()
        {
            try
            {
                size_t iIndex = 0;
                DecodeVarInt(m_RecvBuffer, iIndex);
                uint32_t iPacketId = DecodeVarInt(m_RecvBuffer, iIndex);

                if (iPacketId != 0x00)
                    return Fail("unexpected packet id for status response");

                uint32_t iStringLen = DecodeVarInt(m_RecvBuffer, iIndex);

                if (iStringLen == 0 || iStringLen > kMaxStatusPacketBytes)
                    return Fail("status JSON length is zero or implausibly large");

                size_t iStringEnd = iIndex + iStringLen;
                auto Self = shared_from_this();
                ReadMore(iStringEnd, [this, Self, iIndex, iStringEnd]()
                {
                    OnBodyRead(iIndex, iStringEnd);
                });
            }
            catch (const std::exception& ex)
            {
                Fail(ex.what());
            }
        }

        void OnBodyRead(size_t iIndex, size_t iStringEnd)
        {
            m_szRawJson.assign(m_RecvBuffer.begin() + iIndex,
                               m_RecvBuffer.begin() + iStringEnd);

            try
            {
                m_vecPlayers = ParseOnlinePlayers(m_szRawJson);
            }
            catch (const std::exception& ex)
            {
                return Fail(ex.what());
            }

            Succeed();
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
            json Data = json::parse(szJson, nullptr, false);

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
        asio::io_context m_IOContext;
        tcp::resolver m_Resolver;
        tcp::socket m_Socket;
        asio::steady_timer m_Timer;

        std::vector<uint8_t> m_RecvBuffer;
        size_t m_iTotalRead = 0;

        FnCallback m_fnCallback;
    };
}