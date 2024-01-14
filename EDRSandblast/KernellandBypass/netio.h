typedef enum _SYMBOL_NETIO {
  SYMBOL_NETIO_gWfpGlobal,
  SYMBOL_NETIO_COUNT
} SYMBOL_NETIO;

#define FOREACH_EXPORT_NETIO(G) \
  G(FeGetWfpGlobalPtr),

#define GENERATE_EXP_ENUM_NETIO(ENUM) EXP_NETIO_##ENUM

typedef enum _EXPORT_ID_NETIO {
  FOREACH_EXPORT_NETIO(GENERATE_EXP_ENUM_NETIO)
  EXPORT_NETIO_COUNT
} EXPORT_ID_NETIO;

extern MODULE_PATCHED ModuleNetio;
BOOL EnumNetio();
void DisableNetio();

typedef struct _NETIO_SETTING {
  OS_KEY os;
  UINT sizeOffset;
  // UINT offset_list; next pointer
  UINT8 itemSize;
} NETIO_SETTING, *PNETIO_SETTING;
