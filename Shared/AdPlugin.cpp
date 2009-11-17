// This is part of AdPlugin, a Browser Helper Object for Internet Explorer
// This file contains the basic DLL entry points. 

#include "AdPluginStdAfx.h"

#if (defined PRODUCT_ADBLOCKER)
 #include "../AdBlocker/AdBlocker.h"
 #include "../AdBlocker/AdBlocker_i.c"
#elif (defined PRODUCT_DOWNLOADHELPER)
 #include "../DownloadHelper/DownloadHelper.h"
 #include "../DownloadHelper/DownloadHelper_i.c"
#endif

#include "AdPluginClass.h"
#include "AdPluginClient.h"
#include "AdPluginSystem.h"
#include "AdPluginSettings.h"
#include "AdPluginIniFile.h"
#include "AdPluginDictionary.h"
#ifdef SUPPORT_FILTER
 #include "AdPluginFilterClass.h"
#endif
#ifdef SUPPORT_CONFIG
 #include "AdPluginConfig.h"
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_AdPluginClass, CPluginClass)
END_OBJECT_MAP()


class CPluginApp : public CWinApp
{

public:

	CPluginApp();

	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CPluginApp, CWinApp)
END_MESSAGE_MAP()



CPluginApp theApp;

CPluginApp::CPluginApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

BOOL CPluginApp::InitInstance()
{
	TCHAR szFilename[MAX_PATH];
	GetModuleFileName(NULL, szFilename, MAX_PATH);
	_tcslwr_s(szFilename);

	if (_tcsstr(szFilename, _T("explorer.exe")))
	{
		return FALSE;
	}

    _Module.Init(ObjectMap, AfxGetInstanceHandle(), &LIBID_AdPluginLib);

	CWinApp::InitInstance();

	return TRUE;
}


STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)
{
	return _Module.RegisterServer(TRUE);
}

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}

void InitPlugin(bool isInstall)
{
	CPluginSystem* system = CPluginSystem::GetInstance();

    CPluginSettings* settings = CPluginSettings::GetInstance();

    settings->EraseTab();

    settings->Remove(SETTING_PLUGIN_SELFTEST);
	settings->SetValue(SETTING_PLUGIN_INFO_PANEL, isInstall ? 1 : 2);
    settings->Write();

	if (isInstall)
	{
		DEBUG_GENERAL(
			L"================================================================================\nINSTALLER " + 
			CString(IEPLUGIN_VERSION) + 
			L"\n================================================================================")
	}
	else
	{
		DEBUG_GENERAL(
			L"================================================================================\nUPDATER " + 
			CString(IEPLUGIN_VERSION) + L" (UPDATED FROM " + settings->GetString(SETTING_PLUGIN_VERSION) + L")"
			L"\n================================================================================")
	}

    // Create default filters
#ifdef SUPPORT_FILTER
    DEBUG_GENERAL(L"*** Generating default filters")
    CPluginFilter::CreateFilters();
#endif

    // Create default dictionary
    CPluginDictionary::GetInstance();   

    // Create default config file
#ifdef SUPPORT_CONFIG
    DEBUG_GENERAL("*** Generating config file")
    CPluginConfig::GetInstance();
#endif

	HKEY hKey = NULL;
	DWORD dwDisposition = 0;

	DWORD dwResult = ::RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\IE Download Helper", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &dwDisposition);
	if (dwResult == ERROR_SUCCESS)
	{
		CString pluginId = system->GetPluginId();

		::RegSetValueEx(hKey, L"PluginId", 0, REG_SZ, (const BYTE*)pluginId.GetBuffer(), 2*(pluginId.GetLength() + 1));

		::RegCloseKey(hKey);
	}
/*
    // Create uninstall ini file
	char path[MAX_PATH];

	if (::SHGetSpecialFolderPathA(NULL, path, CSIDL_WINDOWS, TRUE))
	{
	    if (::PathAppendA(path, UNINSTALL_INI_FILE))
	    {
	        CPluginIniFile iniFile(path);

            CPluginIniFile::TSectionData data;
            data.insert(std::make_pair("pluginid", system->GetPluginId()));

            iniFile.UpdateSection("Settings", data);
            if (!iniFile.Write())
            {
                DEBUG_ERROR_LOG(iniFile.GetLastError(), PLUGIN_ERROR_INSTALL, PLUGIN_ERROR_INSTALL_CREATE_INI_FILE, "Install::Create init file")
            }
        }
        else
        {
            DEBUG_ERROR_LOG(::GetLastError(), PLUGIN_ERROR_INSTALL, PLUGIN_ERROR_INSTALL_APPEND_PATH, "Install::Append path")
        }
	}
	else
	{
		DEBUG_ERROR_LOG(::GetLastError(), PLUGIN_ERROR_INSTALL, PLUGIN_ERROR_INSTALL_GET_WINDOWS_PATH, "Install::SHGetSpecialFolderPathA failed");
	}
*/
    // Post async plugin error
    CPluginError pluginError;
    while (CPluginClient::PopFirstPluginError(pluginError))
    {
        CPluginClient::LogPluginError(pluginError.GetErrorCode(), pluginError.GetErrorId(), pluginError.GetErrorSubid(), pluginError.GetErrorDescription(), true, pluginError.GetProcessId(), pluginError.GetThreadId());
    }
}

// Called from installer
EXTERN_C void STDAPICALLTYPE OnInstall(void)
{
	InitPlugin(true);
}

// Called from updater
EXTERN_C void STDAPICALLTYPE OnUpdate(void)
{
	InitPlugin(false);
}