// SPDX-License-Identifier: GPL-2.0-or-later
import SwiftUI
import ImageIO

// Immutable retained pixels cross from background decoding to the native view.
struct NativeImageThumbnail: @unchecked Sendable {
    let image: CGImage
    static func decode(_ data: Data) async throws -> NativeImageThumbnail {
        try await Task.detached(priority: .userInitiated) {
            guard let image = CGImageSourceCreateWithData(data as CFData, nil),
                  CGImageSourceGetCount(image) > 0 else { throw NativeImageDecodeError.unsupported }
            let options: [String: Any] = [
                kCGImageSourceCreateThumbnailFromImageAlways as String: true,
                kCGImageSourceCreateThumbnailWithTransform as String: true,
                kCGImageSourceThumbnailMaxPixelSize as String: 2048
            ]
            guard let thumbnail = CGImageSourceCreateThumbnailAtIndex(image, 0, options as CFDictionary) else {
                throw NativeImageDecodeError.unsupported
            }
            return NativeImageThumbnail(image: thumbnail)
        }.value
    }
}
private enum NativeImageDecodeError: Error { case unsupported }
struct BookImagePreview: Identifiable, Sendable {
    let id = UUID()
    let source: RenderedBookImage
    let thumbnail: NativeImageThumbnail
    static func decode(_ source: RenderedBookImage) async throws -> BookImagePreview {
        BookImagePreview(source: source, thumbnail: try await NativeImageThumbnail.decode(source.data))
    }
}
struct VerseImagePreview: Identifiable, Sendable {
    let id = UUID()
    let source: RenderedVerseImage
    let thumbnail: NativeImageThumbnail
    static func decode(_ source: RenderedVerseImage) async throws -> VerseImagePreview {
        VerseImagePreview(source: source, thumbnail: try await NativeImageThumbnail.decode(source.data))
    }
}
private struct ImagePreviewContent: View {
    let title: String, module: String, label: String
    let thumbnail: NativeImageThumbnail
    let close: () -> Void
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                VStack(alignment: .leading) {
                    Text(title).font(.headline)
                    Text(module).font(.caption).foregroundStyle(.secondary)
                }
                Spacer()
                Button("Close", action: close).keyboardShortcut(.cancelAction)
            }.padding(12)
            Divider()
            Image(thumbnail.image, scale: 1, label: Text(label)).resizable().scaledToFit().padding(12)
        }.frame(minWidth: 360, idealWidth: 640, minHeight: 280, idealHeight: 480)
    }
}
struct BookImagePreviewView: View {
    let image: BookImagePreview
    let close: () -> Void
    var body: some View {
        ImagePreviewContent(title: URL(fileURLWithPath: image.source.filename).lastPathComponent,
            module: image.source.module, label: "Book image preview", thumbnail: image.thumbnail, close: close)
    }
}
struct VerseImagePreviewView: View {
    let image: VerseImagePreview
    let close: () -> Void
    var body: some View {
        ImagePreviewContent(title: "Image", module: image.source.module, label: "Image preview",
            thumbnail: image.thumbnail, close: close)
    }
}
