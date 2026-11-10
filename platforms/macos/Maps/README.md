# Bundled map libraries

The app copies these upstream distributions into its local map resources.

- MapLibre GL JS 5.12.0, `dist/maplibre-gl.js`, `dist/maplibre-gl.css`
  and `dist/LICENSE.txt` from the `maplibre-gl` npm package.
- PMTiles 4.3.0, `dist/pmtiles.js` from the `pmtiles` npm package.
  The package declares BSD-3-Clause. The license is from the upstream
  Protomaps PMTiles repository.

The panel uses browser File objects to read archive ranges from disk.
No external tile service or runtime package installation is needed.
