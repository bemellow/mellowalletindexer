#pragma once
#include "globals.h"
#include <common/misc.h>
#include <mutex>
#include <string>
#include <chrono>
#include <iostream>
#include <boost/optional.hpp>
#include <array>
#include <cstring>
#include <cstdio>

class TaskProgress{
	typedef std::chrono::steady_clock::time_point time_t;
	std::string task_name;
	double progress = -1;
	double total = -1;
	boost::optional<time_t> started;
	boost::optional<time_t> last_update;
	int state = 0;

	std::mutex mutex;
	static const std::chrono::steady_clock::rep ns_per_sec = 1'000'000'000;

	struct estimate{
		int h, m, s;
	};

	static time_t now(){
		return std::chrono::steady_clock::now();
	}
	static void write_line(char *dst, int state, int percentage, const estimate &e, bool complete){
		static const char spinner[] = "|/-\\";
		auto c = spinner[state % 4];
		if (!(state % 10) || complete){
			percentage = std::min(percentage, 100);

			if (e.h >= 0)
				sprintf(dst, "%c %02d:%02d:%02d (%3d%%) [", c, e.h, e.m, e.s, percentage);
			else
				sprintf(dst, "%c ??:??:?? (%3d%%) [", c, percentage);
			if (percentage >= 0){
				auto on = percentage * 59 / 100;
				on = std::min(on, 59);
				auto off = 59 - on;
				memset(dst + 19, '#', on);
				memset(dst + 19 + on, ' ', off);
			}else{
				
			}
			dst[78] = ']';
			dst[79] = !complete ? '\r' : '\n';
			dst[80] = 0;
		}else{
			dst[0] = c;
			dst[1] = '\r';
			dst[2] = 0;
		}
	}
	static estimate estimate_remaining(double elapsed, double total, double progress){
		auto seconds_d = ceil((elapsed / progress) * (total - progress));
		if (seconds_d > 99 * 3600 + 59 * 60 + 59)
			return {99, 59, 59};
		auto s = (int)seconds_d;
		auto h = s / 3600;
		s %= 3600;
		auto m = s / 60;
		s %= 60;
		return {h, m, s};
	}
public:
	TaskProgress(){}
	TaskProgress(const TaskProgress &) = delete;
	const TaskProgress &operator=(const TaskProgress &) = delete;
	TaskProgress(TaskProgress &&other){
		*this = std::move(other);
	}
	const TaskProgress &operator=(TaskProgress &&other){
		this->task_name = std::move(other.task_name);
		this->progress = other.progress;
		this->total = other.total;
		this->started = std::move(other.started);
		this->last_update = std::move(other.last_update);
		return *this;
	}
	TaskProgress(const std::string &name): task_name(name){
		mstdout << current_time_string() << " - " << name << std::endl;
	}
	~TaskProgress(){
		if (this->started.has_value()){
			char line[128];
			write_line(line, this->state, 100, {0, 0, 0}, true);
			mstdout << line;
		}
	}
	void set_total(double total){
		this->total = total;
	}
	void start(double total = -1){
		this->started = this->now();
		if (total >= 0)
			this->total = total;
		this->progress = 0;

		char line[128];
		write_line(line, this->state, 0, {-1, -1, -1}, false);
		mstdout << line;
	}
	void report_progress(double added_progress = -1){
		if (!this->started.has_value())
			return;
		auto now = this->now();

		int percentage = -1;
		estimate e = {-1, -1, -1};
		int state;
		{
			LOCK_MUTEX(this->mutex);
			if (added_progress >= 0)
				this->progress += added_progress;
			if (this->last_update.has_value() && now - *this->last_update < std::chrono::milliseconds(100))
				return;
			this->last_update = now;
			if (this->progress >= 0 && this->total >= 0)
				percentage = (int)(this->progress / this->total * 100);
			if (this->started.has_value())
				e = this->estimate_remaining((double)(now - *this->started).count() / ns_per_sec, this->total, this->progress);
			state = this->state++;
			this->state %= 20;
		}
		char line[128];
		write_line(line, state, percentage, e, false);
		mstdout << line;
	}
};
