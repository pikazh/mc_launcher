#include "LoadingWnd.h"

LRESULT LoadingWnd::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled)
{
	if (wParam == 100)
		bHandled = FALSE;

	return 0;
}
