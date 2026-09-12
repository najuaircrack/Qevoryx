#include "transport/file_transport.hpp"
#include "common/platform.hpp"
#include <fcntl.h>

#if QEVORYX_PLATFORM_WINDOWS
#include <io.h>
#define QEVORYX_OPEN(path, flags, mode) _open(path, flags, mode)
#define QEVORYX_CLOSE(fd) _close(fd)
#define QEVORYX_WRITE(fd, buf, len) _write(fd, buf, len)
#define ssize_t int
#else
#include <unistd.h>
#define QEVORYX_OPEN(path, flags, mode) open(path, flags, mode)
#define QEVORYX_CLOSE(fd) ::close(fd)
#define QEVORYX_WRITE(fd, buf, len) ::write(fd, buf, len)
#endif

namespace transport {

FileTransport::FileTransport(const std::string& path)
    : fd_(QEVORYX_OPEN(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644)) {}

FileTransport::~FileTransport() {
    close();
}

bool FileTransport::transmit(const common::PacketBuffer& packet) {
    if (fd_ < 0) return false;
    ssize_t written = QEVORYX_WRITE(fd_, packet.ptr(), packet.size);
    return written == static_cast<ssize_t>(packet.size);
}

void FileTransport::close() noexcept {
    if (fd_ >= 0) {
        QEVORYX_CLOSE(fd_);
        fd_ = -1;
    }
}

} // namespace transport
