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

#pragma once

#include <memory>
#include <functional>

/**
 * @brief Schedule jobs run on a separate thread.
 *
 * This class uses a lock-free queue to schedule jobs
 * on the audio thread and execute them on a side thread.
 * This is normally used for samples streaming.
 */
class Worker final
{
public:

    class Job
    {
    public:
        virtual void run() = 0;
        virtual ~Job() = default;
    };

    //------------------------------------------------------

    /// Maximum allowed number of queued jobs
    constexpr static size_t DefaultCapacity = 1024;

    Worker();
    ~Worker();
    Worker(const Worker&) = delete;
    Worker& operator = (const Worker&) = delete;

    void start();
    void stop();

    /**
     * @brief Add job to the queue.
     *
     * @note This must be called from audio thread only.
     */
    bool addJob(Job* job);
    [[nodiscard]] bool isRunning() const noexcept;

    void purge();

private:
    struct Impl;
    std::unique_ptr<Impl> d{};
};


