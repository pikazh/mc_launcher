#pragma once

#include "base/sigslot.h"
#include "base/CommonDefine.h"
#include "base/NetworkTask.h"
#include "document.h"

#include <memory>
#include <thread>
#include <map>
#include <string>


class OfficialMinecraftVersions
	: public NetworkTask
	, public sigslot::has_slots<>
{
public:
	OfficialMinecraftVersions();
	virtual ~OfficialMinecraftVersions();

	DISABLE_COPY_AND_ASSIGN(OfficialMinecraftVersions)

public:

	enum ReleaseType
	{
		kOldAlpha = 0,
		kOldBeta,
		kSnapshot,
		kRelease,
	};

	struct VersionInfo
	{
		std::string release_time;
		ReleaseType type;
		std::string id;
		std::string url;
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
	bool parse_file(const std::wstring &file);

	virtual bool on_before_schedule() override;
	virtual bool on_after_schedule() override;
	virtual struct curl_slist* on_set_header(struct curl_slist* h) override;

	void on_network_task_state_changed(std::shared_ptr<NetworkTask>, State state);
private:

	bool update_success_ = false;
	std::vector<std::shared_ptr<VersionInfo>> mc_versions_;
	FILE* save_file_ = nullptr;
};

