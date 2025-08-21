// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Wally Young <wallywyyoung@users.noreply.github.com>
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ---------------------------------------------------------------------------

#include "ObjectBuffer.h"
#include "aeolus/Semaphore.h"
#include "aeolus/Worker.h"

#include <atomic>
#include <cassert>
#include <thread>
#include <thread_pool/thread_safe_queue.h>

#include "utilities/SimdUtilities.h"

struct Worker::Impl {
    ObjectBuffer<Job*> jobsQueue {};
    Semaphore sema{ 0 };
    std::atomic_bool running { false };
    std::unique_ptr<std::thread> thread{};

    Impl() = default;

    ~Impl() { stop(); }

    void run() {
        SimdUtilities::enableFlushToZero();
        while (running) {
            wait();

            if (Job* job = nullptr; running && jobsQueue.pop(job)) {
                assert(job != nullptr);
                job->run();
            }
        }
        SimdUtilities::disableFlushToZero();
    }

    bool addJob (Job* job) {
        assert(job != nullptr);
        const auto ok = jobsQueue.push(job);
        wakeUp();
        return ok;
    }

    void start() {
        purge();
        if (thread == nullptr) {
            running = true;
            thread = std::make_unique<std::thread> (&Impl::run, this);
        }
    }

    void stop() {
        if (thread != nullptr) {
            running = false;
            wakeUp();
            if (thread->joinable()) {
                thread->join();
            }
        }
    }

    bool isRunning() const noexcept {
        return running;
    }

    void purge() {
        Job* job;
        while (jobsQueue.pop(job)) {
            // Do nothing.
        }
    }

    void wait() {
        sema.wait();
    }

    void wakeUp() {
        sema.notify();
    }
};

//----------------------------------------------------------

Worker::Worker() : d(std::make_unique<Impl>()) { }

Worker::~Worker() = default;

void Worker::start()
{
    d->start();
}

void Worker::stop()
{
    d->stop();
}

bool Worker::addJob(Job* job)
{
    return d->addJob (job);
}

bool Worker::isRunning() const noexcept
{
    return d->isRunning();
}

void Worker::purge()
{
    d->purge();
}


