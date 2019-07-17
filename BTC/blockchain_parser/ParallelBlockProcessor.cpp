#include "ParallelBlockProcessor.h"
#include "globals.h"
#include "Paths.h"
#include <common/misc.h>
#include <thread>

ParallelBlockProcessor::ParallelBlockProcessor(const char *task_name, const Paths &paths, bool testnet)
		: paths(paths)
		, testnet(testnet){
	u64 bytes;
	this->queue = list_block_files(paths.config_path, bytes);
	{
		std::stringstream stream;
		stream << task_name << " (" << SizeFormatter(bytes) << ")...";
		this->progress = TaskProgress(stream.str());
		this->progress.set_total(bytes);
	}
}

void ParallelBlockProcessor::process(int concurrency_limit){
	auto thread_count = std::thread::hardware_concurrency();
	if (concurrency_limit > 0)
		thread_count = std::min<decltype(thread_count)>(thread_count, concurrency_limit);
	std::vector<std::unique_ptr<std::thread>> threads(thread_count);
	this->run = true;
	this->progress.start();
	for (auto &p : threads)
		p.reset(new std::thread([this](){ this->thread_func(); }));
	for (auto &p : threads)
		p->join();
}

class ParallelBlockParser : public AbstractBlockFileParser{
	std::vector<u8> buffer;
	std::string path;
	ParallelBlockProcessor *pbp;
	void *tls;
protected:
	const void *get_data() override{
		if (!this->buffer.size())
			return this;
		return &this->buffer[0];
	}
	size_t get_data_size() override{
		return this->buffer.size();
	}
	std::string get_path() override{
		return this->path;
	}
	void on_block(std::unique_ptr<Block> &&block) override{
		this->pbp->on_block(std::move(block), this->tls);
	}
	void report_progress(u64 p) override{
		this->pbp->progress.report_progress(p);
		this->pbp->report_progress(p);
	}
public:
	ParallelBlockParser(bool testnet, std::vector<u8> &&buffer, const std::string &path, ParallelBlockProcessor &pbp, void *tls)
		: AbstractBlockFileParser(testnet)
		, buffer(std::move(buffer))
		, path(path)
		, pbp(&pbp)
		, tls(tls)
	{}
};

bool ParallelBlockProcessor::internal_continue_running(){
	return this->run && this->continue_running();
}

void ParallelBlockProcessor::thread_func(){
	auto tls = this->get_threadlocal_data();
	std::string path;
	try{
		while (this->internal_continue_running()){
			std::vector<u8> buffer;
			{
				LOCK_MUTEX(this->queue_mutex);
				if (!this->queue.size())
					break;
				path = std::move(this->queue.front());
				this->queue.pop_front();
				buffer = load_file(path);
			}

			ParallelBlockParser pbp(this->testnet, std::move(buffer), path, *this, tls.get());
			pbp.parse();
		}
		this->on_thread_returning(tls.get());
	}catch (std::exception &e){
		mstderr << "While processing " << path << ": " << e.what() << std::endl;
		this->run = false;
	}
}
