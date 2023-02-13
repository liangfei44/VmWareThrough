/******************************************************************************
 * Copyright 2009 VMware, Inc.  All rights reserved.
 *
 * VixMntapi.h  --
 *
 *      Contains function declarations for mounting VMDK files.
 *
 *******************************************************************************/

#ifndef _VIXMNTAPI_H_
#define _VIXMNTAPI_H_

#include <stdlib.h>
#include <stdarg.h>
#include "vm_basic_types.h"
#include "vixDiskLib.h"

#if defined(__cplusplus)
extern "C" {
#endif


/*
 * Ensure 4 byte struct alignment. VC++ default is 8 byte alignment, VMware
 * libs need 4.
 */
#if defined(_WIN32)
#include "pshpack4.h"
#endif

#define VIXMNTAPI_MAJOR_VERSION     1
#define VIXMNTAPI_MINOR_VERSION     0

/**
 * OS Family types - Currently, only Windows OS is recognized.
 */
typedef enum VixOsFamily {
   VIXMNTAPI_NO_OS            =  0,
   VIXMNTAPI_WINDOWS          =  1,
   VIXMNTAPI_OTHER            =  255
} VixOsFamily;

/**
 * Information about the default OS installed on the disks. Windows only.
 */
typedef struct {
   VixOsFamily family;        // OS Family
   uint32 majorVersion;       // On Windows, 4=NT, 5=2000 and above
   uint32 minorVersion;       // On Windows, 0=2000, 1=XP, 2=2003
   Bool  osIs64Bit;           // True if the OS is 64-bit
   char* vendor;              // e.g. Microsoft, RedHat, etc ...
   char* edition;             // e.g. Desktop, Enterprise, etc ...
   char* osFolder;            // Location where the default OS is installed
} VixOsInfo;

/**
 * Type of the volume.
 */
typedef enum VixVolumeType {
   VIXMNTAPI_UNKNOWN_VOLTYPE  = 0,
   VIXMNTAPI_BASIC_PARTITION  = 1,
   VIXMNTAPI_GPT_PARTITION    = 2,
   VIXMNTAPI_DYNAMIC_VOLUME   = 3,
   VIXMNTAPI_LVM_VOLUME       = 4
} VixVolumeType;

/**
 * Volume information.
 */
typedef struct {
   VixVolumeType    type;              // Type of the volume
   Bool             isMounted;         // True if the volume is mounted on the proxy.
   char            *symbolicLink;      // Path to the volume mount point,
                                       // NULL if the volume is not mounted on the proxy.
   size_t  numGuestMountPoints;        // Number of mount points for the volume in the guest
                                       // 0 if the volume is not mounted on the proxy.
   char *inGuestMountPoints[1];        // Mount points for the volume in the guest
} VixVolumeInfo;

/**
 * Diskset information.
 */
typedef struct {
   uint32 openFlags;
   char *mountPath;
} VixDiskSetInfo;

typedef struct VixDiskSetHandleStruct *VixDiskSetHandle;
typedef struct VixVolumeHandleStruct *VixVolumeHandle;

/**
 * Initializes the VixDiskMount library.
 * @param majorVersion [in] API major version.
 * @param minorVersion [in] API minor version.
 * @param log [in] Callback function to write log messages.
 * @param warn [in] Callback function to write warning messages.
 * @param panic [in] Callback function to report fatal errors.
 * @param libDir [in] Installation directory for library files - can be NULL.
 * @param configFile [in] Path name of the configuration file containing :
 *                tmpDirectory = <path to tempdir>
 * @return VIX_OK if success, suitable VIX error code otherwise.
 */
VixError
VixMntapi_Init(uint32 majorVersion,
               uint32 minorVersion,
               VixDiskLibGenericLogFunc *log,
               VixDiskLibGenericLogFunc *warn,
               VixDiskLibGenericLogFunc *panic,
               const char *libDir,
               const char *configFile);

/**
 * Cleans up VixDiskMount library.
 */
void
VixMntapi_Exit(void);

#ifdef _WIN32
/**
 * Opens the set of disks for mounting. All the disks for a dynamic volume or
 * LDM volume must be opened together.
 * @param diskHandles [in] Array of handles to open disks.
 * @param numberOfDisks [in] Number of disk handles in the array.
 * @param openMode [in, optional] Mode to open the diskset - Can be
 *             VIXDISKLIB_FLAG_OPEN_READ_ONLY
 * @param handle [out] Disk set handle filled in.
 * @return VIX_OK if success, suitable VIX error code otherwise.
 * Supported only on Windows.
 */
VixError
VixMntapi_OpenDiskSet(VixDiskLibHandle diskHandles[],
                      size_t numberOfDisks,
                      uint32 openMode,
                      VixDiskSetHandle *handle);
#endif

/**
 * Opens the set of disks for mounting.
 * @param connection [in] VixDiskLibConnection to use for opening the disks.
 *          VixDiskLib_Open with the specified flags will be called on each
 *          disk to open.
 * @param diskNames [in] Array of names of disks to open.
 * @param numberOfDisks [in] Number of disk handles in the array.
 *                           Must be 1 for Linux.
 * @param flags [in] Flags to open the disk.
 * @param handle [out] Disk set handle filled in.
 * @return VIX_OK if success, suitable VIX error code otherwise.
 */
VixError
VixMntapi_OpenDisks(VixDiskLibConnection connection,
                    const char *diskNames[],
                    size_t numberOfDisks,
                    uint32 openFlags,
                    VixDiskSetHandle *handle);

/**
 * Retrieves the diskSet information.
 * @param diskHandle [in] Handle to an opened diskSet.
 * @param diskSetInfo [out] Information about the diskSet.
 * @return VIX_OK if success, suitable VIX error code otherwise.
 */
VixError
VixMntapi_GetDiskSetInfo(VixDiskSetHandle handle,
                         VixDiskSetInfo **diskSetInfo);

/**
 * Frees the diskSet information.
 * @param diskSetInfo [in] Information about the diskSet to be free'ed.
 */
void
VixMntapi_FreeDiskSetInfo(VixDiskSetInfo *diskSetInfo);


/**
 * Closes the disk set.
 * @param diskSet [in] Handle to an open disk set.
 * @return VIX_OK if success, suitable VIX error code otherwise.
 */
VixError
VixMntapi_CloseDiskSet(VixDiskSetHandle diskSet);

/**
 * Retrieves handles to the volumes in the disk set.
 * @param diskSet [in] Handle to an open disk set.
 * @param numberOfVolumes [out] Number of volume handles .
 * @param volumeHandles [out] Array of volume handles.
 * @return VIX_OK if success, suitable VIX error code otherwise.
 */
VixError
VixMntapi_GetVolumeHandles(VixDiskSetHandle diskSet,
                           size_t *numberOfVolumes,
                           VixVolumeHandle *volumeHandles[]);

/**
 * Frees memory allocated in VixMntapi_GetVolumes.
 * @param volumeHandles [in] Volume handle to be freed.
 */
void
VixMntapi_FreeVolumeHandles(VixVolumeHandle *volumeHandles);

/**
 * Retrieves information about the default operating system in the disk set.
 * @param diskSet [in] Handle to an open disk set.
 * @param info [out] OS information filled up.
 * @return VIX_OK if success, suitable VIX error code otherwise.
 */
VixError
VixMntapi_GetOsInfo(VixDiskSetHandle diskSet,
                    VixOsInfo **info);

/**
 * Frees memory allocated in VixMntapi_GetOperatingSystemInfo.
 * @param info [in] OS info to be freed.
 */
void
VixMntapi_FreeOsInfo(VixOsInfo *info);

/**
 * Mounts the volume. After mounting the volume, use VixMntapi_GetVolumeInfo
 * to obtain the path to the mounted volume.
 * @param volumeHandle [in] Handle to a volume.
 * @param readOnly [in] Whether to mount the volume in read-only mode.
 * @return VIX_OK if success, suitable VIX error code otherwise.
 */
VixError
VixMntapi_MountVolume(VixVolumeHandle volumeHandle,
                      Bool readOnly);

/**
 * Unmounts the volume.
 * @param volumeHandle [in] Handle to a volume.
 * @param force [in] Force unmount even if files are open on the volume.
 * @return VIX_OK if success, suitable VIX error code otherwise.
 */
VixError
VixMntapi_DismountVolume(VixVolumeHandle volumeHandle,
                         Bool force);

/**
 * Retrieves information about a volume. Some of the volume information is
 * only available if the volume is mounted. Hence, this must be called after
 * calling VixMntapi_MountVolume.
 * @param volumeHandle [in] Handle to a volume.
 * @param info [out] Volume information filled up.
 * @return VIX_OK if success, suitable VIX error code otherwise.
 */
VixError
VixMntapi_GetVolumeInfo(VixVolumeHandle volumeHandle,
                        VixVolumeInfo **info);

/**
 * Frees memory allocated in VixMntapi_GetVolumeInfo.
 * @param info [in] Volume info to be freed.
 */
void
VixMntapi_FreeVolumeInfo(VixVolumeInfo *info);


/*
 * Restore alignment.
 */
#if defined(_WIN32)
#include "poppack.h"
#endif

#if defined(__cplusplus)
}
#endif

#endif // !defined _VIXMNTAPI_H_

