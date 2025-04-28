#include <combaseapi.h>
#include <uuids.h>
#include <strmif.h>
#include <iostream>

#define __STREAMS__
#include <ks.h>
#include <ksproxy.h>
#include <vidcap.h>
#include <ksmedia.h>
#include <cfgmgr32.h>

#pragma comment(lib, "strmiids")
#pragma comment(lib, "ksproxy.lib")
#pragma comment(lib, "cfgmgr32.lib")


struct GUID_NAME {
    const char* name;
    GUID guid;
};

#define NUM_GUID_TABLE 6
GUID_NAME guidTable[NUM_GUID_TABLE] = {
    {"infoxu",      {0x69678ee4, 0x410f, 0x40db, { 0xa8, 0x50, 0x74, 0x20, 0xd7, 0xd8, 0x24, 0x0e},},},
    {"testxu",      {0x1F5D4CA9, 0xDE11, 0x4487, { 0x84, 0x0D, 0x50, 0x93, 0x3C, 0x8E, 0xC8, 0xD1},},},
    {"videoxu",     {0x49E40215, 0xF434, 0x47FE, { 0xB1, 0x58, 0x0E, 0x88, 0x50, 0x23, 0xE5, 0x1B},},},
    {"pcxu",        {0xFFE52D21, 0x8030, 0x4E2C, { 0x82, 0xD9, 0xF5, 0x87, 0xD0, 0x05, 0x40, 0xBD},},},
    {"settings",    {0x2b02cf13, 0x77bd, 0x47a5, { 0x8f, 0x98, 0x08, 0x51, 0xa7, 0xa7, 0x68, 0x0c},},},
    {"metadata",    {0x1607f965, 0x608f, 0x448c, { 0xa8, 0xb6, 0xaf, 0x39, 0xc1, 0xc8, 0x84, 0x24},},},
};

static bool ParseGUID(const char* str, GUID* guid)
{
    USHORT data41;
    ULONGLONG data42;
    char* start = (char*)str;
    char* end;
    // 36fc9e60-c465-11cf-8056-444553540000
    guid->Data1 = strtoul(start, &end, 16);
    if (end - start != 8) return false;
    if (*end != '-') return false;

    start = end + 1;
    guid->Data2 = (USHORT)strtoul(start, &end, 16);
    if (end - start != 4) return false;
    if (*end != '-') return false;
    start = end + 1;

    start = end + 1;
    guid->Data3 = (USHORT)strtoul(start, &end, 16);
    if (end - start != 4) return false;
    if (*end != '-') return false;

    start = end + 1;
    data41 = (USHORT)strtoul(start, &end, 16);
    if (end - start != 4) return false;
    if (*end != '-') return false;

    start = end + 1;
    data42 = (ULONGLONG)strtoull(start, &end, 16);
    if (end - start != 12) return false;
    if (*end != '\0') return false;

    guid->Data4[0] = (UCHAR)(data41 >> 8);
    guid->Data4[1] = (UCHAR)data41;
    guid->Data4[2] = (UCHAR)(data42 >> 40);
    guid->Data4[3] = (UCHAR)(data42 >> 32);
    guid->Data4[4] = (UCHAR)(data42 >> 24);
    guid->Data4[5] = (UCHAR)(data42 >> 16);
    guid->Data4[6] = (UCHAR)(data42 >> 8);
    guid->Data4[7] = (UCHAR)data42;
    return true;
}

char *guid2str(GUID guid, char* str, int len)
{
    // {0x1607f965, 0x608f, 0x448c, { 0xa8, 0xb6, 0xaf, 0x39, 0xc1, 0xc8, 0x84, 0x24}
    sprintf_s(str, len, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        guid.Data1,
        guid.Data2,
        guid.Data3,
        guid.Data4[0], guid.Data4[1],
        guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return str;
}

static void Help()
{
    printf("Syntax:\n");
    printf("  LVXU.EXE <xu-name|guid> read <cs>\n");
    printf("  - to read extension unit\n");
    printf("  LVXU.EXE <xu-name|guid> write <cs> <data>\n");
    printf("  - to write extension unit\n");
    printf("<xu-name>:\n");

    for (int i = 0; i < NUM_GUID_TABLE; i++) {
        char str[64];
        printf("  %-9s %s\n", guidTable[i].name, guid2str(guidTable[i].guid, str, sizeof(str)));
    }

    printf("<cs>:\n");
    printf("  Can be in decimal or hexadecimal (start with 0x)\n");
    printf("<data>:\n");
    printf("  Must be in hexadecimal. Numbers that do not begin with 0x are also considered hexadecimal.\n");
    printf("\n");
    printf("Options:\n");
    printf("  --vid <vid>\n");
    printf("  --pid <pid>\n");
    printf("  --intf <interface>\n");
    printf("\n");
    printf("Example:\n");
    printf("  LVXU.EXE infoxu read 1\n");
    printf("  LVXU.EXE pcxu write 17 0xDF\n");
    printf("  LVXU.EXE FFE52D21-8030-4E2C-82D9-F587D00540BD write 0x11 DF\n");
}

static int hexstr2array(const char* ptr, UCHAR* Data)
{
    char tmp[3] = {0};
    size_t len = strlen(ptr);
    int cnt = 0;

    if (ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X')) {
        ptr += 2;
        len -= 2;
    }

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
        ptr += 2;
        len -= 2;
        Data++;
        cnt++;
    }
    return cnt;
}

static DWORD GetNodeNumber(IBaseFilter *pFilter)
{
    DWORD nNodes = 0;
    HRESULT hr;
    IKsTopologyInfo* pKsTopologyInfo = NULL;

    hr = pFilter->QueryInterface(__uuidof(IKsTopologyInfo), (void**)(&pKsTopologyInfo));
    if (FAILED(hr)) goto Return;
    hr = pKsTopologyInfo->get_NumNodes(&nNodes);
    if (FAILED(hr)) goto Return;
Return:
    pKsTopologyInfo->Release();
    return nNodes;
}

static int ParseArguments(int argc, const char** argv, int& VID, int& PID, int& Intf, const char*& xuName, GUID& guid, bool& write, ULONG& ControlSelector, UCHAR *Data, ULONG &DataLen)
{
    int inState = 0;

    VID = -1;
    PID = -1;
    Intf = -1;
    DataLen = 0;

    while (argc) {
        if (strcmp(argv[0], "--vid") == 0) {
            argc--;
            argv++;
            if (argc == 0) {
                printf("syntax error!\n");
                return false;
            }
            VID = strtol(argv[0], NULL, 16);
        }
        else if (strcmp(argv[0], "--pid") == 0) {
            argc--;
            argv++;
            if (argc == 0) {
                printf("syntax error!\n");
                return false;
            }
            PID = strtol(argv[0], NULL, 16);
        }
        else if (strcmp(argv[0], "--intf") == 0) {
            argc--;
            argv++;
            if (argc == 0) {
                printf("syntax error!\n");
                return false;
            }
            Intf = strtol(argv[0], NULL, 0);
        }
        else if (argv[0][0] == '-') {
            printf("unsupported option: \"%s\"\n", argv[0]);
            return false;
        }
        else {
            switch (inState) {
            case 0: // GUID
                for (int i = 0; i < NUM_GUID_TABLE; i++) {
                    if (strcmp(argv[0], guidTable[i].name) == 0) {
                        guid = guidTable[i].guid;
                        xuName = argv[0];
                        break;
                    }
                }
                if (xuName == NULL) {
                    if (ParseGUID(argv[0], &guid)) {
                        int i;
                        for (i = 0; i < NUM_GUID_TABLE; i++) {
                            if (IsEqualGUID(guid, guidTable[i].guid)) {
                                xuName = guidTable[i].name;
                                break;
                            }
                        }
                        if (i == NUM_GUID_TABLE) {
                            xuName = argv[0];
                        }
                    }
                    else {
                        printf("syntax error: \"%s\" isn't an XU\n", argv[0]);
                        return false;
                    }
                }
                break;
            case 1: // read|write
                if (strcmp(argv[0], "write") == 0) {
                    write = true;
                }
                else if (strcmp(argv[0], "read") == 0) {
                    write = false;
                }
                else {
                    printf("syntax error: the 3rd parameter (%s) must be \"read\" or \"write\"\n", argv[0]);
                    return false;
                }
                break;
            case 2: // CS
                ControlSelector = strtol(argv[0], NULL, 0);
                if (ControlSelector == 0) {
                    printf("syntax error: \"%s\" isn't a valid control selector number\n", argv[0]);
                    return false;
                }
                break;
            case 3:
                if (write) {
                    DataLen = hexstr2array(argv[0], Data);
                }
                break;
            }
            inState++;
        }
        argc--;
        argv++;
    }

    if (write && inState != 4 || !write && inState != 3) {
        printf("syntax error!\n");
        return false;
    }

    return true;
}


int main(int argc, const char **argv)
{
    HRESULT hr;

    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker *pEnum = NULL;
    IMoniker* pMoniker = NULL;
    IBindCtx* pBindCtx = NULL;
    DWORD nNodes;
    KSP_NODE XU = { 0 };
    ULONG xuLength = 0;
    ULONG byteReturned = 0;
    ULONG ControlSelector;
    bool write = false;
    bool done = false;
    UCHAR Data[512] = { 0 };
    ULONG DataLen = 0;
    int VID = -1;
    int PID = -1;
    int Intf = -1;
    int inState = 0;
    const char* xuName = NULL;

    // skip argv[0]
    argc--;
    argv++;

    hr = CoInitialize(0);
    if (FAILED(hr)) {
        return hr;
    }

    if (argc==0 || !ParseArguments(argc, argv, VID, PID, Intf, xuName, XU.Property.Set, write, ControlSelector, Data, DataLen)) {
        Help();
        goto Return;
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
        while (!done && (hr = pEnum->Next(1, &pMoniker, &cFetched)) == S_OK)
        {
            LPOLESTR wszDisplayName = NULL;
            IBaseFilter* pFilter = NULL;
            IKsControl* pKsControl = NULL;

            // parse DisplayName to get VID, PID, Interface
            hr = pMoniker->GetDisplayName(pBindCtx, NULL, &wszDisplayName);
            if (FAILED(hr)) goto Next;
            // @device:pnp:\\?\usb#vid_046d&pid_094f&mi_00#6&2c308b75&4&0000#{65e8773d-8f56-11d0-a3b9-00a0c9223196}\global
            WCHAR* wstr;
            if (VID != -1) {
                wstr = wcsstr(wszDisplayName, L"#vid_");
                if (!wstr) goto Next;
                if (wcstoul(wstr + 5, NULL, 16) != VID) goto Next;
            }
            if (PID != -1) {
                wstr = wcsstr(wszDisplayName, L"&pid_");
                if (!wstr) goto Next;
                if (wcstoul(wstr + 5, NULL, 16) != PID) goto Next;
            }
            if (Intf != -1) {
                wstr = wcsstr(wszDisplayName, L"&mi_");
                if (!wstr) goto Next;
                if (wcstoul(wstr + 4, NULL, 16) != Intf) goto Next;
            }

            hr = pMoniker->BindToObject(pBindCtx, NULL, __uuidof(IBaseFilter), (void**)&pFilter);
            if (FAILED(hr)) goto Next;

            hr = pFilter->QueryInterface(__uuidof(IKsControl), (void**)(&pKsControl));
            if (FAILED(hr)) goto Next;

            // find NodeID
            nNodes = GetNodeNumber(pFilter);
            XU.Property.Id = KSPROPERTY_EXTENSION_UNIT_INFO;
            XU.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
            for (XU.NodeId = 0; XU.NodeId < nNodes; XU.NodeId++) {
                hr = pKsControl->KsProperty((KSPROPERTY*)&XU, sizeof(XU), NULL, 0, &byteReturned);
                if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
                    hr = S_OK;
                    break;
                }
            }
            if (XU.NodeId == nNodes) goto Next;

            // get control length
            XU.Property.Id = ControlSelector;
            XU.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
            hr = pKsControl->KsProperty((KSPROPERTY*)&XU, sizeof(XU), NULL, 0, &xuLength);
            if (hr != HRESULT_FROM_WIN32(ERROR_MORE_DATA)) goto Next;

            // access data
            if (write) {
                XU.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
                hr = pKsControl->KsProperty((KSPROPERTY*)&XU, sizeof(XU), Data, sizeof(Data), &xuLength);
                if (SUCCEEDED(hr)) {
                    done = true;
                }
            }
            else {
                XU.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
                hr = pKsControl->KsProperty((KSPROPERTY*)&XU, sizeof(XU), Data, sizeof(Data), &xuLength);
                if (SUCCEEDED(hr)) {
                    for (UWORD i = 0; i < xuLength; i++) {
                        printf("%02X", Data[i]);
                    }
                    printf("\n");
                    done = true;
                }
            }

        Next:
            if (pKsControl) pKsControl->Release();
            if (pFilter) pFilter->Release();
            if (wszDisplayName) CoTaskMemFree(wszDisplayName);
            pMoniker->Release();
        }
    }
    if (!done) {
        if (write) {
            printf("failed to write");
            for (ULONG i = 0; i < DataLen; i++) {
                printf(" %02X", Data[i]);
            }
            printf(" to");
        }
        else {
            printf("failed to read from");
        }
        if (VID != -1) {
            printf(" vid=%04x", VID);
        }
        if (PID != -1) {
            printf(" pid=%04x", VID);
        }
        if (Intf != -1) {
            printf(" interface=%d", Intf);
        }
        printf(" <%s> cs=%d", xuName, ControlSelector);
        printf("\n");
    }

Return:
    if (FAILED(hr)) {
        printf("error=%d\n", hr);
    }
    if (pEnum) pEnum->Release();
    if (pDevEnum) pDevEnum->Release();
    if (pBindCtx) pBindCtx->Release();
    CoUninitialize();
    return 0;
}