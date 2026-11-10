const statusLine = document.getElementById("status");
const archivePicker = document.getElementById("archive");
const protocol = new pmtiles.Protocol();

class LocalArchiveSource extends pmtiles.FileSource {
    getKey() { return "local-basemap"; }
}
maplibregl.addProtocol("pmtiles", protocol.tile);

const map = new maplibregl.Map({
    container: "map",
    center: [35.2, 32],
    zoom: 5,
    style: {
        version: 8,
        sources: {},
        layers: [{id: "background", type: "background", paint: {"background-color": "#eee8dc"}}]
    }
});
map.addControl(new maplibregl.NavigationControl());

const places = [
    ["Jerusalem", 35.23, 31.78],
    ["Bethlehem", 35.20, 31.71],
    ["Nazareth", 35.30, 32.70]
];
for (const [name, longitude, latitude] of places) {
    const label = document.createElement("span");
    label.textContent = name;
    new maplibregl.Marker()
        .setLngLat([longitude, latitude])
        .setPopup(new maplibregl.Popup().setDOMContent(label))
        .addTo(map);
}

function removeBasemap() {
    for (const id of ["archive-water", "archive-roads", "archive-raster"]) {
        if (map.getLayer(id)) map.removeLayer(id);
    }
    if (map.getSource("archive")) map.removeSource("archive");
}

map.once("load", () => {
    archivePicker.addEventListener("change", async () => {
        const file = archivePicker.files[0];
        if (!file) return;
        archivePicker.disabled = true;
        try {
            const archive = new pmtiles.PMTiles(new LocalArchiveSource(file));
            const header = await archive.getHeader();
            const metadata = await archive.getMetadata();
            const vector = header.tileType === pmtiles.TileType.Mvt;
            const raster = [pmtiles.TileType.Png, pmtiles.TileType.Jpeg, pmtiles.TileType.Webp].includes(header.tileType);
            if (!vector && !raster) throw new Error("Unsupported tile format");
            const layers = metadata.vector_layers?.map(layer => layer.id) ?? [];
            if (vector && !layers.includes("water") && !layers.includes("roads")) {
                throw new Error("Unsupported vector layers");
            }
            removeBasemap();
            protocol.add(archive);
            map.addSource("archive", {
                type: vector ? "vector" : "raster",
                url: "pmtiles://" + archive.source.getKey()
            });
            if (vector) {
                if (layers.includes("water")) {
                    map.addLayer({id: "archive-water", type: "fill", source: "archive",
                        "source-layer": "water", paint: {"fill-color": "#9fc4df"}});
                }
                if (layers.includes("roads")) {
                    map.addLayer({id: "archive-roads", type: "line", source: "archive",
                        "source-layer": "roads", paint: {"line-color": "#a89d8d"}});
                }
            } else {
                map.addLayer({id: "archive-raster", type: "raster", source: "archive"});
            }
            map.fitBounds([[header.minLon, header.minLat], [header.maxLon, header.maxLat]], {padding: 30});
            statusLine.textContent = vector ? "Local archive selected. © OpenStreetMap contributors." : "Local raster archive selected. Check its license for attribution.";
        } catch {
            statusLine.textContent = "Could not open this archive. Use a raster or Protomaps vector PMTiles file.";
        } finally {
            archivePicker.value = "";
            archivePicker.disabled = false;
        }
    });
});
map.on("error", () => {
    statusLine.textContent = "A map resource could not be rendered.";
});
