#pragma once

#include "base/sigslot.h"

#include <memory>

class DownloadingItemController
	: public sigslot::has_slots<>
	, public std::enable_shared_from_this<DownloadingItemController>
{
public:
	DownloadingItemController() = default;
	virtual ~DownloadingItemController() {};

public:

	enum ItemType
	{
		kOfficial= 0,
		kThirdParty,
	};

	enum State
	{
		kWaiting = 0,
		kDownloading,
		kUnZipping,
		kFinish,
		kFailed,
		kPaused,
		kNeedRemove,
	};

	virtual void start() = 0;
	virtual void pause() = 0;
	virtual void remove() = 0;

	virtual void get_download_progress(uint64_t &downloaded, uint64_t &total) = 0;

	sigslot::signal2<std::shared_ptr<DownloadingItemController>, State> state_changed;
	State state() { return state_; }
	ItemType type() { return type_; }
protected:
	void set_state(State state)
	{
		state_ = state;
		state_changed(this->shared_from_this(), state_);
	}

protected:
	State state_;
	ItemType type_;
};

