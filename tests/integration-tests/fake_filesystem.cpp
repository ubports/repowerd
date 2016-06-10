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

#include "fake_filesystem.h"
#include "src/adapters/fd.h"

#include <sstream>
#include <stdexcept>

namespace
{

std::vector<std::string> split_dirs(std::string const& path)
{
    std::vector<std::string> dirs;

    size_t sep = 0;
    while ((sep = path.find_first_of('/', sep)) != std::string::npos)
    {
        if (sep == 0)
            dirs.push_back("/");
        else
            dirs.push_back(path.substr(0, sep));
        ++sep;
    }

    return dirs;
}

class LiveStreamBuf : public std::streambuf
{
public:
    LiveStreamBuf(std::string& str)
        : str{str}
    {
    }

private:
    int_type overflow(int_type c) override
    {
        if (c != EOF)
            str.push_back(c);
        return c;
    }

    std::string& str;
};

class LiveOStream : public std::ostream
{
public:
    LiveOStream(std::unique_ptr<LiveStreamBuf> streambuf)
        : std::ostream{streambuf.get()},
          streambuf{std::move(streambuf)}
    {
    }

private:
    std::unique_ptr<LiveStreamBuf> const streambuf;
};

}

repowerd::test::FakeFilesystem::~FakeFilesystem()
{
    if (!paths.empty())
        throw std::runtime_error("Open files at exit");
}

bool repowerd::test::FakeFilesystem::is_regular_file(
    std::string const& path) const
{
    return files.find(path) != files.end();
}

std::unique_ptr<std::istream> repowerd::test::FakeFilesystem::istream(
    std::string const& path) const
{
    if (files.find(path) == files.end())
    {
        return std::make_unique<std::stringstream>("");
    }
    else
    {
        return std::make_unique<std::stringstream>(files.at(path)->back());
    }
}

std::unique_ptr<std::ostream> repowerd::test::FakeFilesystem::ostream(
    std::string const& path) const
{
    if (files.find(path) == files.end())
    {
        return std::make_unique<std::stringstream>("");
    }
    else
    {
        files[path]->push_back("");
        return std::make_unique<LiveOStream>(
            std::make_unique<LiveStreamBuf>(files[path]->back()));
    }
}

std::vector<std::string> repowerd::test::FakeFilesystem::subdirs(
    std::string const& path) const
{
    std::vector<std::string> return_dirs;

    for (auto const& dir : directories)
    {
        if (dir.size() > path.size() && dir.find(path) == 0 &&
            dir[path.size()] == '/' && dir.find_last_of('/') == path.size())
        {
            return_dirs.push_back(dir);
        }

    }
    return return_dirs;
}


repowerd::Fd repowerd::test::FakeFilesystem::open(
    char const* path, int) const
{
    int fd = 100;
    for (; fd < 200; ++fd)
        if (paths.find(fd) == paths.end()) break;

    paths[fd] = path;

    return {fd, [this](int fd) { paths.erase(fd); return 0; }};
}

int repowerd::test::FakeFilesystem::ioctl(
    int fd, unsigned long request, void* args) const
{
    if (paths.find(fd) == paths.end())
        return -1;

    auto const& path = paths[fd];

    if (ioctl_handlers.find(path) == ioctl_handlers.end())
        return -1;
    else
        return ioctl_handlers.at(path)(path.c_str(), request, args);
}

void repowerd::test::FakeFilesystem::add_file_with_contents(
    std::string const& path, std::string const& contents)
{
    files[path] = std::make_shared<std::deque<std::string>>();
    files[path]->push_back(contents);

    auto const dirs = split_dirs(path);

    for (auto const& dir : dirs)
        add_dir(dir);
}

std::shared_ptr<std::deque<std::string>>
repowerd::test::FakeFilesystem::add_file_with_live_contents(
    std::string const& path)
{
    files[path] = std::make_shared<std::deque<std::string>>();
    return files[path];
}

void repowerd::test::FakeFilesystem::add_file_ioctl(
    std::string const& path,
    FakeFilesystemIoctlHandler const& handler)
{
    add_file_with_contents(path, "");
    ioctl_handlers[path] = handler;
}

void repowerd::test::FakeFilesystem::add_dir(
    std::string const& path)
{
    directories.insert(path);
}
