#include <combaseapi.h>
#include <uuids.h>
#include <strmif.h>
#include <iostream>

#define __STREAMS__
#include <ks.h>
#include <ksproxy.h>
#include <vidcap.h>

#pragma comment(lib, "strmiids")

using namespace std;


#if 0
luvc_WriteEx(
	hFW = fwi,
	xu = unit,
	Selector = dwAddress,
	pdata = pbValue,
	szLength = bSize,
	pReturnBufferSize = 0);
Device d(hFW)

DevSpecificWriteDevice(
	xu = xu,
	Selector = Selector,
	data = pdata,
	sz = szLength,
	ret = pReturnBufferSize)

	GenericAccessProperty(
		pGuid = pInterface(xu),
		Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY,
		Selector = Selector,
		data = data,
		data_length = sz,
		dwreturned = ret);
KSP_NODE ExtensionProp = {
  Set = pGuid,
  Id = Selector,
  Flags = Flags,
  NodeId = pGuid.second // how?????
}

AccessProperty(
	pNode = (KSPROPERTY*)&ExtensionProp,
	szNode = sizeof(KSP_NODE),
	data = data,
	data_length = data_length,
	dwreturned = &transfer)

	ksdispatch::ksproperty(
		handle = hDevice = m_KsObject->KsGetObjectHandle(),
		Property = (LPVOID)pNode,
		PropertyLength = szNode,
		PropertyData = data,
		DataLength = data_length,
		BytesReturned = dwreturned);

KsSynchronousDeviceControl( // KsProxy.h API
	Handle = handle,
	IoControl = IOCTL_KS_PROPERTY,
	InBuffer = Property,
	InLength = PropertyLength,
	OutBuffer = PropertyData,
	OutLength = DataLength,
	BytesReturned = BytesReturned
#endif






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
			int node;
			IPropertyBag* pPropertyBag = NULL;
			IKsObject* pKsObject = NULL;

			IBaseFilter* pFilter;
			IKsTopologyInfo* pKsTopologyInfo = NULL;
			DWORD nNodes;
			HANDLE handle;
			KSP_NODE ExtensionProp;

			hr = pMoniker->BindToObject(pBindCtx, NULL, IID_IBaseFilter, (void**)&pFilter);
			if (FAILED(hr)) continue;
			hr = pFilter->QueryInterface(__uuidof(IKsTopologyInfo), (void**)(&pKsTopologyInfo));
			if (FAILED(hr)) continue;
			hr = pFilter->QueryInterface(__uuidof(IKsObject), (void**)(&pKsObject));
			if (FAILED(hr)) continue;


			nNodes = 0;
			hr = pKsTopologyInfo->get_NumNodes(&nNodes);
			if (FAILED(hr)) continue;

			for (node = 0; node < nNodes; node++) {
				GUID nodeType;
				pKsTopologyInfo->get_NodeType(node, &nodeType);
			}

#if 0

			handle = pKsObject->KsGetObjectHandle();
			if (!handle) continue;

			KsSynchronousDeviceControl(handle, IOCTL_KS_PROPERTY, )



			ExtensionProp.Property.Set = GUID;
			ExtensionProp.Property.Id = 1;
			ExtensionProp.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
			ExtensionProp.NodeId; = ? ? ? ;

			KsSynchronousDeviceControl(handle, IOCTL_KS_PROPERTY, );
			pMoniker->BindToStorage(pBindCtx, NULL, IID_IPropertyBag, (void**)&pPropertyBag);
#endif
		}
	}

Return:
	if (pBindCtx) pBindCtx->Release();
	CoUninitialize();
    return 0;
}