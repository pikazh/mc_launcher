#include "DefaultServerList.h"
#include "base/JsonHelper.h"
#include "document.h"
#include "base/asynchronous_task_dispatch.h"

#include <codecvt>

DefaultServerList * DefaultServerList::instance()
{
	static std::shared_ptr<DefaultServerList> inst(new DefaultServerList());
	return inst.get();
}

void DefaultServerList::fetch_server_list()
{
	if (!!http_download_)
		return;

	fetch_finished_ = false;
	server_list_.clear();

	http_download_.reset(new HttpDownloadToBufferWithCache);
	http_download_->state_changed.connect(this, &DefaultServerList::on_nettask_state_changed);
	std::string url = "http://launcher.zuimc.com/servers.php";
	http_download_->set_url(url);
	http_download_->start_download();
}

void DefaultServerList::stop_fetch()
{
	if (!!http_download_)
	{
		http_download_->stop_download();
		http_download_.reset();
	}
}

bool DefaultServerList::is_fetch_finished()
{
	return fetch_finished_;
}

std::vector<struct DefaultServerList::ServerItem> DefaultServerList::server_list()
{
	return server_list_;
}

void DefaultServerList::on_nettask_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state)
{
	if (state == NetworkTask::kStop && task->task_id() == http_download_->task_id())
	{
		if (http_download_->download_success())
		{
			parse_buffer(http_download_->download_buffer());
		}

		http_download_.reset();

		std::weak_ptr<DefaultServerList> weak_obj = this->shared_from_this();
		AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
			{
				obj->fetch_finished_ = true;
				obj->fetch_finished();
			}
				
		}), true);
	}
}

void DefaultServerList::parse_buffer(const std::string &buf)
{
	rapidjson::Document doc;
	doc.Parse(buf.c_str(), buf.length());
	if (doc.IsArray())
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		auto array = doc.GetArray();
		for (auto it = array.begin(); it != array.end(); it++)
		{
			if (it->IsObject())
			{
				std::string name;
				std::string host;
				if (JsonGetStringMemberValue(*it, "name", name) && !name.empty()
					&& JsonGetStringMemberValue(*it, "host", host) && !host.empty())
				{
					ServerItem item;
					item.host = host;
					item.name = name;

					server_list_.push_back(item);
				}
			}
		}
	}
}

