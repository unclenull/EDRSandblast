#include "./symbol.h"

typedef enum _SYM_KNL {
  SYM_KNL_PspLoadImageNotifyRoutine,
  SYM_KNL_WmipLoggerContext,
  SYM_KNL_EtwpGetSiloDriverState,
  SYM_KNL_EtwpSiloState,
  SYM_KNL_EtwpHostSiloState,
  SYM_KNL_COUNT
} SYM_KNL;

#define FOREACH_EXPORT_KNL(G) \
  G(PsSetLoadImageNotifyRoutine), \
  G(PsSetLoadImageNotifyRoutineEx), \
  \
  G(PsSetCreateProcessNotifyRoutine), \
  G(PsRemoveCreateThreadNotifyRoutine), \
  G(ObCreateObjectType), \
  G(CmUnRegisterCallback), \
  \
  G(NtTraceEvent), \
  G(EtwSendTraceBuffer), \

#define GENERATE_EXP_ENUM_KNL(ENUM) EXP_KNL_##ENUM

typedef enum _EXPORT_ID_KNL {
  FOREACH_EXPORT_KNL(GENERATE_EXP_ENUM_KNL)
  EXP_KNL_COUNT
} EXPORT_ID_KNL;
extern DWORD64 KernelSymbolAddresses[SYM_KNL_COUNT];
BOOL KernelSetup(PSYSTEM_MODULE_INFORMATION moduleRawList);

BOOL EnumEtw();
void DisableEtw();
