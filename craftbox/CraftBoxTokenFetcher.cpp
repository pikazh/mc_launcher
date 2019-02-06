#include "CraftBoxTokenFetcher.h"
#include "base/JsonHelper.h"
#include "document.h"
#include "base/asynchronous_task_dispatch.h"

#include <codecvt>

CraftBoxTokenFetcher::CraftBoxTokenFetcher()
{
}


CraftBoxTokenFetcher::~CraftBoxTokenFetcher()
{
}

void CraftBoxTokenFetcher::begin_fetch(const std::string &cookie, const std::string &pid)
{
	if (!!http_download_)
		return;

	craftbox_player_token_info_.reset();
	http_download_.reset(new HttpDownloadToBufferWithCache);
	http_download_->state_changed.connect(this, &CraftBoxTokenFetcher::on_nettask_state_changed);
	std::string url = "http://user.zuimc.com/Api/player_prelogin?pid=" + pid;
	http_download_->set_url(url);
	http_download_->set_cookie(cookie);
	http_download_->start_download();

	pid_ = std::wstring(pid.begin(), pid.end());
}

void CraftBoxTokenFetcher::stop_fetch()
{
	if (!!http_download_)
	{
		http_download_->stop_download();
	}
}

void CraftBoxTokenFetcher::on_nettask_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state)
{
	if (state == NetworkTask::kStop && task->task_id() == http_download_->task_id())
	{
		if (http_download_->download_success())
		{
			parse_buffer(http_download_->download_buffer());
		}

		http_download_.reset();

		std::weak_ptr<CraftBoxTokenFetcher> weak_obj = this->shared_from_this();
		AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
		{
			auto obj = weak_obj.lock();
			if (!!obj)
				obj->fetch_finished();
		}), true);
	}
}

void CraftBoxTokenFetcher::parse_buffer(const std::string &buf)
{
	rapidjson::Document doc;
	doc.Parse(buf.c_str(), buf.length());
	if (doc.IsObject())
	{
		std::string code, msg;
		JsonGetStringMemberValue(doc, "code", code);
		JsonGetStringMemberValue(doc, "msg", msg);
		if (code != "0")
		{
			if (!msg.empty())
			{
				craftbox_player_token_info_.reset(new CraftBoxPlayerTokenInfo);
				std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
				craftbox_player_token_info_->err_msg = converter.from_bytes(msg);
				return;
			}
			else
			{
				DCHECK(0);
			}
		}
		else
		{
			rapidjson::Value *data = nullptr;
			JsonGetObjectMemberValue(doc, "data", &data);
			if (data != nullptr)
			{
				std::string player_name, ban = "0", session;
				JsonGetStringMemberValue(*data, "playername", player_name);
				if (player_name.empty())
					return;

				JsonGetStringMemberValue(*data, "ban", ban);

				JsonGetStringMemberValue(*data, "session", session);
				if (session.empty())
					return;

				craftbox_player_token_info_.reset(new CraftBoxPlayerTokenInfo);
				craftbox_player_token_info_->pid = pid_;
				craftbox_player_token_info_->session = std::wstring(session.begin(), session.end());
				craftbox_player_token_info_->ban = ban;
				craftbox_player_token_info_->player_name = std::wstring(player_name.begin(), player_name.end());

			}
		}
	}
}
