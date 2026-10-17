// SPDX-License-Identifier: GPL-2.0-or-later
import SwiftUI
import ImageIO

// CGImage is an immutable retained snapshot; this wrapper only transfers a
// newly decoded thumbnail from a background task to the native view.
struct BookImageThumbnail: @unchecked Sendable { let image: CGImage }
struct BookImagePreview: Identifiable, Sendable {
    let id = UUID()
    let source: RenderedBookImage
    let thumbnail: BookImageThumbnail
    static func decode(_ source: RenderedBookImage) async throws -> BookImagePreview {
        try await Task.detached(priority: .userInitiated) {
            guard let image = CGImageSourceCreateWithData(source.data as CFData, nil),
                  CGImageSourceGetCount(image) > 0 else { throw BookImageDecodeError.unsupported }
            let options: [String: Any] = [
                kCGImageSourceCreateThumbnailFromImageAlways as String: true,
                kCGImageSourceCreateThumbnailWithTransform as String: true,
                kCGImageSourceThumbnailMaxPixelSize as String: 2048
            ]
            guard let thumbnail = CGImageSourceCreateThumbnailAtIndex(image, 0, options as CFDictionary) else {
                throw BookImageDecodeError.unsupported
            }
            return BookImagePreview(source: source, thumbnail: BookImageThumbnail(image: thumbnail))
        }.value
    }
}
private enum BookImageDecodeError: Error { case unsupported }
struct BookImagePreviewView: View {
    let image: BookImagePreview
    let close: () -> Void
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                VStack(alignment: .leading) {
                    Text(URL(fileURLWithPath: image.source.filename).lastPathComponent).font(.headline)
                    Text(image.source.module).font(.caption).foregroundStyle(.secondary)
                }
                Spacer()
                Button("Close", action: close).keyboardShortcut(.cancelAction)
            }.padding(12)
            Divider()
            Image(image.thumbnail.image, scale: 1, label: Text("Book image preview")).resizable().scaledToFit().padding(12)
        }.frame(minWidth: 360, idealWidth: 640, minHeight: 280, idealHeight: 480)
    }
}
