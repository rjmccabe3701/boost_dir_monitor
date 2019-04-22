//
// Copyright (c) 2008, 2009 Boris Schaeling <boris@highscore.de>
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_BASIC_DIR_MONITOR_SERVICE_HPP
#define BOOST_ASIO_BASIC_DIR_MONITOR_SERVICE_HPP

#include "dir_monitor_impl.hpp"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <string>
#include <stdexcept>

namespace boost {
namespace asio {

template <typename DirMonitorImplementation = dir_monitor_impl>
class basic_dir_monitor_service
    : public boost::asio::io_context::service
{
public:
    static boost::asio::io_context::id id;

    explicit basic_dir_monitor_service(boost::asio::io_context &io_context)
        : boost::asio::io_context::service(io_context),
        async_monitor_work_(new boost::asio::io_context::work(async_monitor_io_context_)),
        async_monitor_thread_(boost::bind(&boost::asio::io_context::run, &async_monitor_io_context_))
    {
    }

    ~basic_dir_monitor_service()
    {
        // The async_monitor thread will finish when async_monitor_work_ is reset as all asynchronous
        // operations have been aborted and were discarded before (in destroy).
        async_monitor_work_.reset();

        // Event processing is stopped to discard queued operations.
        async_monitor_io_context_.stop();

        // The async_monitor thread is joined to make sure the directory monitor service is
        // destroyed _after_ the thread is finished (not that the thread tries to access
        // instance properties which don't exist anymore).
        async_monitor_thread_.join();
    }

    typedef boost::shared_ptr<DirMonitorImplementation> implementation_type;

    void construct(implementation_type &impl)
    {
        impl.reset(new DirMonitorImplementation());

        // begin_read() can't be called within the constructor but must be called
        // explicitly as it calls shared_from_this().
        impl->begin_read();
    }

    void destroy(implementation_type &impl)
    {
        // If an asynchronous call is currently waiting for an event
        // we must interrupt the blocked call to make sure it returns.
        impl->destroy();

        impl.reset();
    }

    void add_directory(implementation_type &impl, const std::string &dirname)
    {
        if (!boost::filesystem::is_directory(dirname))
            throw std::invalid_argument("boost::asio::basic_dir_monitor_service::add_directory: " + dirname + " is not a valid directory entry");

        impl->add_directory(dirname);
    }

    void remove_directory(implementation_type &impl, const std::string &dirname)
    {
        impl->remove_directory(dirname);
    }

    dir_monitor_event monitor(implementation_type &impl, boost::system::error_code &ec)
    {
        return impl->popfront_event(ec);
    }

    template <typename Handler>
    class monitor_operation
    {
    public:
        monitor_operation(implementation_type &impl, boost::asio::io_context &io_context, Handler handler)
            : impl_(impl),
            io_context_(io_context),
            work_(io_context),
            handler_(handler)
        {
        }

        void operator()() const
        {
            implementation_type impl = impl_.lock();
            if (impl)
            {
                boost::system::error_code ec;
                dir_monitor_event ev = impl->popfront_event(ec);
                this->io_context_.post(boost::asio::detail::bind_handler(handler_, ec, ev));
            }
            else
            {
                this->io_context_.post(boost::asio::detail::bind_handler(handler_, boost::asio::error::operation_aborted, dir_monitor_event()));
            }
        }

    private:
        boost::weak_ptr<DirMonitorImplementation> impl_;
        boost::asio::io_context &io_context_;
        boost::asio::io_context::work work_;
        Handler handler_;
    };

    template <typename Handler>
    void async_monitor(implementation_type &impl, Handler handler)
    {
        this->async_monitor_io_context_.post(monitor_operation<Handler>(impl,
                 this->get_io_context(), handler));
    }

private:
    void shutdown_service()
    {
    }

    boost::asio::io_context async_monitor_io_context_;
    boost::scoped_ptr<boost::asio::io_context::work> async_monitor_work_;
    boost::thread async_monitor_thread_;
};

template <typename DirMonitorImplementation>
boost::asio::io_context::id basic_dir_monitor_service<DirMonitorImplementation>::id;

}
}

#endif
