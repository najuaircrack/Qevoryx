#include "transport/file_transport.hpp"
#include "common/platform.hpp"
#include <fcntl.h>
#include <cerrno>

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
    : fd_(QEVORYX_OPEN(path.c_str(),
#if QEVORYX_PLATFORM_WINDOWS
                       O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
#else
                       O_WRONLY | O_CREAT | O_TRUNC,
#endif
                       0644)) {}

FileTransport::~FileTransport() {
    close();
}

bool FileTransport::transmit(const common::PacketBuffer& packet) {
    if (fd_ < 0) return false;

    std::size_t total_written = 0;
    while (total_written < packet.size) {
        const ssize_t written = QEVORYX_WRITE(fd_,
                                                packet.ptr() + total_written,
                                                packet.size - total_written);
        if (written < 0) {
#if !QEVORYX_PLATFORM_WINDOWS
            if (errno == EINTR) {
                continue;
            }
#endif
            return false;
        }
        if (written == 0) {
            return false;
        }
        total_written += static_cast<std::size_t>(written);
    }

    return true;
}

void FileTransport::close() noexcept {
    if (fd_ >= 0) {
        QEVORYX_CLOSE(fd_);
        fd_ = -1;
    }
}

} // namespace transport
