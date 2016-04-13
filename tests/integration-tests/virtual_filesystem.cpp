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

#include "virtual_filesystem.h"

#include <cstdlib>
#include <unistd.h>
#include <system_error>
#include <cstring>

namespace rt = repowerd::test;

namespace
{

rt::VirtualFilesystem* vfs()
{
    return static_cast<rt::VirtualFilesystem*>(fuse_get_context()->private_data);
}

void* vfs_init(fuse_conn_info* /*conn*/)
{
    return vfs();
}

std::string make_temp_dir()
{
    char dir_tmpl[] = "/tmp/repowerd-test-vfs-XXXXXX";
    if (!mkdtemp(dir_tmpl))
        throw std::system_error{errno, std::system_category()};
    return dir_tmpl;
}

std::pair<std::string,std::string> split_path(std::string const& path)
{
    auto last_sep = path.find_last_of('/');
    return {path.substr(0, last_sep + 1), path.substr(last_sep + 1)};
}

std::string normalize_dir(std::string const& path)
{
    if (path.back() != '/')
        return path + "/";
    else
        return path;
}

}

rt::VirtualFilesystem::VirtualFilesystem()
    : mount_point_{make_temp_dir()},
      fuse_handle{nullptr, fuse_destroy}
{
    char const* argv[] = {"test", "-odirect_io", nullptr};
    fuse_args args{2, const_cast<char**>(argv), 0};
    auto const args_raii = std::unique_ptr<fuse_args,decltype(&fuse_opt_free_args)>{
        &args, fuse_opt_free_args};

    fuse_operations fuse_ops{};
    fuse_ops.init = vfs_init;
    fuse_ops.getattr = vfs_getattr;
    fuse_ops.readdir = vfs_readdir;
    fuse_ops.open = vfs_open;
    fuse_ops.read = vfs_read;
    fuse_ops.write = vfs_write;

    auto fuse_chan_handle = fuse_mount(mount_point_.c_str(), &args);
    if (!fuse_chan_handle)
        throw std::runtime_error{"Failed to mount virtual filesystem"};

    fuse_handle.reset(
        fuse_new(fuse_chan_handle, &args, &fuse_ops, sizeof(fuse_ops), this));

    if (!fuse_handle)
    {
        fuse_unmount(mount_point_.c_str(), fuse_chan_handle);
        throw std::runtime_error{"Failed to create virtual filesystem"};
    }

    vfs_thread = std::thread{
        [this]
        {
            fuse_loop(fuse_handle.get());
        }};

    add_directory("/");
}

rt::VirtualFilesystem::~VirtualFilesystem()
{
    system(("fusermount -u " + mount_point_).c_str());
    vfs_thread.join();
    rmdir(mount_point_.c_str());
}

std::string rt::VirtualFilesystem::mount_point()
{
    return mount_point_;
}

std::string rt::VirtualFilesystem::full_path(std::string const& path)
{
    return mount_point_ + path;
}

void rt::VirtualFilesystem::add_directory(std::string const& path)
{
    auto components = split_path(path);
    if (components.second != "")
        directories[components.first].insert(components.second);
    if (directories.find(normalize_dir(path)) == directories.end())
        directories[normalize_dir(path)] = {};
}

void rt::VirtualFilesystem::add_file(
    std::string const& path,
    VirtualFilesystemReadHandler const& read_handler,
    VirtualFilesystemWriteHandler const& write_handler)
{
    auto components = split_path(path);
    directories[components.first].insert(components.second);
    files[path] = {read_handler, write_handler};
}

int rt::VirtualFilesystem::vfs_getattr(char const* path, struct stat* stbuf)
{
    return vfs()->getattr(path, stbuf);
}

int rt::VirtualFilesystem::vfs_readdir(
    char const* path,
    void* buf,
    fuse_fill_dir_t filler,
    off_t offset,
    struct fuse_file_info* fi)
{
    return vfs()->readdir(path, buf, filler, offset, fi);
}

int rt::VirtualFilesystem::vfs_open(const char *path, struct fuse_file_info *fi)
{
    return vfs()->open(path, fi);
}

int rt::VirtualFilesystem::vfs_read(
    char const* path,
    char* buf,
    size_t size,
    off_t offset,
    struct fuse_file_info* fi)
{
    return vfs()->read(path, buf, size, offset, fi);
}

int rt::VirtualFilesystem::vfs_write(
    char const* path,
    char const* buf,
    size_t size,
    off_t offset,
    struct fuse_file_info* fi)
{
    return vfs()->write(path, buf, size, offset, fi);
}

int rt::VirtualFilesystem::getattr(char const* path, struct stat* stbuf)
{
    int result = 0;

    memset(stbuf, 0, sizeof(struct stat));

    if (files.find(path) != files.end())
    {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 0;
    }
    else if (directories.find(normalize_dir(path)) != directories.end())
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 1;
    }
    else
    {
        result = -ENOENT;
    }

    return result;
}

int rt::VirtualFilesystem::readdir(
    char const* path_cstr,
    void* buf,
    fuse_fill_dir_t filler,
    off_t  offset,
    struct fuse_file_info* /*fi*/)
{
    auto const path = normalize_dir(path_cstr);

    if (offset != 0)
        return -EINVAL;

    if (directories.find(path) == directories.end())
        return -ENOENT;

    filler(buf, ".", nullptr, 0);
    filler(buf, "..", nullptr, 0);

    for (auto const& entry : directories[path])
    {
        struct stat stbuf;
        getattr((path + entry).c_str(), &stbuf);
        filler(buf, entry.c_str(), &stbuf, 0);
    }
    return 0;
}

int rt::VirtualFilesystem::open(char const* path, struct fuse_file_info* /*fi*/)
{
    if (files.find(path) == files.end())
        return -ENOENT;

    return 0;
}

int rt::VirtualFilesystem::read(
    char const* path,
    char* buf,
    size_t  size,
    off_t  offset,
    struct fuse_file_info* /*fi*/)
{
    if (files.find(path) == files.end())
        return -ENOENT;

    auto const& file_handlers = files[path];
    return file_handlers.read_handler(path, buf, size, offset);
}

int rt::VirtualFilesystem::write(
    char const* path,
    char const* buf,
    size_t  size,
    off_t  offset,
    struct fuse_file_info* /*fi*/)
{
    if (files.find(path) == files.end())
        return -ENOENT;

    auto const& file_handlers = files[path];
    return file_handlers.write_handler(path, buf, size, offset);
}
