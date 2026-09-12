#pragma once

#include "transport/packet_transport.hpp"
#include <string>

namespace transport {

class FileTransport final : public PacketTransport {
public:
    explicit FileTransport(const std::string& path);
    ~FileTransport() override;

    FileTransport(const FileTransport&) = delete;
    FileTransport& operator=(const FileTransport&) = delete;
    FileTransport(FileTransport&&) = delete;
    FileTransport& operator=(FileTransport&&) = delete;

    bool transmit(const common::PacketBuffer& packet) override;
    void close() noexcept override;

private:
    int fd_;
};

} // namespace transport
