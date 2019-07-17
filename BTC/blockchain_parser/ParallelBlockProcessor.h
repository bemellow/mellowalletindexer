#pragma once

#include "Paths.h"
#include <libbtcparser/Block.h>
#include "ProgressDisplay.h"
#include <atomic>
#include <memory>

class ParallelBlockProcessor{
	std::atomic<bool> run;
	std::deque<std::string> queue;
	std::mutex queue_mutex;
	TaskProgress progress;
	bool testnet;

	void thread_func();
	bool internal_continue_running();
protected:
	Paths paths;

	friend class ParallelBlockParser;
	virtual bool continue_running(){
		return true;
	}
	virtual void on_block(std::unique_ptr<Block> &&block, void *tls) = 0;
	virtual void report_progress(u64 p){}
	typedef std::unique_ptr<void, void(*)(void *)> tls_t;
	static void null_releaser(void *){}
	virtual tls_t get_threadlocal_data(){
		return { nullptr, null_releaser };
	}
	virtual void on_thread_returning(void *tls){}
public:
	ParallelBlockProcessor(const char *task_name, const Paths &paths, bool testnet);
	virtual ~ParallelBlockProcessor(){}
	virtual void process(int concurrency_limit = 0);
};
