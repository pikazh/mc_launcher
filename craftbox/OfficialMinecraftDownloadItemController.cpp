#include "OfficialMinecraftDownloadItemController.h"

OfficialMinecraftDownloadItemController::OfficialMinecraftDownloadItemController(std::shared_ptr<MinecraftDownload> dl_obj)
	: dl_obj_(dl_obj)
{
	type_ = kOfficial;
	state_ = dl_obj_->is_downloading() ? kDownloading : kPaused;

	dl_obj_->download_finished.connect(this, &OfficialMinecraftDownloadItemController::on_task_download_finished);
}


OfficialMinecraftDownloadItemController::~OfficialMinecraftDownloadItemController()
{
}

void OfficialMinecraftDownloadItemController::get_download_progress(uint64_t &downloaded, uint64_t &total)
{
	downloaded = dl_obj_->downloaded_item_num();
	total = dl_obj_->all_download_item_num();
}

void OfficialMinecraftDownloadItemController::pause()
{
	call_action_ = kPause;
	if(dl_obj_->is_downloading())
		dl_obj_->stop_download();
	else
	{
		set_state(DownloadingItemController::kPaused);
	}
}

void OfficialMinecraftDownloadItemController::start()
{
	call_action_ = kStart;
	dl_obj_->start_download(true);
	set_state(kDownloading);
}

void OfficialMinecraftDownloadItemController::remove()
{
	call_action_ = kRemove;
	if (dl_obj_->is_downloading())
		dl_obj_->stop_download();
	else
	{
		set_state(DownloadingItemController::kNeedRemove);
	}
}

void OfficialMinecraftDownloadItemController::on_task_download_finished(std::shared_ptr<MinecraftDownload> dl_obj, bool success)
{
	if (success)
	{
		set_state(DownloadingItemController::kFinish);
	}
	else
	{
		switch (call_action_)
		{
		case kPause:
			set_state(DownloadingItemController::kPaused);
			break;
		case kRemove:
			set_state(DownloadingItemController::kNeedRemove);
			break;
		default:
			set_state(DownloadingItemController::kFailed);
			break;
		}

	}

	call_action_ = kNotCall;
}
