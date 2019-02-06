#include "BaseWindow.h"

BaseWindow::BaseWindow(const std::wstring & xml_file)
	: xml_file_(xml_file)
{
}

void BaseWindow::InitWindow()
{

}

LRESULT BaseWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_pm.Init(m_hWnd);
	DuiLib::CDialogBuilder builder;
	DuiLib::CControlUI* pRoot = builder.Create(xml_file_.c_str(), (UINT)0, NULL, &m_pm);
	ASSERT(pRoot && "Failed to parse XML");
	m_pm.AttachDialog(pRoot);
	m_pm.AddNotifier(this);

	InitWindow();
	return 0;
}

LRESULT BaseWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = TRUE;
	lRes = OnMessageMap(uMsg, wParam, lParam, bHandled);

	if (bHandled)
		return lRes;

	if (m_pm.MessageHandler(uMsg, wParam, lParam, lRes))
		return lRes;

	return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}

void BaseWindow::Notify(DuiLib::TNotifyUI& msg)
{

}

LRESULT BaseWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	bHandled = FALSE;
	return 0;
}
