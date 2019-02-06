#include "UserJoinedServerInfo.h"
#include "base/JsonHelper.h"
#include "document.h"
#include "writer.h"
#include "glog/logging.h"
#include "base/asynchronous_task_dispatch.h"

#include <codecvt>
#include <windns.h>

UserJoinedServerInfo::UserJoinedServerInfo()
{
}

UserJoinedServerInfo::~UserJoinedServerInfo()
{
}

void UserJoinedServerInfo::begin_fetch(const std::string &cookie)
{
	if (!!http_download_)
		return;

	join_server_infos_.clear();
	http_download_.reset(new HttpDownloadToBufferWithCache);
	http_download_->state_changed.connect(this, &UserJoinedServerInfo::on_nettask_state_changed);
	http_download_->set_url("http://user.zuimc.com/Api/list_join_server");
	http_download_->set_cookie(cookie);
	http_download_->start_download();
}

void UserJoinedServerInfo::stop_fetch()
{
	if (!!http_download_)
	{
		http_download_->stop_download();
	}
}

void UserJoinedServerInfo::on_nettask_state_changed(std::shared_ptr<NetworkTask> task, NetworkTask::State state)
{
	if (state == NetworkTask::kStop && task->task_id() == http_download_->task_id())
	{
		std::weak_ptr<UserJoinedServerInfo> weak_obj = this->shared_from_this();
		//加了DNS解析，导致会卡主线程UI，所以移动到了单独线程
		std::thread t([weak_obj]()
		{
			auto obj = weak_obj.lock();
			if (!obj)
				return;

			if (obj->http_download_->download_success())
			{
				obj->parse_buffer(obj->http_download_->download_buffer());
			}

			obj->http_download_.reset();

			AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
			{
				auto obj = weak_obj.lock();
				if (!!obj)
					obj->fetch_finished();
			}), true);
		});
		t.detach();
	}
}

void UserJoinedServerInfo::parse_buffer(const std::string &buf)
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
			LOG(ERROR) << " user join server info not right: " << code << msg;
		}
		else
		{
			rapidjson::Value *data = nullptr;
			JsonGetArrayMemberValue(doc, "data", &data);
			if (data != nullptr)
			{
				std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
				for (auto it = data->Begin(); it != data->End(); it++)
				{
					if (it->IsObject())
					{
						
						std::string sid;
						JsonGetStringMemberValue(*it, "sid", sid);
						if(sid.empty())
							continue;

						std::wstring sid_w(sid.begin(), sid.end());

						auto it2 = join_server_infos_.find(sid_w);
						if (it2 != join_server_infos_.end())
						{
							std::string pid, player_name;
							JsonGetStringMemberValue(*it, "pid", pid);
							if (pid.empty())
								continue;

							JsonGetStringMemberValue(*it, "player_name", player_name);
							if (player_name.empty())
								continue;

							std::string ban;
							JsonGetStringMemberValue(*it, "ban", ban);

							std::shared_ptr<PlayerInfoInServer> player_info(new PlayerInfoInServer);
							player_info->pid = std::wstring(pid.begin(), pid.end());
							player_info->player_name = std::wstring(player_name.begin(), player_name.end());
							player_info->ban = ban;
							it2->second->players.push_back(player_info);
						}
						else
						{
							
							std::string server_name;
							JsonGetStringMemberValue(*it, "server_name", server_name);
							if (server_name.empty())
								continue;

							std::string host_and_port, host;
							uint16_t port = 25565;
							JsonGetStringMemberValue(*it, "host", host_and_port);
							if (host_and_port.empty())
								continue;

							auto index = host_and_port.find(":");
							if (index == std::string::npos)
							{
								std::string query_server = "_minecraft._tcp." + host_and_port;
								PDNS_RECORDA ppQueryResultsSet = NULL;
								DNS_STATUS s = DnsQuery_A(
									query_server.c_str(),
									DNS_TYPE_SRV,
									DNS_QUERY_BYPASS_CACHE,
									NULL,
									(PDNS_RECORD *)&ppQueryResultsSet,
									NULL
								);
								if (s == 0 && ppQueryResultsSet)
								{
									host = ppQueryResultsSet->Data.SRV.pNameTarget;
									//LOG(INFO) << "srv record for " << query_server << "is: " << server;
									port = ppQueryResultsSet->Data.SRV.wPort;

									DnsRecordListFree(ppQueryResultsSet, DnsFreeRecordList);
								}
								else
								{
									host = host_and_port;
								}
								
							}
							else
							{
								host = host_and_port.substr(0, index);
								std::string port_str = host_and_port.substr(index + 1);
								port = atoi(port_str.c_str());
							}

							std::string pid, player_name;
							JsonGetStringMemberValue(*it, "pid", pid);
							if (pid.empty())
								continue;

							JsonGetStringMemberValue(*it, "player_name", player_name);
							if (player_name.empty())
								continue;

							std::string ban;
							JsonGetStringMemberValue(*it, "ban", ban);

							std::shared_ptr<JoinServerInfo> join_server_info(new JoinServerInfo);
							join_server_info->host = std::wstring(host.begin(), host.end());
							join_server_info->port = port;

							join_server_info->server_name = converter.from_bytes(server_name);
							join_server_info->sid = sid_w;
							std::shared_ptr<PlayerInfoInServer> player_info(new PlayerInfoInServer);
							player_info->pid = std::wstring(pid.begin(), pid.end());
							player_info->player_name = std::wstring(player_name.begin(), player_name.end());
							player_info->ban = ban;
							join_server_info->players.push_back(player_info);

							join_server_infos_[sid_w] = join_server_info;
						}
					}
				}
			}
		}
	}
}
