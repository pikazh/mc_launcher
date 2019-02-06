#pragma once

#include "base/sigslot.h"
#include "base/CommonDefine.h"
#include "base/NetworkTask.h"
#include "document.h"

#include <memory>
#include <thread>
#include <map>
#include <string>


class IntegrationMinecraftVersions
	: public NetworkTask
	, public sigslot::has_slots<>
{
public:
	IntegrationMinecraftVersions();
	virtual ~IntegrationMinecraftVersions();

	DISABLE_COPY_AND_ASSIGN(IntegrationMinecraftVersions)

public:

	struct VersionInfo
	{
		std::wstring name;
		std::wstring desc;
		std::string sha1;
		uint64_t size;
		std::string profile_url;
	};

	void start_update_lists();
	void stop_update_lists();

	sigslot::signal1<bool> update_finish;

	auto version_list()
	{
		return mc_versions_;
	}

	bool update_success() { return update_success_; }

protected:
	static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata);
	bool parse_content();

	virtual bool on_before_schedule() override;
	virtual bool on_after_schedule() override;
	virtual struct curl_slist* on_set_header(curl_slist* header) override;

	void on_network_task_state_changed(std::shared_ptr<NetworkTask>, State state);
private:

	bool update_success_ = false;
	std::vector<std::shared_ptr<VersionInfo>> mc_versions_;
	std::string buffer_;
};

