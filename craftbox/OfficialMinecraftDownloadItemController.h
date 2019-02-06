#pragma once

#include "DownloadingItemController.h"
#include "minecraft_common\MinecraftDownload.h"

#include <memory>
#include <string>

class OfficialMinecraftDownloadItemController
	: public DownloadingItemController
{
public:
	OfficialMinecraftDownloadItemController(std::shared_ptr<MinecraftDownload> dl_obj);
	virtual ~OfficialMinecraftDownloadItemController();

	virtual void get_download_progress(uint64_t &downloaded, uint64_t &total) override;

	virtual void start() override;
	virtual void pause() override;
	virtual void remove() override;


	std::shared_ptr<MinecraftDownload> download_obj() { return dl_obj_; }

protected:
	void on_task_download_finished(std::shared_ptr<MinecraftDownload> dl_obj, bool success);
private:

	enum CallAction
	{
		kNotCall = -1,
		kStart = 0,
		kPause,
		kRemove,
	};

	std::shared_ptr<MinecraftDownload> dl_obj_;
	CallAction call_action_ = kNotCall;
};

