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

// Coordinates identify the modern settlements. References identify biblical mentions.
const places = [
    ["Jerusalem", 35.21633, 31.76904, 281184, "2 Samuel 5:6–9"],
    ["Bethlehem", 35.20376, 31.70487, 284315, "Matthew 2:1"],
    ["Nazareth", 35.29719, 32.70087, 294098, "Luke 1:26"],
    ["Jericho", 35.45000, 31.86667, 284899, "Luke 19:1"]
];
for (const [name, longitude, latitude, source, reference] of places) {
    const label = document.createElement("span");
    label.textContent = `${name} · ${reference} · Modern coordinates from GeoNames #${source}`;
    new maplibregl.Marker()
        .setLngLat([longitude, latitude])
        .setPopup(new maplibregl.Popup().setDOMContent(label))
        .addTo(map);
}

const packBridge = window.webkit?.messageHandlers?.mapPack;
const installPack = document.getElementById("install-pack");
const downloadPack = document.getElementById("download-pack");
const removePack = document.getElementById("remove-pack");
let archiveSequence = 0;

class InstalledArchiveSource {
    constructor(staged = false) {
        this.staged = staged;
        this.key = "regional-pack-" + ++archiveSequence;
    }

    getKey() { return this.key; }

    async getBytes(offset, length) {
        const encoded = await packBridge.postMessage({action: "read", staged: this.staged, offset, length});
        return {data: Uint8Array.from(atob(encoded), byte => byte.charCodeAt(0)).buffer};
    }
}

let placeLabels = [];

function clearPlaceLabels() {
    for (const marker of placeLabels) marker.remove();
    placeLabels = [];
}

map.on("idle", () => {
    clearPlaceLabels();
    if (!map.getLayer("archive-places")) return;
    const seen = new Set();
    for (const feature of map.queryRenderedFeatures({layers: ["archive-places"]})) {
        const name = feature.properties["name:en"] || feature.properties.name;
        if (typeof name !== "string" || feature.geometry.type !== "Point" || seen.has(name)) continue;
        seen.add(name);
        const element = document.createElement("span");
        element.className = "archive-place-label";
        element.textContent = name;
        placeLabels.push(new maplibregl.Marker({element, anchor: "left", offset: [6, 0]})
            .setLngLat(feature.geometry.coordinates).addTo(map));
        if (placeLabels.length === 30) break;
    }
});

function removeBasemap() {
    clearPlaceLabels();
    for (const id of ["archive-water", "archive-roads", "archive-raster", "archive-places"]) {
        if (map.getLayer(id)) map.removeLayer(id);
    }
    if (map.getSource("archive")) map.removeSource("archive");
    protocol.tiles.clear();
}

async function inspectArchive(archive) {
    const header = await archive.getHeader();
    const metadata = await archive.getMetadata();
    const vector = header.tileType === pmtiles.TileType.Mvt;
    const raster = [pmtiles.TileType.Png, pmtiles.TileType.Jpeg, pmtiles.TileType.Webp].includes(header.tileType);
    if (!vector && !raster) throw new Error("Unsupported tile format");
    const layers = metadata.vector_layers?.map(layer => layer.id) ?? [];
    if (vector && !layers.includes("water") && !layers.includes("roads")) {
        throw new Error("Unsupported vector layers");
    }
    if (header.minLon >= header.maxLon || header.minLat >= header.maxLat) {
        throw new Error("Invalid archive bounds");
    }
    return {header, vector, layers};
}

function showArchive(archive, {header, vector, layers}) {
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
        if (layers.includes("places")) {
            map.addLayer({id: "archive-places", type: "circle", source: "archive",
                "source-layer": "places", filter: ["==", ["geometry-type"], "Point"],
                paint: {"circle-radius": 3, "circle-color": "#526470", "circle-stroke-width": 1,
                    "circle-stroke-color": "#fff"}});
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
}

async function showInstalledPack() {
    const archive = new pmtiles.PMTiles(new InstalledArchiveSource());
    showArchive(archive, await inspectArchive(archive));
}

async function updatePack(action) {
    installPack.disabled = true;
    downloadPack.disabled = true;
    removePack.disabled = true;
    archivePicker.disabled = true;
    try {
        if (action === "install" || action === "download") {
            const selected = await packBridge.postMessage({action: action === "download" ? "download" : "choose"});
            if (selected) {
                await inspectArchive(new pmtiles.PMTiles(new InstalledArchiveSource(true)));
                await packBridge.postMessage({action: "install"});
                await showInstalledPack();
            }
        } else if (action === "remove") {
            await packBridge.postMessage({action: "remove"});
            removeBasemap();
            statusLine.textContent = "Showing bundled place markers.";
        } else if (action === "load" && await packBridge.postMessage({action: "installed"})) {
            await showInstalledPack();
        }
    } catch {
        statusLine.textContent = "Could not open or update the installed map pack.";
    } finally {
        try {
            await packBridge.postMessage({action: "discard"});
            removePack.disabled = !await packBridge.postMessage({action: "installed"});
        } finally {
            installPack.disabled = false;
            downloadPack.disabled = false;
            archivePicker.disabled = false;
        }
    }
}

map.once("load", () => {
    if (packBridge) {
        installPack.hidden = false;
        removePack.hidden = false;
        downloadPack.hidden = false;
        installPack.addEventListener("click", () => updatePack("install"));
        downloadPack.addEventListener("click", () => updatePack("download"));
        removePack.addEventListener("click", () => updatePack("remove"));
        updatePack("load");
    }
    archivePicker.addEventListener("change", async () => {
        const file = archivePicker.files[0];
        if (!file) return;
        archivePicker.disabled = true;
        try {
            const archive = new pmtiles.PMTiles(new LocalArchiveSource(file));
            showArchive(archive, await inspectArchive(archive));
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
