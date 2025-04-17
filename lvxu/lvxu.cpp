#include <combaseapi.h>
#include <uuids.h>
#include <strmif.h>
#include <iostream>

#define __STREAMS__
#include <ks.h>
#include <ksproxy.h>


#pragma comment(lib, "strmiids")

using namespace std;

int main(int argc, const char **argv)
{
	HRESULT hr;

	ICreateDevEnum* pDevEnum = NULL;
	IEnumMoniker *pEnum = NULL;
	IMoniker* pMoniker = NULL;
	IBaseFilter* pFilter = NULL;
	IPropertyBag* pPropertyBag = NULL;
	IKsObject* pKsObject = NULL;
	IBindCtx* pBindCtx = NULL;

	hr = CoInitialize(0);
	if (FAILED(hr)) {
		return hr;
	}

	hr = CreateBindCtx(NULL, &pBindCtx);
	if (FAILED(hr)) {
		goto Return;
		return hr;
	}

	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDevEnum);
	if (FAILED(hr)) {
		goto Return;
		return hr;
	}

	hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
	if (FAILED(hr)) {
		goto Return;
		return hr;
	}

	if (pEnum) {
		DWORD cFetched;
		while ((hr = pEnum->Next(1, &pMoniker, &cFetched)) == S_OK)
		{
			if (!SUCCEEDED(hr)) continue;
			pMoniker->BindToObject(pBindCtx, NULL, IID_IBaseFilter, (void**)&pFilter);
			if (pFilter) {
				pFilter->QueryInterface(__uuidof(IKsObject), (void**)&pKsObject);
			}
			pMoniker->BindToStorage(pBindCtx, NULL, IID_IPropertyBag, (void**)&pPropertyBag);
		}
	}

Return:
	if (pBindCtx) pBindCtx->Release();
	CoUninitialize();
    return 0;
}