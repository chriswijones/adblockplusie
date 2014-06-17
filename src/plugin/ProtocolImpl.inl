// Copyright 2007 Igor Tandetnik
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PASSTHROUGHAPP_PROTOCOLIMPL_INL
#define PASSTHROUGHAPP_PROTOCOLIMPL_INL

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef PASSTHROUGHAPP_PROTOCOLIMPL_H
#error ProtocolImpl.inl requires ProtocolImpl.h to be included first
#endif

namespace PassthroughAPP
{

  namespace Detail
  {
    template <class T>
    inline HRESULT WINAPI QIPassthrough<T>::
      QueryInterfacePassthroughT(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw)
    {
      ATLASSERT(pv != 0);
      T* pT = static_cast<T*>(pv);

      IUnknown* punkTarget = pT->GetTargetUnknown();
      ATLASSERT(punkTarget != 0);
      if (!punkTarget)
      {
        ATLTRACE(_T("Interface queried before target unknown is set"));
        return E_UNEXPECTED;
      }

      IUnknown* punkWrapper = pT->GetUnknown();

      typename T::ObjectLock lock(pT);
      return QueryInterfacePassthrough(
        pv, riid, ppv, dw, punkTarget, punkWrapper);
    }

    template <class T>
    inline HRESULT WINAPI QIPassthrough<T>::
      QueryInterfaceDebugT(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw)
    {
      ATLASSERT(pv != 0);
      T* pT = static_cast<T*>(pv);

      IUnknown* punkTarget = pT->GetTargetUnknown();
      //	ATLASSERT(punkTarget != 0);
      if (!punkTarget)
      {
        //		ATLTRACE(_T("Interface queried before target unknown is set"));
        return E_UNEXPECTED;
      }

      typename T::ObjectLock lock(pT);

      return QueryInterfaceDebug(pv, riid, ppv, dw, punkTarget);
    }

    inline HRESULT WINAPI QueryInterfacePassthrough(void* pv, REFIID riid,
      LPVOID* ppv, DWORD_PTR dw, IUnknown* punkTarget, IUnknown* punkWrapper)
    {
      ATLASSERT(pv != 0);
      ATLASSERT(ppv != 0);
      ATLASSERT(dw != 0);
      ATLASSERT(punkTarget != 0);


      const PassthroughItfData& data =
        *reinterpret_cast<const PassthroughItfData*>(dw);

      IUnknown** ppUnk = reinterpret_cast<IUnknown**>(
        static_cast<char*>(pv) + data.offsetUnk);

      HRESULT hr = S_OK;
      if (!*ppUnk)
      {
        CComPtr<IUnknown> spUnk;
        hr = punkTarget->QueryInterface(riid,
          reinterpret_cast<void**>(&spUnk));
        ATLASSERT(FAILED(hr) || spUnk != 0);
        if (SUCCEEDED(hr))
        {
          *ppUnk = spUnk.Detach();

          // Need to QI for base interface to fill in base target pointer
          if (data.piidBase)
          {
            ATLASSERT(punkWrapper != 0);
            hr = punkWrapper->QueryInterface(*data.piidBase,
              reinterpret_cast<void**>(&spUnk));
            // since QI for derived interface succeeded,
            // QI for base interface must succeed, too
            ATLASSERT(SUCCEEDED(hr));
          }
        }
      }
      if (SUCCEEDED(hr))
      {
        CComPtr<IUnknown> spItf = reinterpret_cast<IUnknown*>(
          static_cast<char*>(pv) + data.offsetItf);
        *ppv = spItf.Detach();
      }
      else
      {
        ATLASSERT(_T("Interface not supported by target unknown"));
      }
      return hr;
    }

    inline HRESULT WINAPI QueryInterfaceDebug(void* pv, REFIID riid,
      LPVOID* ppv, DWORD_PTR dw, IUnknown* punkTarget)
    {
      ATLASSERT(pv != 0);
      ATLASSERT(ppv != 0);
      ATLASSERT(punkTarget != 0);

      CComPtr<IUnknown> spUnk;
      HRESULT hr = punkTarget->QueryInterface(riid,
        reinterpret_cast<void**>(&spUnk));
      ATLASSERT(FAILED(hr) || spUnk != 0);
      if (SUCCEEDED(hr))
      {
        //Added by MH to display interface CLSID in debug
        OLECHAR wszBuff[39];
        int i = StringFromGUID2(riid, wszBuff, 39);
        USES_CONVERSION;
        ATLTRACE(_T("=====>Unrecognized interface supported by target unknown=%s\n"), OLE2T(wszBuff));
      }

      // We don't support this interface, so return an error.
      // The operations above are for debugging purposes only,
      // this function is not supposed to ever return success
      return E_NOINTERFACE;
    }

    inline HRESULT QueryServicePassthrough(REFGUID guidService,
      IUnknown* punkThis, REFIID riid, void** ppv,
      IServiceProvider* pClientProvider)
    {
      ATLASSERT(punkThis != 0);
      CComPtr<IUnknown> spDummy;
      HRESULT hr = pClientProvider ?
        pClientProvider->QueryService(guidService, riid,
        reinterpret_cast<void**>(&spDummy)) :
      E_NOINTERFACE;
      if (SUCCEEDED(hr))
      {
        hr = punkThis->QueryInterface(riid, ppv);
      }
      return hr;
    }

  } // end namespace PassthroughAPP::Detail

  // ===== IInternetProtocolImpl =====

  inline STDMETHODIMP IInternetProtocolImpl::SetTargetUnknown(
    IUnknown* punkTarget)
  {
    ATLASSERT(punkTarget != 0);
    if (!punkTarget)
    {
      return E_POINTER;
    }

    // This method should only be called once, and be the only source
    // of target interface pointers.
    ATLASSERT(m_spInternetProtocolUnk == 0);
    ATLASSERT(m_spInternetProtocol == 0);
    if (m_spInternetProtocolUnk || m_spInternetProtocol)
    {
      return E_UNEXPECTED;
    }

    // We expect the target unknown to implement at least IInternetProtocol
    // Otherwise we reject it
    HRESULT hr = punkTarget->QueryInterface(&m_spInternetProtocol);
    ATLASSERT(FAILED(hr) || m_spInternetProtocol != 0);
    if (FAILED(hr))
    {
      return hr;
    }

    ATLASSERT(m_spInternetProtocolInfo == 0);
    ATLASSERT(m_spInternetPriority == 0);
    ATLASSERT(m_spInternetThreadSwitch == 0);
    ATLASSERT(m_spWinInetInfo == 0);
    ATLASSERT(m_spWinInetHttpInfo == 0);

    m_spInternetProtocolUnk = punkTarget;
    return S_OK;
  }

  inline void IInternetProtocolImpl::ReleaseAll()
  {
    m_spInternetProtocolUnk.Release();
    m_spInternetProtocol.Release();
    m_spInternetProtocolInfo.Release();
    m_spInternetPriority.Release();
    m_spInternetThreadSwitch.Release();
    m_spWinInetInfo.Release();
    m_spWinInetHttpInfo.Release();
  }

  // IInternetProtocolRoot
  inline STDMETHODIMP IInternetProtocolImpl::Start(
    /* [in] */ LPCWSTR szUrl,
    /* [in] */ IInternetProtocolSink *pOIProtSink,
    /* [in] */ IInternetBindInfo *pOIBindInfo,
    /* [in] */ DWORD grfPI,
    /* [in] */ HANDLE_PTR dwReserved)
  {
    ATLASSERT(m_spInternetProtocol != 0);
    return m_spInternetProtocol ?
      m_spInternetProtocol->Start(szUrl, pOIProtSink, pOIBindInfo, grfPI,
      dwReserved) :
    E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::Continue(
    /* [in] */ PROTOCOLDATA *pProtocolData)
  {
    ATLASSERT(m_spInternetProtocol != 0);
    return m_spInternetProtocol ?
      m_spInternetProtocol->Continue(pProtocolData) :
      E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::Abort(
    /* [in] */ HRESULT hrReason,
    /* [in] */ DWORD dwOptions)
  {
    ATLASSERT(m_spInternetProtocol != 0);
    return m_spInternetProtocol ?
      m_spInternetProtocol->Abort(hrReason, dwOptions) :
      E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::Terminate(
    /* [in] */ DWORD dwOptions)
  {
    ATLASSERT(m_spInternetProtocol != 0);
    return m_spInternetProtocol ?
      m_spInternetProtocol->Terminate(dwOptions) :
      E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::Suspend()
  {
    ATLASSERT(m_spInternetProtocol != 0);
    return m_spInternetProtocol ?
      m_spInternetProtocol->Suspend() :
      E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::Resume()
  {
    ATLASSERT(m_spInternetProtocol != 0);
    return m_spInternetProtocol ?
      m_spInternetProtocol->Resume() :
      E_UNEXPECTED;
  }

  // IInternetProtocol
  inline STDMETHODIMP IInternetProtocolImpl::Read(
    /* [in, out] */ void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbRead)
  {
    ATLASSERT(m_spInternetProtocol != 0);
    return m_spInternetProtocol ?
      m_spInternetProtocol->Read(pv, cb, pcbRead) :
      E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::Seek(
    /* [in] */ LARGE_INTEGER dlibMove,
    /* [in] */ DWORD dwOrigin,
    /* [out] */ ULARGE_INTEGER *plibNewPosition)
  {
    ATLASSERT(m_spInternetProtocol != 0);
    return m_spInternetProtocol ?
      m_spInternetProtocol->Seek(dlibMove, dwOrigin, plibNewPosition) :
      E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::LockRequest(
    /* [in] */ DWORD dwOptions)
  {
    ATLASSERT(m_spInternetProtocol != 0);
    return m_spInternetProtocol ?
      m_spInternetProtocol->LockRequest(dwOptions) :
      E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::UnlockRequest()
  {
    ATLASSERT(m_spInternetProtocol != 0);
    return m_spInternetProtocol ?
      m_spInternetProtocol->UnlockRequest() :
      E_UNEXPECTED;
  }

  // IInternetProtocolInfo
  inline STDMETHODIMP IInternetProtocolImpl::ParseUrl(
    /* [in] */ LPCWSTR pwzUrl,
    /* [in] */ PARSEACTION ParseAction,
    /* [in] */ DWORD dwParseFlags,
    /* [out] */ LPWSTR pwzResult,
    /* [in] */ DWORD cchResult,
    /* [out] */ DWORD *pcchResult,
    /* [in] */ DWORD dwReserved)
  {
    ATLASSERT(m_spInternetProtocolInfo != 0);
    return m_spInternetProtocolInfo ?
      m_spInternetProtocolInfo->ParseUrl(pwzUrl, ParseAction, dwParseFlags,
      pwzResult, cchResult, pcchResult, dwReserved) :
    E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::CombineUrl(
    /* [in] */ LPCWSTR pwzBaseUrl,
    /* [in] */ LPCWSTR pwzRelativeUrl,
    /* [in] */ DWORD dwCombineFlags,
    /* [out] */ LPWSTR pwzResult,
    /* [in] */ DWORD cchResult,
    /* [out] */ DWORD *pcchResult,
    /* [in] */ DWORD dwReserved)
  {
    ATLASSERT(m_spInternetProtocolInfo != 0);
    return m_spInternetProtocolInfo ?
      m_spInternetProtocolInfo->CombineUrl(pwzBaseUrl, pwzRelativeUrl,
      dwCombineFlags, pwzResult, cchResult, pcchResult, dwReserved) :
    E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::CompareUrl(
    /* [in] */ LPCWSTR pwzUrl1,
    /* [in] */ LPCWSTR pwzUrl2,
    /* [in] */ DWORD dwCompareFlags)
  {
    ATLASSERT(m_spInternetProtocolInfo != 0);
    return m_spInternetProtocolInfo ?
      m_spInternetProtocolInfo->CompareUrl(pwzUrl1,pwzUrl2, dwCompareFlags) :
      E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::QueryInfo(
    /* [in] */ LPCWSTR pwzUrl,
    /* [in] */ QUERYOPTION QueryOption,
    /* [in] */ DWORD dwQueryFlags,
    /* [in, out] */ LPVOID pBuffer,
    /* [in] */ DWORD cbBuffer,
    /* [in, out] */ DWORD *pcbBuf,
    /* [in] */ DWORD dwReserved)
  {
    ATLASSERT(m_spInternetProtocolInfo != 0);
    return m_spInternetProtocolInfo ?
      m_spInternetProtocolInfo->QueryInfo(pwzUrl, QueryOption, dwQueryFlags,
      pBuffer, cbBuffer, pcbBuf, dwReserved) :
    E_UNEXPECTED;
  }

  // IInternetPriority
  inline STDMETHODIMP IInternetProtocolImpl::SetPriority(
    /* [in] */ LONG nPriority)
  {
    ATLASSERT(m_spInternetPriority != 0);
    return m_spInternetPriority ?
      m_spInternetPriority->SetPriority(nPriority) :
      E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::GetPriority(
    /* [out] */ LONG *pnPriority)
  {
    ATLASSERT(m_spInternetPriority != 0);
    return m_spInternetPriority ?
      m_spInternetPriority->GetPriority(pnPriority) :
      E_UNEXPECTED;
  }

  // IInternetThreadSwitch
  inline STDMETHODIMP IInternetProtocolImpl::Prepare()
  {
    ATLASSERT(m_spInternetThreadSwitch != 0);
    return m_spInternetThreadSwitch ?
      m_spInternetThreadSwitch->Prepare() :
      E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolImpl::Continue()
  {
    ATLASSERT(m_spInternetThreadSwitch != 0);
    return m_spInternetThreadSwitch ?
      m_spInternetThreadSwitch->Continue() :
      E_UNEXPECTED;
  }

  // IWinInetInfo
  inline STDMETHODIMP IInternetProtocolImpl::QueryOption(
    /* [in] */ DWORD dwOption,
    /* [in, out] */ LPVOID pBuffer,
    /* [in, out] */ DWORD *pcbBuf)
  {
    ATLASSERT(m_spWinInetInfo != 0);
    return m_spWinInetInfo ?
      m_spWinInetInfo->QueryOption(dwOption, pBuffer, pcbBuf) :
      E_UNEXPECTED;
  }

  // IWinInetHttpInfo
  inline STDMETHODIMP IInternetProtocolImpl::QueryInfo(
    /* [in] */ DWORD dwOption,
    /* [in, out] */ LPVOID pBuffer,
    /* [in, out] */ DWORD *pcbBuf,
    /* [in, out] */ DWORD *pdwFlags,
    /* [in, out] */ DWORD *pdwReserved)
  {
    ATLASSERT(m_spWinInetHttpInfo  != 0);
    return m_spWinInetHttpInfo ?
      m_spWinInetHttpInfo->QueryInfo(dwOption, pBuffer, pcbBuf, pdwFlags,
      pdwReserved) :
    E_UNEXPECTED;
  }

  // ===== IInternetProtocolSinkImpl =====

  inline HRESULT IInternetProtocolSinkImpl::OnStart(LPCWSTR szUrl,
    IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
    DWORD grfPI, HANDLE_PTR dwReserved, IInternetProtocol* pTargetProtocol)
  {
    ATLASSERT(pOIProtSink != 0);
    ATLASSERT(pOIBindInfo != 0);
    ATLASSERT(pTargetProtocol != 0);
    if (!pOIProtSink || !pOIBindInfo || !pTargetProtocol)
    {
      return E_POINTER;
    }

    // This method should only be called once, and be the only source
    // of target interface pointers.
    ATLASSERT(m_spInternetProtocolSink == 0);
    ATLASSERT(m_spTargetProtocol == 0);
    if (m_spInternetProtocolSink || m_spTargetProtocol)
    {
      return E_UNEXPECTED;
    }

    ATLASSERT(m_spServiceProvider == 0);

    m_spInternetProtocolSink = pOIProtSink;
    m_spTargetProtocol = pTargetProtocol;
    return S_OK;
  }

  inline void IInternetProtocolSinkImpl::ReleaseAll()
  {
    m_spInternetProtocolSink.Release();
    m_spServiceProvider.Release();
    m_spTargetProtocol.Release();
  }

  inline IServiceProvider* IInternetProtocolSinkImpl::GetClientServiceProvider()
  {
    return m_spServiceProvider;
  }

  inline HRESULT IInternetProtocolSinkImpl::QueryServiceFromClient(
    REFGUID guidService, REFIID riid, void** ppvObject)
  {
    HRESULT hr = S_OK;
    CComPtr<IServiceProvider> spClientProvider = m_spServiceProvider;
    if (!spClientProvider)
    {
      hr = m_spInternetProtocolSink->QueryInterface(&spClientProvider);
      ATLASSERT(SUCCEEDED(hr) && spClientProvider != 0);
    }
    if (SUCCEEDED(hr))
    {
      hr = spClientProvider->QueryService(guidService, riid, ppvObject);
    }
    return hr;
  }

  // IInternetProtocolSink
  inline STDMETHODIMP IInternetProtocolSinkImpl::Switch(
    /* [in] */ PROTOCOLDATA *pProtocolData)
  {
    ATLASSERT(m_spInternetProtocolSink != 0);
    return m_spInternetProtocolSink ?
      m_spInternetProtocolSink->Switch(pProtocolData) :
      E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolSinkImpl::ReportProgress(
    /* [in] */ ULONG ulStatusCode,
    /* [in] */ LPCWSTR szStatusText)
  {
    ATLASSERT(m_spInternetProtocolSink != 0);
    return m_spInternetProtocolSink ?
      m_spInternetProtocolSink->ReportProgress(ulStatusCode, szStatusText) :
      E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolSinkImpl::ReportData(
    /* [in] */ DWORD grfBSCF,
    /* [in] */ ULONG ulProgress,
    /* [in] */ ULONG ulProgressMax)
  {
    ATLASSERT(m_spInternetProtocolSink != 0);
    return m_spInternetProtocolSink ?
      m_spInternetProtocolSink->ReportData(grfBSCF, ulProgress,
      ulProgressMax) :
    E_UNEXPECTED;
  }

  inline STDMETHODIMP IInternetProtocolSinkImpl::ReportResult(
    /* [in] */ HRESULT hrResult,
    /* [in] */ DWORD dwError,
    /* [in] */ LPCWSTR szResult)
  {
    ATLASSERT(m_spInternetProtocolSink != 0);

    // On some websites there are broken requests, that keep on redirecting without a stop.
    // For example: http://telegrafua.com/ has a request to: http://www.ool.ua/webroot/delivery/images/ool-button.gif
    // which causes IE tab to crash in urlmon.dll. That request doesn't ever resolve, since it's constantly responding 
    // with HTTP 302. 

    // It seems like WinInet (urlmon.dll) doesn't expect anything like that to happen, and relies on APP to make sure it doesn't.
    // So here we detect if there has been too many redirects (ie INET_E_REDIRECT_FAILED) and we just notify WinInet
    // that there has been a generic problem, not redirect specific error. 

      // This seem to only postpone the issue to some different place. Undo for now
//    if (hrResult == INET_E_REDIRECT_FAILED)
//      hrResult = S_FALSE;

    return m_spInternetProtocolSink ?
      m_spInternetProtocolSink->ReportResult(hrResult, dwError, szResult) :
      E_UNEXPECTED;
  }

  // IServiceProvider
  inline STDMETHODIMP IInternetProtocolSinkImpl::QueryService(
    /* [in] */ REFGUID guidService,
    /* [in] */ REFIID riid,
    /* [out] */ void** ppvObject)
  {
    ATLASSERT(m_spServiceProvider != 0);
    return m_spServiceProvider ?
      m_spServiceProvider->QueryService(guidService, riid, ppvObject) :
      E_UNEXPECTED;
  }

  // ===== CInternetProtocolSinkWithSP =====

  template <class T, class ThreadModel>
  inline HRESULT CInternetProtocolSinkWithSP<T, ThreadModel>::OnStart(
    LPCWSTR szUrl, IInternetProtocolSink *pOIProtSink,
    IInternetBindInfo *pOIBindInfo,	DWORD grfPI, HANDLE_PTR dwReserved,
    IInternetProtocol* pTargetProtocol)
  {
    ATLASSERT(m_spServiceProvider == 0);
    if (m_spServiceProvider)
    {
      return E_UNEXPECTED;
    }
    HRESULT hr = BaseClass::OnStart(szUrl, pOIProtSink, pOIBindInfo, grfPI,
      dwReserved, pTargetProtocol);
    if (SUCCEEDED(hr))
    {
      pOIProtSink->QueryInterface(&m_spServiceProvider);
    }
    return hr;
  }

  template <class T, class ThreadModel>
  inline HRESULT CInternetProtocolSinkWithSP<T, ThreadModel>::
    _InternalQueryService(REFGUID guidService, REFIID riid, void** ppvObject)
  {
    return E_NOINTERFACE;
  }

  template <class T, class ThreadModel>
  inline STDMETHODIMP CInternetProtocolSinkWithSP<T, ThreadModel>::QueryService(
    REFGUID guidService, REFIID riid, void** ppv)
  {
    T* pT = static_cast<T*>(this);
    HRESULT hr = pT->_InternalQueryService(guidService, riid, ppv);
    if (FAILED(hr) && m_spServiceProvider)
    {
      hr = m_spServiceProvider->QueryService(guidService, riid, ppv);
    }
    return hr;
  }
} // end namespace PassthroughAPP

#endif // PASSTHROUGHAPP_PROTOCOLIMPL_INL
