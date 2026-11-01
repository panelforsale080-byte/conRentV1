# conRentV1

Same-process IL2CPP dump agent for an app you own or are authorized to modify.

This repo contains only the updated dump source. The previous LibTool tree is left unchanged.

## Output
After Unity/IL2CPP is ready, `libTool.so` writes:

- `Android/data/<package>/files/dump.cs`
- `Android/data/<package>/files/log.txt`

Method lines use the true library base (`libil2cpp.so`, `liblogic.so`, `libcsharp.so`, or any app `.so` that exports IL2CPP APIs):

```
// Offset: 0x1a208
// Address: 0x7a......
// Lib: liblogic.so
```

Field lines use the IL2CPP field offset:

```
int coins; // Offset: 0x18 Lib: liblogic.so
```

## Build
Push to `main` or run **Build IL2CPP Agent**. Download artifact `libTool-arm64-v8a`.
