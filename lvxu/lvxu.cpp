#include <combaseapi.h>
#include <uuids.h>
#include <strmif.h>
#include <iostream>

#define __STREAMS__
#include <ks.h>
#include <ksproxy.h>
#include <vidcap.h>

#pragma comment(lib, "strmiids")
#pragma comment(lib, "ksproxy.lib")

using namespace std;

static const GUID LVXU_DEVICE_INFO = { 0x69678ee4, 0x410f, 0x40db, { 0xa8, 0x50, 0x74, 0x20, 0xd7, 0xd8, 0x24, 0xe } };

int main(int argc, const char **argv)
{
	HRESULT hr;

	ICreateDevEnum* pDevEnum = NULL;
	IEnumMoniker *pEnum = NULL;
	IMoniker* pMoniker = NULL;
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
			DWORD node;
			IPropertyBag* pPropertyBag = NULL;
			IKsObject* pKsObject = NULL;

			IBaseFilter* pFilter;
			IKsTopologyInfo* pKsTopologyInfo = NULL;
			DWORD nNodes;
			HANDLE handle;
			KSP_NODE ExtensionProp;
			char Data[sizeof(ExtensionProp)] = { 0 };
			ULONG len = 0;

			hr = pMoniker->BindToObject(pBindCtx, NULL, IID_IBaseFilter, (void**)&pFilter);
			if (FAILED(hr)) continue;
			hr = pFilter->QueryInterface(__uuidof(IKsTopologyInfo), (void**)(&pKsTopologyInfo));
			if (FAILED(hr)) continue;
			hr = pFilter->QueryInterface(__uuidof(IKsObject), (void**)(&pKsObject));
			if (FAILED(hr)) continue;

			nNodes = 0;
			hr = pKsTopologyInfo->get_NumNodes(&nNodes);
			if (FAILED(hr)) continue;

			GUID nodeType;
			for (node = 0; node < nNodes; node++) {
				pKsTopologyInfo->get_NodeType(node, &nodeType);
			}

			handle = pKsObject->KsGetObjectHandle();
			if (!handle) continue;

			ExtensionProp.Property.Set = LVXU_DEVICE_INFO;
			ExtensionProp.Property.Id = 1;
			ExtensionProp.Property.Flags = KSPROPERTY_TYPE_SETSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;
			ExtensionProp.NodeId = 0;


			ExtensionProp.Property.Flags = KSPROPERTY_TYPE_SETSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;
			for (node = 0; node < nNodes; node++) {
				ExtensionProp.Property.Id = node;
				if (SUCCEEDED(KsSynchronousDeviceControl(handle, IOCTL_KS_PROPERTY, &ExtensionProp, sizeof(ExtensionProp), Data, sizeof(Data), &len))) {
					ExtensionProp.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
					hr = KsSynchronousDeviceControl(handle, IOCTL_KS_PROPERTY, &ExtensionProp, sizeof(ExtensionProp), Data, sizeof(Data), &len);
					break;
				}
			}

#if 0
			pMoniker->BindToStorage(pBindCtx, NULL, IID_IPropertyBag, (void**)&pPropertyBag);
#endif
		}
	}

Return:
	if (pBindCtx) pBindCtx->Release();
	CoUninitialize();
    return 0;
}