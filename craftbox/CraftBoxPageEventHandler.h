#pragma once

#include "DuiLib/UIlib.h"
#include "DuiLib/Utils/WebBrowserEventHandler.h"
#include "base/sigslot.h"

#include <memory>

class CraftBoxPageEventHandler
	: public DuiLib::CWebBrowserEventHandler
	, public IDispatch
	, public std::enable_shared_from_this<CraftBoxPageEventHandler>
{
public:
	CraftBoxPageEventHandler()
	{
		refNum_ = 1;
	}
	~CraftBoxPageEventHandler(void)
	{
	}
public:

	virtual HRESULT STDMETHODCALLTYPE GetExternal(
		/* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispatch)
	{
		*ppDispatch = this;
		AddRef();
		return S_OK;
	}
	// IUnknown Methods

	STDMETHODIMP QueryInterface(REFIID iid, void**ppvObject)
	{
		*ppvObject = NULL;

		if (iid == IID_IUnknown)
			*ppvObject = this;
		else if (iid == IID_IDispatch)
			*ppvObject = (IDispatch*)this;
		if (*ppvObject != nullptr)
		{
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return ::InterlockedIncrement(&refNum_);
	}

	STDMETHODIMP_(ULONG) Release()
	{
		::InterlockedDecrement(&refNum_);
		if (refNum_ == 0)
		{
			delete this;
		}
		return refNum_;
	}

	// IDispatch Methods

	HRESULT _stdcall GetTypeInfoCount(
		unsigned int * pctinfo)
	{
		return E_NOTIMPL;
	}

	HRESULT _stdcall GetTypeInfo(
		unsigned int iTInfo,
		LCID lcid,
		ITypeInfo FAR* FAR* ppTInfo)
	{
		return E_NOTIMPL;
	}

	HRESULT _stdcall GetIDsOfNames(
		REFIID riid,
		OLECHAR FAR* FAR* rgszNames,
		unsigned int cNames,
		LCID lcid,
		DISPID FAR* rgDispId
	);

	HRESULT _stdcall Invoke(
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS* pDispParams,
		VARIANT* pVarResult,
		EXCEPINFO* pExcepInfo,
		unsigned int* puArgErr
	);

	virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(
		/* [in] */ LPMSG lpMsg,
		/* [in] */ const GUID __RPC_FAR *pguidCmdGroup,
		/* [in] */ DWORD nCmdID) override;

	virtual HRESULT STDMETHODCALLTYPE ShowContextMenu(
		/* [in] */ DWORD dwID,
		/* [in] */ POINT __RPC_FAR *ppt,
		/* [in] */ IUnknown __RPC_FAR *pcmdtReserved,
		/* [in] */ IDispatch __RPC_FAR *pdispReserved);

	virtual void NewWindow3(IDispatch **pDisp, VARIANT_BOOL *&Cancel, DWORD dwFlags, BSTR bstrUrlContext, BSTR bstrUrl) override;

	virtual void CommandStateChange(long Command, VARIANT_BOOL Enable) override;
	virtual void TitleChange(BSTR title) override;

	sigslot::signal9<std::wstring, std::wstring, int, std::wstring, std::wstring, int, std::string, bool, std::vector<std::wstring>> launch_server;
	sigslot::signal1<bool> login_success;
	sigslot::signal1<const std::string&> modify_header;
	sigslot::signal2<std::wstring, std::wstring> need_update_pids;
	sigslot::signal4<std::wstring, std::wstring, std::string, uint64_t> download_game;
	sigslot::signal1<CraftBoxPageEventHandler*> navigator_command_state_changed;
	sigslot::signal4<std::wstring, int, int, bool> new_web_page;
	sigslot::signal1<std::wstring> title_changed;

	bool backward_enable_ = false;
	bool forward_enable_ = false;

private:
	
	long refNum_;
};