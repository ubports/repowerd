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

#include "event_loop.h"

repowerd::EventLoop::EventLoop()
    : main_context{g_main_context_new()},
      main_loop{g_main_loop_new(main_context, FALSE)}
{
    loop_thread = std::thread{
        [this]
        {
            g_main_context_push_thread_default(main_context);
            g_main_loop_run(main_loop);
        }};

    enqueue([]{}).wait();
}

repowerd::EventLoop::~EventLoop()
{
    stop();
}

void repowerd::EventLoop::stop()
{
    if (main_loop)
        g_main_loop_quit(main_loop);
    if (loop_thread.joinable())
        loop_thread.join();
    if (main_loop)
    {
        g_main_loop_unref(main_loop);
        main_loop = nullptr;
    }
    if (main_context)
    {
        g_main_context_unref(main_context);
        main_context = nullptr;
    }
}

std::future<void> repowerd::EventLoop::enqueue(std::function<void()> const& callback)
{
    struct IdleContext
    {
        IdleContext(std::function<void()> const& callback)
            : callback{callback}
        {
        }

        static gboolean static_call(IdleContext* ctx)
        {
            try
            {
                ctx->callback();
                ctx->done.set_value();
            }
            catch (...)
            {
                ctx->done.set_exception(std::current_exception());
            }
            return G_SOURCE_REMOVE;
        }

        static void static_destroy(IdleContext* ctx) { delete ctx; }
        std::function<void()> const callback;
        std::promise<void> done;
    };

    auto const gsource = g_idle_source_new();
    auto const ctx = new IdleContext{callback};
    g_source_set_callback(
            gsource,
            reinterpret_cast<GSourceFunc>(&IdleContext::static_call),
            ctx,
            reinterpret_cast<GDestroyNotify>(&IdleContext::static_destroy));

    auto future = ctx->done.get_future();

    g_source_attach(gsource, main_context);
    g_source_unref(gsource);

    return future;
}

std::future<void> repowerd::EventLoop::schedule_in(
    std::chrono::milliseconds timeout,
    std::function<void()> const& callback)
{
    struct TimeoutContext
    {
        TimeoutContext(std::function<void()> const& callback)
            : callback{callback}
        {
        }

        static gboolean static_call(TimeoutContext* ctx)
        {
            try
            {
                ctx->callback();
                ctx->done.set_value();
            }
            catch (...)
            {
                ctx->done.set_exception(std::current_exception());
            }
            return G_SOURCE_REMOVE;
        }

        static void static_destroy(TimeoutContext* ctx) { delete ctx; }
        std::function<void()> const callback;
        std::promise<void> done;
    };

    auto const gsource = g_timeout_source_new(timeout.count());
    auto const ctx = new TimeoutContext{callback};
    g_source_set_callback(
            gsource,
            reinterpret_cast<GSourceFunc>(&TimeoutContext::static_call),
            ctx,
            reinterpret_cast<GDestroyNotify>(&TimeoutContext::static_destroy));

    auto future = ctx->done.get_future();

    g_source_attach(gsource, main_context);
    g_source_unref(gsource);

    return future;
}
