#include "PluginStdAfx.h"

#include "PluginMimeFilterClient.h"
#include "PluginClient.h"
#include "PluginWbPassThrough.h"


typedef PassthroughAPP::CMetaFactory<PassthroughAPP::CComClassFactoryProtocol,WBPassthru> MetaFactory;


CPluginMimeFilterClient::CPluginMimeFilterClient() : m_classFactory(NULL), m_spCFHTTP(NULL),  m_spCFHTTPS(NULL)
{
  // Should only be called once
  // We register mime filters here
  // Register asynchronous protocol
  CComPtr<IInternetSession> spSession;
  HRESULT hr = ::CoInternetGetSession(0, &spSession, 0);
  if (FAILED(hr) || !spSession)
  {
    DEBUG_ERROR_LOG(hr, PLUGIN_ERROR_SESSION, PLUGIN_ERROR_SESSION_GET_INTERNET_SESSION, "MimeClient::CoInternetGetSession failed");
    return;
  }

  hr = MetaFactory::CreateInstance(CLSID_HttpProtocol, &m_spCFHTTP);
  if (FAILED(hr) || !m_spCFHTTP)
  {
    DEBUG_ERROR_LOG(hr, PLUGIN_ERROR_SESSION, PLUGIN_ERROR_SESSION_CREATE_HTTP_INSTANCE, "MimeClient::CreateInstance failed");
    return;
  }

  hr = spSession->RegisterNameSpace(m_spCFHTTP, CLSID_HttpProtocol, L"http", 0, 0, 0);
  if (FAILED(hr))
  {
    DEBUG_ERROR_LOG(hr, PLUGIN_ERROR_SESSION, PLUGIN_ERROR_SESSION_REGISTER_HTTP_NAMESPACE, "MimeClient::RegisterNameSpace failed");
    return;
  }

  hr = MetaFactory::CreateInstance(CLSID_HttpSProtocol, &m_spCFHTTPS);
  if (FAILED(hr) || !m_spCFHTTPS)
  {
    DEBUG_ERROR_LOG(hr, PLUGIN_ERROR_SESSION, PLUGIN_ERROR_SESSION_CREATE_HTTPS_INSTANCE, "MimeClient::CreateInstance failed");
    return;
  }

  hr = spSession->RegisterNameSpace(m_spCFHTTPS, CLSID_HttpSProtocol, L"https", 0, 0, 0);
  if (FAILED(hr))
  {
    DEBUG_ERROR_LOG(hr, PLUGIN_ERROR_SESSION, PLUGIN_ERROR_SESSION_REGISTER_HTTPS_NAMESPACE, "MimeClient::RegisterNameSpace failed");
    return;
  }

}


CPluginMimeFilterClient::~CPluginMimeFilterClient()
{
  CComPtr<IInternetSession> spSession;

  ::CoInternetGetSession(0, &spSession, 0);
  if (spSession)
  {
    spSession->UnregisterNameSpace(m_spCFHTTP, L"http");
    if (m_spCFHTTP != NULL)
    {
      m_spCFHTTP.Release();
      m_spCFHTTP = NULL;
    }
    spSession->UnregisterNameSpace(m_spCFHTTPS, L"https");
    if (m_spCFHTTPS != NULL)
    {
      m_spCFHTTPS.Release();
      m_spCFHTTPS = NULL;
    }

  }
}

void CPluginMimeFilterClient::Unregister()
{
  CComPtr<IInternetSession> spSession;

  ::CoInternetGetSession(0, &spSession, 0);
  if (spSession)
  {
    spSession->UnregisterNameSpace(m_spCFHTTP, L"http");
    spSession->UnregisterNameSpace(m_spCFHTTPS, L"https");
  }
}
