# Biblical maps

The Mac reader has a prototype under **Preferences → Biblical Map…**.
It shows three illustrative place markers and opens a local raster or
Protomaps-style PMTiles archive for the current session. The vector preview
draws water and roads. Raster archives can include labels and icons in their
tile images. The map panel bundles its scripts and blocks network requests.

Use MapLibre GL JS in the Mac reader's map panel. Bundle the renderer and style
locally so the same map page can later run in the Linux frontend. Keep place
records separate from the basemap. Store names, aliases, coordinates, sources,
and identification uncertainty in GeoJSON. Match selected text locally and let
the reader choose when a name refers to multiple places.

Start with a small bundled place layer and a coarse regional outline. Offer an
optional Eastern Mediterranean and Near East basemap pack for more detail.
Use a regional PMTiles extract and include its style, fonts, sprites, attribution,
and license notices. Keep routes and historical boundaries in separate layers
so they can be toggled and sourced independently.

Start with a local pack import. Verify that the Mac web view can read ranges
from an imported archive and render tiles, labels, and icons with networking
disabled. Test missing or damaged packs and confirm the bundled place layer
still opens. Once the import works, add an explicit download that shows the
region, version, and size before fetching. Verify the checksum before installing,
retain the previous pack if an update fails, and allow removal to reclaim space.
Store packs separately from SWORD modules and user data.

- [PMTiles integration with MapLibre](https://docs.protomaps.com/pmtiles/maplibre)
- [Regional extracts](https://docs.protomaps.com/guide/getting-started)
- [Offline basemap assets](https://docs.protomaps.com/basemaps/maplibre)
