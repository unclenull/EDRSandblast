 ```
 _ESERVERSILO_GLOBALS PspHostSiloGlobals
    _ETW_SILODRIVERSTATE* EtwSiloState (nt!EtwpHostSiloState)
 ```

 ```
_ETW_SILODRIVERSTATE
[dt nt!_ETW_SILODRIVERSTATE poi(nt!EtwpHostSiloState)] 
  _ETW_HASH_BUCKET[0x40] EtwpGuidHashTable // providers
  _WMI_LOGGER_CONTEXT** EtwpLoggerContext; // Sessions
      struct _ETW_EVENT_CALLBACK_CONTEXT* CallbackContext; // Dynamic tracing callback registered by AV maybe
  _ETW_SYSTEM_LOGGER_SETTINGS SystemLoggerSettings; // for EtwpLogKernelEvent
 ```


 ```
_ETW_REG_ENTRY provider handle
 ```

 ```
NtTraceEvent(0, flag, ...)
flag: 500h security
 ```

## provider
- Each is represented by a `_ETW_GUID_ENTRY`
  + stored in the central place `nt!EtwpHostSiloState.EtwpGuidHashTable`
    - three list with each to a type
       ```
      _ETW_HASH_BUCKET
        _LIST_ENTRY<_ETW_GUID_ENTRY>[3] ListHead
          (1,2 rarely used)
          0 ETW_GUID_TYPE.EtwTraceGuidType
          1 ETW_GUID_TYPE.EtwNotificationGuidType
          2 ETW_GUID_TYPE.EtwGroupGuidType
       ```
- Can be registered more than once in the same app of different apps
  `_ETW_GUID_ENTRY.RegListHead`
- enable status `_ETW_GUID_ENTRY.ProviderEnableInfo`
- can be consumed by up to 8 loggers.
  each is represented by a `_TRACE_ENABLE_INFO` stored in `_ETW_GUID_ENTRY.EnableInfo`
  + a instance decides which loggers are enabled by `_ETW_REG_ENTRY.EnableMask/GroupEnableMask`
    the set bit position is the index of `_ETW_GUID_ENTRY.EnableInfo`

## logger
a logger is a so called session (`_WMI_LOGGER_CONTEXT`)
- global list `nt!EtwpHostSiloState.EtwpLoggerContext`
  + when used, the address is `1`
  + The first two ones are reserved/always 1
- count `nt!EtwpHostSiloState.MaxLoggers`
- Each logger is a thread of `EtwpLogger`
- subscribed providers are not stored anywhere in the kernel, instead by user space apps perhaps
- buffer
  + a pool of buffers, flushed periodically in a configurable interval
  + if used up, events are lost.
  + each starts with `_WMI_BUFFER_HEADER`
  + filled by
    `EtwpEventWriteFull/EtwpWriteUserEvent/EtwpLogKernelEvent`
    - before writing, they acquire the logger's rundown protection
      + each logger has a corresponding `_EX_RUNDOWN_REF_CACHE_AWARE` stored in `nt!EtwpHostSiloState.EtwpLoggerRundown`
    - `EtwpReserveTraceBuffer` checks if AcceptNewEvents < 0 || size > MaximumEventSize
  + delivered by `EtwpLogger -> EtwpFlushActiveBuffers -> EtwpFlushBuffer`
    - `EtwpFlushBufferToRealtime`
      + write to file
      + `EtwpRealtimeDeliverBuffer` send to realtime consumers 
    - `EtwpFlushBufferToLogfile`
    - `EtwpSendSessionNotification` notify the session when fail
    - `EtwpInvokeEventCallback` dynamic tracing callback

## cache aware rundown protection
Track the references of an object. Before destructuring, the owner waits for the reference count goes to zero.

1. increase references by consumers
  `ExAcquireRundownProtectionCacheAware/ExAcquireRundownProtectionCacheAwareEx`
  - one/specific
2. decrease
  `ExReleaseRundownProtectionCacheAware/ExReleaseRundownProtectionCacheAwareEx`
3. wait for rundown by the owner
  `ExWaitForRundownProtectionReleaseCacheAware`

- data structure for references `_EX_RUNDOWN_REF_CACHE_AWARE`
  + `RunRefs` pointer to array of counter structures
  + `PoolToFree` hardcoded number `0xBADCA11`, seems unused
  + `RunRefSize` size of the counter structure
  + `Number` count of Processors (counter array)

- There is a counter for each processor
- The counter data has a size (`RunRefSize`) of 8 if single cpu
  or with that returned by `KeGetRecommendedSharedDataAlignment()`, although a qword is used actually.
- When 'waiting', `RunRefs` points to an object, which contains the counter and more data
  + **Importantly, the address stored in RunRefs is manipulated, its least significance bit is set to 1**
    This guard against future `acquire`.
  + In aid for this mechanism, any reference count increases after doubled

