/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <thread>
#include <vector>

#define FUSE_USE_VERSION 26
#include <fuse.h>

namespace repowerd
{
namespace test
{

using VirtualFilesystemReadHandler =
    std::function<int(char const* path, char* buf, size_t size, off_t offset)>;
using VirtualFilesystemWriteHandler =
    std::function<int(char const* path, char const* buf, size_t size, off_t offset)>;
using VirtualFilesystemIoctlHandler =
    std::function<int(char const* path, int cmd, void* arg)>;

class VirtualFilesystem
{
public:
    VirtualFilesystem();
    ~VirtualFilesystem();

    std::string mount_point();
    std::string full_path(std::string const& path);

    void add_directory(std::string const& path);
    void add_file(
        std::string const& path,
        VirtualFilesystemReadHandler const& read_handler,
        VirtualFilesystemWriteHandler const& write_handler);
    void add_file_read_write_ioctl(
        std::string const& path,
        VirtualFilesystemReadHandler const& read_handler,
        VirtualFilesystemWriteHandler const& write_handler,
        VirtualFilesystemIoctlHandler const& ioctl_handler);
    void add_file_with_contents(
        std::string const& path,
        std::string const& contents);
    std::shared_ptr<std::vector<std::string>> add_file_with_live_contents(
        std::string const& path);

private:
    static int vfs_getattr(char const* path, struct stat* stbuf);
    static int vfs_readdir(
        char const* path,
        void* buf,
        fuse_fill_dir_t filler,
        off_t offset,
        struct fuse_file_info* fi);
    static int vfs_open(const char *path, struct fuse_file_info* fi);
    static int vfs_read(
        char const* path,
        char* buf,
        size_t size,
        off_t offset,
        struct fuse_file_info* fi);
    static int vfs_write(
        char const* path,
        char const* buf,
        size_t size,
        off_t offset,
        struct fuse_file_info* fi);
    static int vfs_ioctl(
        char const* path,
        int cmd,
        void* arg,
        struct fuse_file_info* fi,
        unsigned int flags,
        void* data);

    int getattr(char const* path, struct stat* stbuf);
    int readdir(
        char const* path,
        void* buf,
        fuse_fill_dir_t filler,
        off_t offset,
        struct fuse_file_info* fi);
    int open(const char *path, struct fuse_file_info* fi);
    int read(
        char const* path,
        char* buf,
        size_t size,
        off_t offset,
        struct fuse_file_info* fi);
    int write(
        char const* path,
        char const* buf,
        size_t size,
        off_t offset,
        struct fuse_file_info* fi);
    int ioctl(
        char const* path,
        int cmd,
        void* arg,
        struct fuse_file_info* fi,
        unsigned int flags,
        void* data);

    std::string const mount_point_;
    std::unique_ptr<fuse,void(*)(fuse*)> fuse_handle;
    std::thread vfs_thread;

    struct FileHandlers
    {
        VirtualFilesystemReadHandler read_handler;
        VirtualFilesystemWriteHandler write_handler;
        VirtualFilesystemIoctlHandler ioctl_handler;
    };

    std::unordered_map<std::string,std::unordered_set<std::string>> directories;
    std::unordered_map<std::string,FileHandlers> files;
};

}
}
