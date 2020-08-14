#include <Encryption/WriteBufferFromFileProvider.h>

namespace DB
{
namespace ErrorCodes
{
extern const int CANNOT_WRITE_TO_FILE_DESCRIPTOR;
extern const int CANNOT_FSYNC;
extern const int CANNOT_SEEK_THROUGH_FILE;
extern const int CANNOT_TRUNCATE_FILE;
} // namespace ErrorCodes

void WriteBufferFromFileProvider::close() { file->close(); }

WriteBufferFromFileProvider::WriteBufferFromFileProvider(const FileProviderPtr & file_provider_,
    const std::string & file_name_,
    const EncryptionPath & encryption_path_,
    bool create_new_encryption_info_,
    size_t buf_size,
    int flags,
    mode_t mode,
    char * existing_memory,
    size_t alignment)
    : WriteBufferFromFileDescriptor(-1, buf_size, existing_memory, alignment),
      file(file_provider_->newWritableFile(file_name_, encryption_path_, true, create_new_encryption_info_, flags, mode))
{
    fd = file->getFd();
}

void WriteBufferFromFileProvider::nextImpl()
{
    if (!offset())
        return;

    size_t bytes_written = 0;
    while (bytes_written != offset())
    {

        ssize_t res = 0;
        {
            res = file->write(working_buffer.begin() + bytes_written, offset() - bytes_written);
        }

        if ((-1 == res || 0 == res) && errno != EINTR)
        {
            throwFromErrno("Cannot write to file " + getFileName(), ErrorCodes::CANNOT_WRITE_TO_FILE_DESCRIPTOR);
        }

        if (res > 0)
            bytes_written += res;
    }
}

WriteBufferFromFileProvider::~WriteBufferFromFileProvider()
{
    if (file->isClosed())
        return;

    try
    {
        next();
    }
    catch (...)
    {
        tryLogCurrentException(__PRETTY_FUNCTION__);
    }

    file->close();
}

} // namespace DB