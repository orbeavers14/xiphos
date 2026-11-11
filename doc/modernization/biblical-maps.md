# Biblical maps

Open **Preferences → Biblical Map…** in the Mac reader. The bundled map shows
Jerusalem, Bethlehem, Nazareth and Jericho. Each marker gives a biblical
reference and credits its modern coordinates to GeoNames. The coordinates
locate modern settlements and are not archaeological site boundaries.

- [Jerusalem](https://www.geonames.org/281184/jerusalem.html)
- [Bethlehem](https://www.geonames.org/284315/bethlehem.html)
- [Nazareth](https://www.geonames.org/294098/nazareth.html)
- [Jericho](https://www.geonames.org/284899/jericho.html)

Use **Open local map archive** to preview a PMTiles file for the current map
window. **Install regional pack…** copies a local raster or Protomaps-style
vector archive into Xiphos application support. The map reopens the installed
pack automatically. **Remove installed pack** removes the copy and returns to
the bundled markers. Packs are stored separately from SWORD modules and
reading profiles. One pack is installed at a time.

**Download pack from URL…** accepts a publisher’s HTTPS link and SHA-256
checksum. Downloads are limited to 128 MiB. The app checks the checksum and
reads the PMTiles header and metadata before replacing the installed archive.
The map itself blocks network requests and reads installed tiles by range.
There is no built-in regional pack catalog.

Raster archives display their tile artwork. The vector preview draws water,
roads and place symbols with up to 30 visible labels using system fonts. On
macOS 27.2, a low-zoom Eastern Mediterranean extract of the 2026-11-10
Protomaps build rendered water, place symbols and labels in WebKit with the
offline content policy. A corrupt archive preserved the preceding map. The
automated raster fixture also exercises local import and corrupt-file fallback.

[PMTiles integration](https://docs.protomaps.com/pmtiles/maplibre) ·
[Regional extracts](https://docs.protomaps.com/guide/getting-started)
