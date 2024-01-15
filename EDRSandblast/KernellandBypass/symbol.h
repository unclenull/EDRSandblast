// echo -n "0x45, 0x33, 0xc0, 0x48, 0x8d, 0x0c, 0xd9, 0x48, 0x8b, 0xd7, 0xe8" | sed "s/0x//g" | tac -s , | tr -d ',' | sed "s/ //g" | rev | cut -c1-16 | rev

typedef enum _OS_KEY {
  OS_UNK,
  OS_XP,
  OS_2K3,
  OS_VISTA,
  OS_7,
  OS_8,
  OS_BLUE,
  OS_10_10240,
  OS_10_10586,
  OS_10_14393,
  OS_10_15063,
  OS_10_16299,
  OS_10_17134,
  OS_10_17763,
  OS_10_18362,
  OS_10_18363,
  OS_10_19041,
  OS_10_19042,
  OS_10_19043,
  OS_10_19044,
  OS_10_19045,
  OS_10_20348,
  OS_10_22000,
  OS_10_22621,
  OS_10_22631
} OS_KEY, *POS_KEY;

#define OS_10 OS_10_10240

extern OS_KEY BuildIDKey;

typedef struct _BUILDID_KEY_MAP {
  WORD buildId;
  OS_KEY key;
} BUILDID_KEY_MAP;

#define STR(STRING) #STRING

#define FOREACH_MODULE(G) \
  G(ntoskrnl, exe), \
  G(ahcache, sys), \
  G(mmcss, sys), \
  G(cng, sys), \
  G(ksecdd, sys), \
  G(tcpip, sys), \
  G(iorate, sys), \
  G(ci, dll), \
  G(dxkrnl, sys), \
  G(peauth, sys), \
  G(ndu, exe), \
  G(mpsdrv, sys), \

#define MODULE_ITEM(name, ext) { STR(name ## . ## ext), 0, 0 }
#define MODULE_ENUM(name, ext) MOD_ ## name

typedef enum _MODULE_ID {
  FOREACH_MODULE(MODULE_ENUM)
  MODULE_ID_COUNT
} MODULE_ID;

typedef struct _MODULE_PATCHED MODULE_PATCHED, *PMODULE_PATCHED;
typedef BOOL (*EXPORT_INITIALIZER)(PMODULE_PATCHED pM);

typedef struct _SYMBOL_META {
  UINT baseId;
  UINT16 range;
  DWORD64 pattern;
  DWORD64 mask;
  INT8 offset;
  BOOL fromSymbol;
} SYMBOL_META, *PSYMBOL_META;

typedef PSYMBOL_META (*SYMBOL_META_GETTER)();
typedef struct _SYMBOL_META_POOL_ITEM {
  UINT id;
  SYMBOL_META_GETTER metaGetter;
} SYMBOL_META_POOL_ITEM, *PSYMBOL_META_POOL_ITEM;

typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY {
    PVOID   Unknown1;
    PVOID   Unknown2;
    DWORD64 Base;
    ULONG   Size;
    ULONG   Flags;
    USHORT  Index;
    USHORT  NameLength;
    USHORT  LoadCount;
    USHORT  PathLength; // The length of the full path excluding the filename
    CHAR    ImageName[256];
} SYSTEM_MODULE_INFORMATION_ENTRY, *PSYSTEM_MODULE_INFORMATION_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION {
    ULONG   Count;
    SYSTEM_MODULE_INFORMATION_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

#define MODULE_MEMBERS \
  PCHAR name; \
  DWORD64 start; \
  DWORD64 end;

typedef struct _MODULE {
  MODULE_MEMBERS
} MODULE, *PMODULE;

struct _MODULE_PATCHED {
  MODULE_MEMBERS
  EXPORT_INITIALIZER exportsInitializer;
  UINT8 exportsCount;
  UINT8 *exportIds;
  PCHAR *exportNames;
  DWORD64 *exportAddresses;
  UINT8 symbolsCount;
  PSYMBOL_META_POOL_ITEM symbolMetaPool;
  DWORD64 *symbolsAddresses;
};

BOOL IsAddressWhitelisted(MODULE_ID *list, UINT8 size, DWORD64 addr);
PBYTE FindVersionedStruct(PBYTE start, UINT8 itemCount, UINT8 itemSize);
BOOL InitPatchedModule(PSYSTEM_MODULE_INFORMATION moduleRawList, PMODULE_PATCHED pM);
PSYSTEM_MODULE_INFORMATION GlobalSetup();

#ifdef _DEBUG
void BelongToDriver(DWORD64 addr);
#endif
