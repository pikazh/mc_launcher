#include "IntergrationDownloadItemController.h"
#include "glog/logging.h"
#include "minecraft_common/ZipCmdProxy.h"
#include "base/asynchronous_task_dispatch.h"
#include "sha.h"
#include "cryptlib.h"
#include "hex.h"
#include "files.h"

#include <thread>
#include <codecvt>

IntergrationDownloadItemController::IntergrationDownloadItemController(std::shared_ptr<HttpResumableDownloadToFile> task)
	: dl_obj_(task)
{
	this->type_ = DownloadingItemController::kThirdParty;

	switch (dl_obj_->state())
	{
	case NetworkTask::kStop:
		state_ = kPaused;
		break;
	case NetworkTask::kRunning:
		state_ = kDownloading;
		break;
	case NetworkTask::kWaiting:
		state_ = kWaiting;
		break;
	default:
		DCHECK(0);
		break;
	}

	dl_obj_->state_changed.connect(this, &IntergrationDownloadItemController::on_network_task_state_changed);
}

IntergrationDownloadItemController::~IntergrationDownloadItemController()
{
}

void IntergrationDownloadItemController::get_download_progress(uint64_t &downloaded, uint64_t &total)
{
	downloaded = dl_obj_->dl_current();
	total = file_size_;
}

void IntergrationDownloadItemController::start()
{
	call_action_ = kStart;
	if (!dl_obj_->is_downloading())
	{
		dl_obj_->start_download();
	}
}

void IntergrationDownloadItemController::pause()
{
	call_action_ = kPause;

	if(state() == kWaiting || state() == kDownloading)
		dl_obj_->stop_download();

}

void IntergrationDownloadItemController::remove()
{
	call_action_ = kRemove;
	if (kUnZipping != state())
	{
		if (dl_obj_->is_downloading())
			dl_obj_->stop_download();
		else
		{
			set_state(DownloadingItemController::kNeedRemove);
		}
	}
	
}

void IntergrationDownloadItemController::on_network_task_state_changed(std::shared_ptr<NetworkTask> dl_obj, NetworkTask::State state)
{
	switch (state)
	{
	case NetworkTask::kStop:
		if (dl_obj_->download_success())
		{
			set_state(DownloadingItemController::kUnZipping);

			std::weak_ptr<IntergrationDownloadItemController> weak_obj = std::dynamic_pointer_cast<IntergrationDownloadItemController>(this->shared_from_this());
			std::thread t([weak_obj]()
			{
				auto obj = weak_obj.lock();
				if (!!obj)
				{
					if (!obj->check_download_file_integrity())
					{
						AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
						{
							auto obj = weak_obj.lock();
							if (!obj)
								return;

							obj->set_state(DownloadingItemController::kFailed);
						}), true);
						
						return;
					}

					ZipCmdProxy zip_cmd;
					std::wstring cmd = L"x \"" + obj->dl_obj_->download_file_path() + L"\" -o\"" + obj->file_save_path_ + L"\" -y";
					int ret = zip_cmd.run(cmd);

					::DeleteFileW(obj->dl_obj_->download_file_path().c_str());

					AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, ret]()
					{
						auto obj = weak_obj.lock();
						if (!obj)
							return;

						if (ret != 0)
						{
							std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
							auto zip_path = converter.to_bytes(obj->dl_obj_->download_file_path());
							auto file_path = converter.to_bytes(obj->file_save_path_);
							LOG(WARNING) << "unzip " << zip_path << "to " << file_path << "failed with err" << ret;
						}

						obj->set_state(DownloadingItemController::kFinish);

					}), true);

				}

			});
			t.detach();
		}
		else
		{
			switch (call_action_)
			{
			case IntergrationDownloadItemController::kPause:
				set_state(DownloadingItemController::kPaused);
				break;
			case IntergrationDownloadItemController::kRemove:
				set_state(DownloadingItemController::kNeedRemove);
				break;
			default:
				set_state(DownloadingItemController::kFailed);
				break;
			}
		}
		break;
	case NetworkTask::kWaiting:
		set_state(DownloadingItemController::kWaiting);
		break;
	case NetworkTask::kRunning:
		set_state(DownloadingItemController::kDownloading);
		break;
	default:
		break;
	}

	call_action_ = kNotCall;
}

bool IntergrationDownloadItemController::check_download_file_integrity()
{
	if (file_size_ == 0 && sha1_.empty())
	{
		//std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		//LOG(WARNING) << "no file size info or sha1 info, skip file checking for" << converter.to_bytes(dl_obj_->download_file_path());
		return true;
	}

	std::ifstream file_stream(dl_obj_->download_file_path(), std::ifstream::binary | std::ifstream::in);
	if (!file_stream.is_open())
	{
		DCHECK(0);
		return false;
	}

	file_stream.seekg(0, std::ios_base::end);
	uint64_t sz = file_stream.tellg();
	if (file_size_ != 0 && sz != file_size_)
	{
		return false;
	}

	if (sha1_.empty())
		return true;

	file_stream.seekg(0, std::ios_base::beg);

	std::string value;
	{
		CryptoPP::SHA1 hash;
		CryptoPP::FileSource fs(file_stream, true /* PumpAll */,
			new CryptoPP::HashFilter(hash,
				new CryptoPP::HexEncoder(
					new CryptoPP::StringSink(value),
					false
				) // HexEncoder
			) // HashFilter
		); // FileSource
	}

	std::transform(sha1_.begin(), sha1_.end(), sha1_.begin(), ::tolower);
	if (0 == sha1_.compare(value))
	{
		return true;
	}

	return false;
}
