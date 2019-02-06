#include <codecvt>
#include <winsock2.h>
#include <windows.h>
#include <windns.h>
#include <atlstr.h>
#include <atlconv.h>

#include "CraftBoxPageEventHandler.h"
#include "minecraft_common/MinecraftUserIdentity.h"
#include "base/asynchronous_task_dispatch.h"
#include "BoxUserInfoManager.h"
#include "base/asynchronous_task_dispatch.h"
#include "document.h"
#include "base/JsonHelper.h"


HRESULT _stdcall CraftBoxPageEventHandler::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, unsigned int* puArgErr)
{
	if (wFlags == DISPATCH_METHOD)
	{
		if (dispIdMember == 100)
		{
			if(pDispParams->cArgs == 1)
			{
				if (pDispParams->rgvarg[0].vt != VT_BSTR)
				{
					::MessageBox(0, L"不合法的启动参数", mcfe::g_app_name, MB_ICONERROR|MB_OK);
				}
				else
				{
					USES_CONVERSION;
					CAtlStringA json = ATL::CW2A(pDispParams->rgvarg[0].bstrVal);
					rapidjson::Document doc;
					doc.Parse(json);
					if (!doc.IsObject())
					{
						return S_OK;
					}

					//0：离线 1:正版 2 : 创世神 3：离线 4：创世神
					int account_type = 0;
					if (!JsonGetIntMemberValue(doc, "type", account_type))
					{
						return S_OK;
					}

					//转换成本地app的枚举值
					if (account_type == 3 || account_type == 0)
						account_type = mcfe::kOffline;
					else if (account_type == 4 || account_type == 2)
						account_type = mcfe::kCraftBox;
					else if (account_type == 1)
						account_type = mcfe::kOfficial;
					else
					{
						DCHECK(0);
					}
					
					std::string login_tips;
					JsonGetStringMemberValue(doc, "loginmessage", login_tips);
					std::string command;
					if (account_type == mcfe::kCraftBox && !JsonGetStringMemberValue(doc, "command", command))
						return S_OK;
					
					std::string server_name_ansi;
					JsonGetStringMemberValue(doc, "servername", server_name_ansi);
					std::wstring server_name = ATL::CA2W(server_name_ansi.c_str());

					std::string pname;
					if (account_type == mcfe::kCraftBox && !JsonGetStringMemberValue(doc, "playername", pname))
						return S_OK;

					std::wstring player_name = ATL::CA2W(pname.c_str());

					std::string host;
					if (!JsonGetStringMemberValue(doc, "host", host))
					{
						return S_OK;
					}

					std::wstring server = ATL::CA2W(host.c_str());

					int port = -1;
					std::string port_str;
					JsonGetStringMemberValue(doc, "port", port_str);
					if (!port_str.empty())
					{
						port = atoi(port_str.c_str());
						if (port <= 0 || port > 65535)
						{
							port = -1;
						}
					}
					else
					{
						std::wstring query_server = L"_minecraft._tcp." + server;
						PDNS_RECORDW ppQueryResultsSet = NULL;
						DNS_STATUS s = DnsQuery_W(
							query_server.c_str(),
							DNS_TYPE_SRV,
							DNS_QUERY_BYPASS_CACHE,
							NULL,
							&ppQueryResultsSet,
							NULL

						);
						if (s == 0 && ppQueryResultsSet)
						{
							server = ppQueryResultsSet->Data.SRV.pNameTarget;
							//LOG(INFO) << "srv record for " << query_server << "is: " << server;
							port = ppQueryResultsSet->Data.SRV.wPort;
							
							DnsRecordListFree(ppQueryResultsSet, DnsFreeRecordList);
						}
					}

					int mod = 0;
					JsonGetIntMemberValue(doc, "mod", mod);
					bool mod_servre = (mod != 0);

					std::string sid;
					JsonGetStringMemberValue(doc, "sid", sid);
					std::wstring sid_w(sid.begin(), sid.end());

					std::vector<std::wstring> client_versions;
					rapidjson::Value *vers = nullptr;
					if (JsonGetArrayMemberValue(doc, "version", &vers) && vers)
					{
						for (auto it = vers->Begin(); it != vers->End(); it++)
						{
							if (it->IsString() && it->GetStringLength() != 0)
							{
								std::string ver(it->GetString());
								client_versions.push_back(std::wstring(ver.begin(), ver.end()));
							}
						}
					}

					std::weak_ptr<CraftBoxPageEventHandler> weak_obj = this->shared_from_this();
					AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, sid_w, command, server_name, player_name, port, server, account_type, mod_servre, client_versions]()
					{
						auto obj = weak_obj.lock();
						if (!!obj)
						{
							obj->launch_server(sid_w, server, port, server_name, player_name, account_type, command, mod_servre, client_versions);
						}
					}), false);
				}
			}
			else
			{
				::MessageBox(0, L"不合法的启动参数", mcfe::g_app_name, MB_ICONERROR | MB_OK);
			}
		}
		else if (dispIdMember == 101)
		{
			if (pDispParams->cArgs >= 1 && pDispParams->rgvarg[0].vt == VT_BOOL)
			{
				std::weak_ptr<CraftBoxPageEventHandler> weak_obj = this->shared_from_this();
				bool auto_login = (pDispParams->rgvarg[0].boolVal != FALSE);
				AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, auto_login]()
				{
					auto obj = weak_obj.lock();
					if (!!obj)
					{
						obj->login_success(auto_login);
					}
				}), false);
			}
		}
		else if (dispIdMember == 102)
		{
			AsyncTaskDispatch::main_thread()->post_task(make_closure([]()
			{
				BoxUserInfoManager::global()->logout();
			}), true);
			
		}
		else if (dispIdMember == 103)
		{
			if (pDispParams->cArgs >= 1 && pDispParams->rgvarg[0].vt == VT_BSTR)
			{
				std::weak_ptr<CraftBoxPageEventHandler> weak_obj = this->shared_from_this();
				std::wstring url_w(pDispParams->rgvarg[0].bstrVal, ::SysStringLen(pDispParams->rgvarg[0].bstrVal));
				std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
				std::string url = converter.to_bytes(url_w);
				AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, url]()
				{
					auto obj = weak_obj.lock();
					if (!!obj)
					{
						obj->modify_header(url);
					}
				}), true);
			}
		}
		else if (dispIdMember == 104)
		{
			if (pDispParams->cArgs >= 2 && pDispParams->rgvarg[0].vt == VT_BSTR && pDispParams->rgvarg[1].vt == VT_BSTR)
			{
				std::wstring pid(pDispParams->rgvarg[0].bstrVal, ::SysStringLen(pDispParams->rgvarg[0].bstrVal));
				std::wstring sid(pDispParams->rgvarg[1].bstrVal, ::SysStringLen(pDispParams->rgvarg[1].bstrVal));
				std::weak_ptr<CraftBoxPageEventHandler> weak_obj = this->shared_from_this();
				AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, sid, pid]()
				{
					auto obj = weak_obj.lock();
					if (!!obj)
					{
						obj->need_update_pids(sid, pid);
					}
				}), true);
			}
			
		}
		else if (dispIdMember == 105)
		{
			if (pDispParams->cArgs >= 4 && pDispParams->rgvarg[0].vt == VT_BSTR 
				&& pDispParams->rgvarg[1].vt == VT_BSTR 
				&& pDispParams->rgvarg[2].vt == VT_BSTR
				&& pDispParams->rgvarg[3].vt == VT_I4)
			{
				std::wstring url(pDispParams->rgvarg[0].bstrVal, ::SysStringLen(pDispParams->rgvarg[0].bstrVal));
				std::wstring name(pDispParams->rgvarg[1].bstrVal, ::SysStringLen(pDispParams->rgvarg[1].bstrVal));
				std::wstring sha1(pDispParams->rgvarg[2].bstrVal, ::SysStringLen(pDispParams->rgvarg[2].bstrVal));
				uint64_t size = pDispParams->rgvarg[3].intVal;
				std::weak_ptr<CraftBoxPageEventHandler> weak_obj = this->shared_from_this();
				AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, url, name, sha1, size]()
				{
					auto obj = weak_obj.lock();
					if (!!obj)
					{
						USES_CONVERSION;
						obj->download_game(url, name, std::string(ATL::CW2A(sha1.c_str())), size);
					}
				}), false);
			}
		}
	}
	
	return S_OK;
}

HRESULT CraftBoxPageEventHandler::TranslateAccelerator(LPMSG lpMsg, const GUID * pguidCmdGroup, DWORD nCmdID)
{

#ifndef _DEBUG
	if (lpMsg->message == WM_KEYDOWN)
	{
		if (lpMsg->wParam == 0x4E || lpMsg->wParam == 0x4F || lpMsg->wParam == 0x4C) //N O L key
		{
			if ((GetKeyState(VK_CONTROL) & (1 << (sizeof(SHORT) * 8 - 1))) != 0)
			{
				AsyncTaskDispatch::main_thread()->post_task(make_closure([]()
				{
					::MessageBoxW(0, L"小伙子, 看你很有前途, 跟我们最MC合作否? 有任何想法和建议, 请联系QQ1516768567", mcfe::g_app_name, MB_OK);
				}), false);

				return S_OK;
			}
		}
	}

#endif
	return S_FALSE;
}

HRESULT _stdcall CraftBoxPageEventHandler::GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgDispId)
{
	if (wcscmp(rgszNames[0], L"ServerLaunch") == 0)
	{
		*rgDispId = 100;
	}
	else if (wcscmp(rgszNames[0], L"OnLoginSuccess") == 0)
	{
		*rgDispId = 101;
	}
	else if (wcscmp(rgszNames[0], L"OnLogout") == 0)
	{
		*rgDispId = 102;
	}
	else if (wcscmp(rgszNames[0], L"OnModifyHeader") == 0)
	{
		*rgDispId = 103;
	}
	else if (wcscmp(rgszNames[0], L"UpdatePids") == 0)
	{
		*rgDispId = 104;
	}
	else if (wcscmp(rgszNames[0], L"DownloadClient") == 0)
	{
		*rgDispId = 105;
	}
	else
		*rgDispId = DISPID_UNKNOWN;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CraftBoxPageEventHandler::ShowContextMenu(/* [in] */ DWORD dwID, /* [in] */ POINT __RPC_FAR *ppt, /* [in] */ IUnknown __RPC_FAR *pcmdtReserved, /* [in] */ IDispatch __RPC_FAR *pdispReserved)
{
	//return E_NOTIMPL;
	//返回 E_NOTIMPL 正常弹出系统右键菜单
	//返回S_OK 则可屏蔽系统右键菜单
#ifdef _DEBUG
	return E_NOTIMPL;
#else
	return S_OK;
#endif
}

void CraftBoxPageEventHandler::NewWindow3(IDispatch **pDisp, VARIANT_BOOL *&Cancel, DWORD dwFlags, BSTR bstrUrlContext, BSTR bstrUrl)
{
	std::wstring url(bstrUrl, ::SysStringLen(bstrUrl));
	std::transform(url.begin(), url.end(), url.begin(), towlower);

	auto begin_index = url.find(L"?");
	if (begin_index == std::wstring::npos)
	{
		return;
	}

	begin_index += 1;
	auto boxw_index = url.find(L"boxw=", begin_index);
	if (boxw_index == std::wstring::npos)
	{
		return;
	}
	boxw_index += wcslen(L"boxw=");
	auto boxw_index_end = url.find(L"&", boxw_index);
	
	std::wstring box_width_str;
	if (boxw_index_end != std::string::npos)
		box_width_str = url.substr(boxw_index, boxw_index_end - boxw_index);
	else
		box_width_str = url.substr(boxw_index);

	if (box_width_str.empty())
		return;

	int width = _wtoi(box_width_str.c_str());
	if (width <= 0)
		return;

	auto boxh_index = url.find(L"boxh=", begin_index);
	if (boxh_index == std::wstring::npos)
	{
		return;
	}
	boxh_index += wcslen(L"boxh=");
	auto boxh_index_end = url.find(L"&", boxh_index);

	std::wstring box_height_str;
	if (boxh_index_end != std::string::npos)
		box_height_str = url.substr(boxh_index, boxh_index_end - boxh_index);
	else
		box_height_str = url.substr(boxh_index);

	if (box_height_str.empty())
		return;

	int height = _wtoi(box_height_str.c_str());
	if (height <= 0)
		return;


	auto boxl_index = url.find(L"boxl=", begin_index);
	bool lock = false;
	if (boxl_index != std::wstring::npos)
	{
		lock = true;
	}

	*Cancel = VARIANT_TRUE;

	std::weak_ptr<CraftBoxPageEventHandler> weak_obj = this->shared_from_this();
	std::wstring web_page_url(bstrUrl, ::SysStringLen(bstrUrl));
	AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, web_page_url, width, height, lock]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->new_web_page(web_page_url, width, height, lock);
		}
	}), false);
}

void CraftBoxPageEventHandler::CommandStateChange(long Command, VARIANT_BOOL Enable)
{
	switch (Command)
	{
	case CSC_NAVIGATEFORWARD:
		forward_enable_ = (Enable != VARIANT_FALSE);
		break;
	case CSC_NAVIGATEBACK:
		backward_enable_ = (Enable != VARIANT_FALSE);
		break;
	default:
		return;
	}

	std::weak_ptr<CraftBoxPageEventHandler> weak_obj = this->shared_from_this();
	AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->navigator_command_state_changed(obj.get());
		}
	}), true);
}

void CraftBoxPageEventHandler::TitleChange(BSTR title)
{
	std::weak_ptr<CraftBoxPageEventHandler> weak_obj = this->shared_from_this();
	std::wstring title_str(title, ::SysStringLen(title));
	AsyncTaskDispatch::main_thread()->post_task(make_closure([weak_obj, title_str]()
	{
		auto obj = weak_obj.lock();
		if (!!obj)
		{
			obj->title_changed(title_str);
		}
	}), false);
}

