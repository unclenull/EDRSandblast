typedef enum TSize { S1, S2, S4, S8 } Size;

typedef union TValue {
  BYTE byte;
  WORD word;
  DWORD dword;
  DWORD64 dword64;
} Value;

typedef struct TRESTORE_POINT {
  DWORD64 address;
  Size size;
  Value value;
  Value newValue;
} RESTORE_POINT;

typedef struct TRESTORE_POINTS {
  USHORT count;
  USHORT max;
  RESTORE_POINT* points;
} RESTORE_POINTS;

typedef void (*PATCH)(DWORD64 address, DWORD64 newValue);
typedef void (*PATCH_SIZE)(DWORD64 address, Value newValue, Size size);
typedef void (*PATCH_KNOWN)(DWORD64 address, DWORD64 newValue, DWORD64 value);
typedef void (*PATCH_EXPLICIT)(DWORD64 address, Value newValue, Value value, Size size);

void patch_r(DWORD64 address, DWORD64 newValue);
void patchSize_r(DWORD64 address, Value newValue, Size size);
void patchKnown_r(DWORD64 address, DWORD64 newValue, DWORD64 value);
void patchExplicit_r(DWORD64 address, Value newValue, Value value, Size size);

void patch_o(DWORD64 address, DWORD64 newValue);
void patchSize_o(DWORD64 address, Value newValue, Size size);
void patchKnown_o(DWORD64 address, DWORD64 newValue, DWORD64 value);
void patchExplicit_o(DWORD64 address, Value newValue, Value value, Size size);

void Restore();

BOOL includesString(TCHAR const** list, int count, TCHAR* entry);
void removeDoubleLinkedNode(DWORD64 LIST_ENTRY);

extern RESTORE_POINTS* RestorePoints;
extern PATCH Patch;
extern PATCH_SIZE PatchSize;
extern PATCH_KNOWN PatchKnown;
extern PATCH_EXPLICIT PatchExplicit;