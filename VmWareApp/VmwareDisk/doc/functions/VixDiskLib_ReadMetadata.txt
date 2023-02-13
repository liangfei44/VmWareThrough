[name]
VixDiskLib_ReadMetadata

[description]
[code]
VixError
VixDiskLib_ReadMetadata(VixDiskLibHandle diskHandle,
                        const char *key,
                        char *buf,
                        size_t bufLen,
                        size_t *requiredLen);
[endcode]

This function retrieves the value of the given key from the disk metadata.

[parameters]
   diskHandle - Handle to an open virtual disk.
   key - Name of the key.
   buf - Buffer to fill in the value.
   bufLen - Length of the buffer for the value.
   requiredLen - Length of the key's value in bytes.

[return]
VIX_OK if the function succeeded, otherwise an appropriate VIX error code.

[remarks]
* Each virtual disk has a small amount of space to save arbitrary <key,value>
  pairs.
* Both key and value can be only ANSI strings.
* If bufLen is less than the space required, VixDiskLib_ReadMetadata() will 
  not modify the keys buffer and will return VIX_E_BUFFER_TOOSMALL.

[example]
[code]
   vixError = VixDiskLib_ReadMetadata(disk.Handle(),
                                      appGlobals.metaKey,
                                      &val[0],
                                      requiredLen,
                                      NULL);
[endcode]

