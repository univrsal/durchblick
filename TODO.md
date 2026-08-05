# Performance roadmap

## High impact

- [x] Add a configurable internal multiview render resolution, independent of
      the OBS canvas and output resolutions. Default to automatic and offer
      presets such as 1280x720, 1920x1080, 2560x1440, and canvas resolution.
- [x] Render each duplicated scene/source at most once per OBS frame into the
      internal-resolution cache, then reuse that texture across cells and
      Durchblick displays.
- [x] Preserve source aspect ratio, HDR/SDR color handling, alpha, safe areas,
      labels, and pixel-identical cell geometry when using the cache.
- [x] Recreate cached render targets only when the configured resolution,
      source dimensions, or OBS color space changes.
- [x] Batch empty grid-cell backgrounds to reduce draw calls on large layouts.
- [x] Use the volume-meter shader to reduce each channel to a small fixed
      number of draw calls.

## Validation

- [x] Add render-time, rendered-pixel, source-cache, and grid-batch counters.
- [ ] Benchmark 4K canvas layouts at internal 4K, 1440p, 1080p, and 720p with
      one and two Durchblick displays.
- [ ] Test Windows D3D11, macOS Metal, Linux OpenGL, SDR, and HDR before making
      the internal-resolution cache the default path.
