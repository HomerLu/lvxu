#include <combaseapi.h>
#include <uuids.h>
#include <strmif.h>
#include <iostream>

#define __STREAMS__
#include <ks.h>
#include <ksproxy.h>
#include <vidcap.h>
#include <ksmedia.h>
#include <map>

#pragma comment(lib, "strmiids")
#pragma comment(lib, "ksproxy.lib")

using namespace std;

#if 0
struct XUDESC {
    GUID guidExtensionCode;
    UCHAR bNumControls;
    UCHAR bNrInPins;
    UCHAR baSourceID[1];
    UCHAR bControlSize;
    UCHAR bmControls[2];
};
#endif


static const GUID LVXU_DEVICE_INFO_GUID = {
    0x69678ee4, 0x410f, 0x40db, { 0xa8, 0x50, 0x74, 0x20, 0xd7, 0xd8, 0x24, 0xe },
};
static const GUID LVXU_TEST_GUID = {
    0x1F5D4CA9, 0xDE11, 0x4487, { 0x84, 0x0D, 0x50, 0x93, 0x3C, 0x8E, 0xC8, 0xD1 },
};
static const GUID LVXU_VIDEO_GUID = {
    0x49E40215, 0xF434, 0x47FE, { 0xB1, 0x58, 0x0E, 0x88, 0x50, 0x23, 0xE5, 0x1B },
};
static const GUID LVXU_PERIPHERAL_GUID = {
    0xFFE52D21, 0x8030, 0x4E2C, { 0x82, 0xD9, 0xF5, 0x87, 0xD0, 0x05, 0x40, 0xBD },
};

static const GUID DEV_SPECIFIC = { 0x941C7AC0, 0xC559, 0x11D0, { 0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1} };

int main(int argc, const char **argv)
{
    HRESULT hr;

    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker *pEnum = NULL;
    IMoniker* pMoniker = NULL;
    IBindCtx* pBindCtx = NULL;
    IKsTopologyInfo* pKsTopologyInfo = NULL;
    IKsControl* pKsControl = NULL;
    DWORD nNodes;
    KSP_NODE XU;
    ULONG xuLength = 0;
    ULONG byteReturned = 0;
    ULONG ControlSelector;
    bool read;

    // set by arguments
    ControlSelector = 1;
    read = true;
    XU.Property.Set = LVXU_DEVICE_INFO_GUID;


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
            IBaseFilter* pFilter;
            IKsControl* pKsControl = NULL;
            IKsTopologyInfo* pKsTopologyInfo = NULL;

            hr = pMoniker->BindToObject(pBindCtx, NULL, IID_IBaseFilter, (void**)&pFilter);
            if (FAILED(hr)) continue;
            hr = pFilter->QueryInterface(__uuidof(IKsTopologyInfo), (void**)(&pKsTopologyInfo));
            if (FAILED(hr)) goto Return;
            pFilter->QueryInterface(__uuidof(IKsControl), (void**)(&pKsControl));
            if (FAILED(hr)) goto Return;
            hr = pKsTopologyInfo->get_NumNodes(&nNodes);
            if (FAILED(hr)) goto Return;

            // find NodeID
            XU.Property.Id = KSPROPERTY_EXTENSION_UNIT_INFO;
            XU.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
            for (XU.NodeId = 0; XU.NodeId < nNodes; XU.NodeId++) {
                hr = pKsControl->KsProperty((KSPROPERTY*)&XU, sizeof(XU), NULL, 0, &byteReturned);
                if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
                    hr = S_OK;
                    break;
                }
            }

            // get control length
            XU.Property.Id = ControlSelector;
            XU.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
            hr = pKsControl->KsProperty((KSPROPERTY*)&XU, sizeof(XU), NULL, 0, &xuLength);
            if (hr != HRESULT_FROM_WIN32(ERROR_MORE_DATA)) goto Return;

            if (read) {
                XU.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
            }
            else {
                XU.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
            }

            // nNode = pFilter->pKsTopologyInfo->get_NodeNumber();
            // len, node = pFilter->pKsControl(guid,cs)

            // pKsControl->get
            // pKsControl->set
            pMoniker->Release();
            pMoniker = NULL;
        }
    }

Return:
    if (pKsControl) pKsControl->Release();
    if (pKsTopologyInfo) pKsTopologyInfo->Release();
    if (pBindCtx) pBindCtx->Release();
    if (pMoniker) pMoniker->Release();
    if (pEnum) pEnum->Release();
    CoUninitialize();
    return 0;
}