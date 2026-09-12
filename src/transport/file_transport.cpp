#include "transport/file_transport.hpp"
#include "common/platform.hpp"
#include <fcntl.h>

#if QEVORYX_PLATFORM_WINDOWS
#include <io.h>
#define open _open
#define close _close
#define write _write
#define ssize_t int
#else
#include <unistd.h>
#endif

namespace transport {

FileTransport::FileTransport(const std::string& path)
    : fd_(open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644)) {}

FileTransport::~FileTransport() {
    close();
}

bool FileTransport::transmit(const common::PacketBuffer& packet) {
    if (fd_ < 0) return false;
    ssize_t written = ::write(fd_, packet.ptr(), packet.size);
    return written == static_cast<ssize_t>(packet.size);
}

void FileTransport::close() noexcept {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

} // namespace transport
