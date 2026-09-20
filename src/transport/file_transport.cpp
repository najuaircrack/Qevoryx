#include "transport/file_transport.hpp"
#include "common/platform.hpp"
#include <fcntl.h>
#include <cerrno>

#if QEVORYX_PLATFORM_WINDOWS
#include <io.h>
#include <share.h>
#define QEVORYX_CLOSE(fd) _close(fd)
#define QEVORYX_WRITE(fd, buf, len) _write(fd, buf, len)
#define ssize_t int
#else
#include <unistd.h>
#define QEVORYX_CLOSE(fd) ::close(fd)
#define QEVORYX_WRITE(fd, buf, len) ::write(fd, buf, len)
#endif

namespace transport {

FileTransport::FileTransport(const std::string& path)
#if QEVORYX_PLATFORM_WINDOWS
    : fd_([] (const std::string& file_path) {
          int descriptor = -1;
          if (_sopen_s(&descriptor, file_path.c_str(),
                       O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, _SH_DENYNO, 0644) != 0) {
              descriptor = -1;
          }
          return descriptor;
      }(path)) {
}
#else
    : fd_(open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0644)) {
}
#endif

FileTransport::~FileTransport() {
    close();
}

bool FileTransport::transmit(const common::PacketBuffer& packet) {
    if (fd_ < 0) return false;

    std::size_t total_written = 0;
    while (total_written < packet.size) {
        const auto remaining = packet.size - total_written;
        const unsigned int write_size = static_cast<unsigned int>(remaining);
        const ssize_t written = QEVORYX_WRITE(fd_,
                                                packet.ptr() + total_written,
                                                write_size);
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
