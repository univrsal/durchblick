# Performance validation

Durchblick reports an aggregate performance line to the OBS log every 30
seconds while a view is visible. It includes CPU submission time per frame,
the pixel reduction from the configured render limit, source-cache hits, and
grid-batch rebuilds.

## Benchmark matrix

Use the same scene collection and record the OBS **Stats** window plus the
Durchblick performance log for at least 60 seconds per case:

| Canvas | Multiview limit | Displays | Layout |
|---|---:|---:|---|
| 3840x2160 | Native | 1 | Default 4x4 |
| 3840x2160 | 2560x1440 | 1 | Default 4x4 |
| 3840x2160 | 1920x1080 | 1 | Default 4x4 |
| 3840x2160 | 1280x720 | 1 | Default 4x4 |
| 3840x2160 | 1920x1080 | 2 | Duplicate scenes |
| 1920x1080 | 1920x1080 | 1 | Default 4x4 |

Repeat the matrix on Windows/D3D11, macOS/Metal, and Linux/OpenGL. For HDR,
repeat the native and 1080p cases with Rec. 2100 PQ and verify highlights,
labels, alpha, safe areas, and preview/program indicators.

The render-resolution control changes only Durchblick. OBS canvas, stream,
recording, screenshots, and virtual-camera output remain at their configured
resolutions.
