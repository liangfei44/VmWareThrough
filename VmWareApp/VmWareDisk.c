#include "../VmWareApp/VmwareDisk/include/vixdisklib.h"
#include <Windows.h>
#include <stdio.h>

BOOLEAN
LoadOneFunc(HINSTANCE hInstLib, void** pFunction, const char* funcName)
{
	BOOLEAN bRet = TRUE;
	*pFunction = (void*)GetProcAddress(hInstLib, funcName);
	if (*pFunction == NULL) {
		bRet = FALSE;
	}
	return bRet;
}

#define LOAD_ONE_FUNC(handle, funcName)  \
   LoadOneFunc(handle, (void**)&(funcName##_Ptr), #funcName)

#define IS_HANDLE_INVALID(handle) ((handle) == INVALID_HANDLE_VALUE)

#define VIXDISKLIB_VERSION_MAJOR 6
#define VIXDISKLIB_VERSION_MINOR 0

static VixError
(*VixDiskLib_InitEx_Ptr)(uint32 majorVersion,
	uint32 minorVersion,
	VixDiskLibGenericLogFunc *log,
	VixDiskLibGenericLogFunc *warn,
	VixDiskLibGenericLogFunc *panic,
	const char* libDir,
	const char* configFile);

static VixError
(*VixDiskLib_Init_Ptr)(uint32 majorVersion,
	uint32 minorVersion,
	VixDiskLibGenericLogFunc *log,
	VixDiskLibGenericLogFunc *warn,
	VixDiskLibGenericLogFunc *panic,
	const char* libDir);

static void
(*VixDiskLib_Exit_Ptr)(void);

static const char *
(*VixDiskLib_ListTransportModes_Ptr)(void);


static VixError
(*VixDiskLib_Cleanup_Ptr)(const VixDiskLibConnectParams *connectParams,
	uint32 *numCleanedUp, uint32 *numRemaining);

static VixError
(*VixDiskLib_Connect_Ptr)(const VixDiskLibConnectParams *connectParams,
	VixDiskLibConnection *connection);

static VixError
(*VixDiskLib_ConnectEx_Ptr)(const VixDiskLibConnectParams *connectParams,
	Bool readOnly,
	const char *snapshotRef,
	const char *transportModes,
	VixDiskLibConnection *connection);

static VixError
(*VixDiskLib_Disconnect_Ptr)(VixDiskLibConnection connection);

static VixError
(*VixDiskLib_Create_Ptr)(const VixDiskLibConnection connection,
	const char *path,
	const VixDiskLibCreateParams *createParams,
	VixDiskLibProgressFunc progressFunc,
	void *progressCallbackData);

static VixError
(*VixDiskLib_CreateChild_Ptr)(VixDiskLibHandle diskHandle,
	const char *childPath,
	VixDiskLibDiskType diskType,
	VixDiskLibProgressFunc progressFunc,
	void *progressCallbackData);

static VixError
(*VixDiskLib_Open_Ptr)(const VixDiskLibConnection connection,
	const char *path,
	uint32 flags,
	VixDiskLibHandle *diskHandle);

static VixError
(*VixDiskLib_GetInfo_Ptr)(VixDiskLibHandle diskHandle,
	VixDiskLibInfo **info);

static void
(*VixDiskLib_FreeInfo_Ptr)(VixDiskLibInfo *info);


static const char *
(*VixDiskLib_GetTransportMode_Ptr)(VixDiskLibHandle diskHandle);

static VixError
(*VixDiskLib_Close_Ptr)(VixDiskLibHandle diskHandle);

static VixError
(*VixDiskLib_Read_Ptr)(VixDiskLibHandle diskHandle,
	VixDiskLibSectorType startSector,
	VixDiskLibSectorType numSectors,
	uint8 *readBuffer);

static VixError
(*VixDiskLib_Write_Ptr)(VixDiskLibHandle diskHandle,
	VixDiskLibSectorType startSector,
	VixDiskLibSectorType numSectors,
	const uint8 *writeBuffer);

static VixError
(*VixDiskLib_ReadMetadata_Ptr)(VixDiskLibHandle diskHandle,
	const char *key,
	char *buf,
	size_t bufLen,
	size_t *requiredLen);

static VixError
(*VixDiskLib_WriteMetadata_Ptr)(VixDiskLibHandle diskHandle,
	const char *key,
	const char *val);

static VixError
(*VixDiskLib_GetMetadataKeys_Ptr)(VixDiskLibHandle diskHandle,
	char *keys,
	size_t maxLen,
	size_t *requiredLen);

static VixError
(*VixDiskLib_Unlink_Ptr)(VixDiskLibConnection connection,
	const char *path);

static VixError
(*VixDiskLib_Grow_Ptr)(VixDiskLibConnection connection,
	const char *path,
	VixDiskLibSectorType capacity,
	Bool updateGeometry,
	VixDiskLibProgressFunc progressFunc,
	void *progressCallbackData);
static VixError
(*VixDiskLib_Shrink_Ptr)(VixDiskLibHandle diskHandle,
	VixDiskLibProgressFunc progressFunc,
	void *progressCallbackData);

static VixError
(*VixDiskLib_Defragment_Ptr)(VixDiskLibHandle diskHandle,
	VixDiskLibProgressFunc progressFunc,
	void *progressCallbackData);

static VixError
(*VixDiskLib_Rename_Ptr)(const char *srcFileName,
	const char *dstFileName);

static VixError
(*VixDiskLib_Clone_Ptr)(const VixDiskLibConnection dstConnection,
	const char *dstPath,
	const VixDiskLibConnection srcConnection,
	const char *srcPath,
	const VixDiskLibCreateParams *vixCreateParams,
	VixDiskLibProgressFunc progressFunc,
	void *progressCallbackData,
	Bool overWrite);

static char *
(*VixDiskLib_GetErrorText_Ptr)(VixError err, const char *locale);

static void
(*VixDiskLib_FreeErrorText_Ptr)(char* errMsg);

static VixError
(*VixDiskLib_Attach_Ptr)(VixDiskLibHandle parent, VixDiskLibHandle child);

static VixError
(*VixDiskLib_SpaceNeededForClone_Ptr)(VixDiskLibHandle diskHandle,
	VixDiskLibDiskType cloneDiskType,
	uint64* spaceNeeded);

static VixError
(*VixDiskLib_CheckRepair_Ptr)(const VixDiskLibConnection connection,
	const char *filename,
	Bool repair);

#define VixDiskLib_InitEx           (*VixDiskLib_InitEx_Ptr)
#define VixDiskLib_Init             (*VixDiskLib_Init_Ptr)
#define VixDiskLib_Exit             (*VixDiskLib_Exit_Ptr)
#define VixDiskLib_ListTransportModes   (*VixDiskLib_ListTransportModes_Ptr)
#define VixDiskLib_Cleanup          (*VixDiskLib_Cleanup_Ptr)
#define VixDiskLib_Connect          (*VixDiskLib_Connect_Ptr)
#define VixDiskLib_ConnectEx        (*VixDiskLib_ConnectEx_Ptr)
#define VixDiskLib_Disconnect       (*VixDiskLib_Disconnect_Ptr)
#define VixDiskLib_Create           (*VixDiskLib_Create_Ptr)
#define VixDiskLib_CreateChild      (*VixDiskLib_CreateChild_Ptr)
#define VixDiskLib_Open             (*VixDiskLib_Open_Ptr)
#define VixDiskLib_GetInfo          (*VixDiskLib_GetInfo_Ptr)
#define VixDiskLib_FreeInfo         (*VixDiskLib_FreeInfo_Ptr)
#define VixDiskLib_GetTransportMode (*VixDiskLib_GetTransportMode_Ptr)
#define VixDiskLib_Close            (*VixDiskLib_Close_Ptr)
#define VixDiskLib_Read             (*VixDiskLib_Read_Ptr)
#define VixDiskLib_Write            (*VixDiskLib_Write_Ptr)
#define VixDiskLib_ReadMetadata     (*VixDiskLib_ReadMetadata_Ptr)
#define VixDiskLib_WriteMetadata    (*VixDiskLib_WriteMetadata_Ptr)
#define VixDiskLib_GetMetadataKeys  (*VixDiskLib_GetMetadataKeys_Ptr)
#define VixDiskLib_Unlink           (*VixDiskLib_Unlink_Ptr)
#define VixDiskLib_Grow             (*VixDiskLib_Grow_Ptr)
#define VixDiskLib_Shrink           (*VixDiskLib_Shrink_Ptr)
#define VixDiskLib_Defragment       (*VixDiskLib_Defragment_Ptr)
#define VixDiskLib_Rename           (*VixDiskLib_Rename_Ptr)
#define VixDiskLib_Clone            (*VixDiskLib_Clone_Ptr)
#define VixDiskLib_GetErrorText     (*VixDiskLib_GetErrorText_Ptr)
#define VixDiskLib_FreeErrorText    (*VixDiskLib_FreeErrorText_Ptr)
#define VixDiskLib_Attach           (*VixDiskLib_Attach_Ptr)
#define VixDiskLib_SpaceNeededForClone   (*VixDiskLib_SpaceNeededForClone_Ptr)
#define VixDiskLib_CheckRepair      (*VixDiskLib_CheckRepair_Ptr)

typedef struct _APP_GLOBALS {
	int command;
	VixDiskLibAdapterType adapterType;
	char *transportModes;
	char *diskPath;
	char *parentPath;
	char *metaKey;
	char *metaVal;
	int filler;
	unsigned mbSize;
	VixDiskLibSectorType numSectors;
	VixDiskLibSectorType startSector;
	VixDiskLibSectorType bufSize;
	uint32 openFlags;
	unsigned numThreads;
	Bool success;
	Bool isRemote;
	char *host;
	char *userName;
	char *password;
	char *thumbPrint;
	int port;
	int nfcHostPort;
	char *srcPath;
	VixDiskLibConnection connection;
	char *vmxSpec;
	BOOLEAN useInitEx;
	char *cfgFile;
	char *libdir;
	char *ssMoRef;
	int repair;
} APP_GLOBALS, *PAPP_GLOBALS;

APP_GLOBALS gAppGlobals;



BOOLEAN
DynLoadDiskLib()
{
	BOOLEAN bRet = FALSE;
	do
	{
		
		HINSTANCE hInstLib = LoadLibraryA("..//VmWareApp//VmwareDisk//bin//vixDiskLib.dll");
		// If the handle is valid, try to get the function address.
		if (IS_HANDLE_INVALID(hInstLib)) {
			printf("VmWareDisk£ºDynLoadDiskLib ErrorCode:%d\n", GetLastError());
			break;
		}
		if (!LOAD_ONE_FUNC(hInstLib, VixDiskLib_InitEx)) {
			break;
		}
		if (!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Init)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Exit)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_ListTransportModes)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Cleanup)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Connect)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_ConnectEx)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Disconnect)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Create)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_CreateChild)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Open)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_GetInfo)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_FreeInfo)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_GetTransportMode)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Close)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Read)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Write)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_ReadMetadata)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_WriteMetadata)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_GetMetadataKeys)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Unlink)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Grow)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Shrink)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Defragment)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Rename)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Clone)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_GetErrorText)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_FreeErrorText)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_Attach)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_SpaceNeededForClone)) {
			break;
		}
		if(!LOAD_ONE_FUNC(hInstLib, VixDiskLib_CheckRepair)) {
			break;
		}
		bRet = TRUE;

	} while (FALSE);

	return bRet;
}



BOOLEAN VmWareDiskInit()
{
	VixDiskLibConnectParams cnxParams = { 0 };
	BOOLEAN bRet = FALSE;
	VixError vixError;
	do
	{
		memset(&gAppGlobals, 0, sizeof gAppGlobals);
		gAppGlobals.command = 0;
		gAppGlobals.adapterType = VIXDISKLIB_ADAPTER_SCSI_BUSLOGIC;
		gAppGlobals.startSector = 0;
		gAppGlobals.numSectors = 1;
		gAppGlobals.mbSize = 100;
		gAppGlobals.filler = 0xff;
		gAppGlobals.openFlags = 0;
		gAppGlobals.numThreads = 1;
		gAppGlobals.success = TRUE;
		gAppGlobals.isRemote = FALSE;
		if (!DynLoadDiskLib()) {
			break;
		}
		vixError = VixDiskLib_InitEx(VIXDISKLIB_VERSION_MAJOR,
			VIXDISKLIB_VERSION_MINOR,
			NULL, NULL, NULL,
			NULL,
			NULL);
		if (VIX_FAILED(vixError)) {
			break;
		}
		vixError = VixDiskLib_Connect(&cnxParams,
			&gAppGlobals.connection);

		if (VIX_FAILED(vixError)) {
			break;
		}

		bRet = TRUE;

	} while (FALSE);	

	return bRet;
}

BOOLEAN VmWareDiskOpen()
{
	BOOLEAN bRet = FALSE;
	VixError vixError = VIX_OK;
	VixDiskLibHandle handle = NULL;
	CHAR* path = "D:\\win10 1909\\Windows101909x64-000004.vmdk";
	do
	{
		vixError = VixDiskLib_Open(gAppGlobals.connection, path, VIXDISKLIB_FLAG_OPEN_READ_ONLY, &handle);
		if (VIX_FAILED(vixError)) {
			break;
		}
		printf("Disk \"%s\" is open using transport mode \"%s\".\n",
			path, VixDiskLib_GetTransportMode(handle));

		bRet = TRUE;

	} while (FALSE);
	
	return bRet;
}