typedef enum _SYMBOL_KNL {
  SYMBOL_KNL_PspLoadImageNotifyRoutine,
  SYMBOL_KNL_COUNT
} SYMBOL_KNL;

#define FOREACH_EXPORT_KNL(G) \
  G(PsSetLoadImageNotifyRoutine), \
  G(PsSetLoadImageNotifyRoutineEx), \
  G(PsSetCreateProcessNotifyRoutine), \
  G(PsRemoveCreateThreadNotifyRoutine), \
  G(ObCreateObjectType), \
  G(CmUnRegisterCallback), \

#define GENERATE_EXP_ENUM_KNL(ENUM) EXP_KNL_##ENUM

typedef enum _EXPORT_ID_KNL {
  FOREACH_EXPORT_KNL(GENERATE_EXP_ENUM_KNL)
  EXPORT_KNL_COUNT
} EXPORT_ID_KNL;
extern MODULE_PATCHED ModuleKernel;


