#pragma once

#include "transport/packet_transport.hpp"
#include <string>

namespace transport {

class FileTransport final : public PacketTransport {
public:
    explicit FileTransport(const std::string& path);
    ~FileTransport() override;

    bool transmit(const common::PacketBuffer& packet) override;
    void close() noexcept override;

private:
    int fd_;
};

} // namespace transport
