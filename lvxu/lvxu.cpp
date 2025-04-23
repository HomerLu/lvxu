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

static void Help()
{
    printf("Syntax:\n");
    printf("  LVXU.EXE <xu-name> read <cs>\n");
    printf("  - to read extension unit\n");
    printf("  LVXU.EXE <xu-name> write <cs> <data>\n");
    printf("  - to write extension unit\n");
    printf("Options:\n");
    printf("  --vid <vid>\n");
    printf("  --pid <pid>\n");
    printf("  --intf <interface>\n");
}

static int hexstr2array(const char* ptr, UCHAR* Data)
{
    char tmp[3];
    size_t len = strlen(ptr);
    int cnt = 0;

    if (len & 1) {
        tmp[0] = ptr[0];
        tmp[1] = 0;
        *Data = (UCHAR)strtoul(tmp, NULL, 16);
        len--;
        Data++;
        ptr++;
        cnt++;
    }

    while (len) {
        tmp[0] = ptr[0];
        tmp[1] = ptr[1];
        tmp[2] = 0;
        *Data = (UCHAR)strtoul(tmp, NULL, 16);
        len-=2;
        Data++;
        ptr+=2;
        cnt++;
    }
    return cnt;
}

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
    bool read = false;
    UCHAR Data[64] = { 0 };
    ULONG DataLen = 0;
    int VID = -1;
    int PID = -1;
    int Intf = -1;
    int inState = 0;

    // skip argv[0]
    argc--;
    argv++;

    hr = CoInitialize(0);
    if (FAILED(hr)) {
        return hr;
    }

    while (argc) {
        if (strcmp(argv[0], "--vid") == 0) {
            argc--;
            argv++;
            if (argc == 0) break;
            VID = strtol(argv[0], NULL, 16);
        }
        else if (strcmp(argv[0], "--pid") == 0) {
            argc--;
            argv++;
            if (argc == 0) break;
            PID = strtol(argv[0], NULL, 16);
        }
        else if (strcmp(argv[0], "--intf") == 0) {
            argc--;
            argv++;
            if (argc == 0) break;
            Intf = strtol(argv[0], NULL, 0);
        }
        else if(argv[0][0]=='-') {
            printf("unsupported option: \"%s\"\n", argv[0]);
            Help();
            goto Return;
        }
        else {
            switch (inState) {
            case 0: // GUID
                if (strcmp(argv[0], "infoxu") == 0) {
                    XU.Property.Set = LVXU_DEVICE_INFO_GUID;
                }
                else if (strcmp(argv[0], "testxu") == 0) {
                    XU.Property.Set = LVXU_TEST_GUID;
                }
                else if (strcmp(argv[0], "videoxu") == 0) {
                    XU.Property.Set = LVXU_VIDEO_GUID;
                }
                else if (strcmp(argv[0], "pcxu") == 0) {
                    XU.Property.Set = LVXU_PERIPHERAL_GUID;
                }
                else {
                    printf("syntax error: \"%s\" isn't an XU\n", argv[0]);
                    Help();
                    goto Return;
                }
                break;
            case 1: // read|write
                if (strcmp(argv[0], "read") == 0) {
                    read = true;
                }
                else if (strcmp(argv[0], "write") == 0) {
                    read = false;
                }
                else {
                    printf("syntax error: \"%s\" must be read or write\n", argv[0]);
                    Help();
                    goto Return;
                }
                break;
            case 2: // CS
                ControlSelector = strtol(argv[0], NULL, 0);
                if (ControlSelector == 0) {
                    printf("syntax error: \"%s\" isn't a valid control selector\n", argv[0]);
                    Help();
                    goto Return;
                }
                break;
            }
            inState++;
            if (inState == 3) break;
        }
        argc--;
        argv++;
    }

    if (!read) {
        if (argc <= 0) {
            printf("syntax error: write command, but data doesn't exist\n");
            Help();
            goto Return;
        }
        DataLen = hexstr2array(argv[0], Data);
        argc--;
        argv++;
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
            if (XU.NodeId == nNodes) continue;

            // get control length
            XU.Property.Id = ControlSelector;
            XU.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
            hr = pKsControl->KsProperty((KSPROPERTY*)&XU, sizeof(XU), NULL, 0, &xuLength);
            if (hr != HRESULT_FROM_WIN32(ERROR_MORE_DATA)) goto Return;

            // access data
            if (read) {
                XU.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
                hr = pKsControl->KsProperty((KSPROPERTY*)&XU, sizeof(XU), Data, sizeof(Data), &xuLength);
                if (FAILED(hr)) goto Return;
                for (UWORD i = 0; i < xuLength; i++) {
                    printf("%02X", Data[i]);
                }
                printf("\n");
                break;
            }
            else {
                XU.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
                hr = pKsControl->KsProperty((KSPROPERTY*)&XU, sizeof(XU), Data, sizeof(Data), &xuLength);
                if (FAILED(hr)) goto Return;
            }

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