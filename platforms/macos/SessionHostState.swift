// SPDX-License-Identifier: GPL-2.0-or-later
import Foundation
import AppKit

struct SessionHostError: LocalizedError {
    let message: String
    var errorDescription: String? { message }
}
struct SessionWindowFrame: Codable, Sendable {
    var x: Double, y: Double, width: Double, height: Double
    init(_ frame: NSRect) { x = frame.origin.x; y = frame.origin.y; width = frame.width; height = frame.height }
    var rect: NSRect { NSRect(x: x, y: y, width: width, height: height) }
    var valid: Bool { [x,y,width,height].allSatisfy { $0.isFinite && abs($0) <= 100_000 } && width >= 700 && height >= 450 }
}
struct SessionSearchDraft: Codable, Sendable {
    var module: String, modules: [String], multiple: Bool, expression: String
    var mode: Int32, caseSensitive: Bool, annotationOptions: Bool
    var strongs: Bool, morphology: Bool, footnotes: Bool
    var scope: String, boundsModule: String, bounds: String
    @MainActor init(_ search: SearchModel) {
        module = search.module; modules = search.selectedModules; multiple = search.searchMultipleModules
        expression = search.expression; mode = search.mode.rawValue; caseSensitive = search.caseSensitive
        annotationOptions = search.setAnnotationOptions; strongs = search.searchStrongs
        morphology = search.searchMorphology; footnotes = search.searchFootnotes
        scope = search.scopeChoice.rawValue; boundsModule = search.boundsModule; bounds = search.bounds
    }
    @MainActor func apply(to search: SearchModel) {
        search.module = module; search.selectedModules = modules; search.searchMultipleModules = multiple
        search.expression = expression; search.mode = CoreSearchMode(rawValue: mode) ?? .phrase
        search.caseSensitive = caseSensitive; search.setAnnotationOptions = annotationOptions
        search.searchStrongs = strongs; search.searchMorphology = morphology; search.searchFootnotes = footnotes
        // A last-results scope cannot survive the intentionally discarded results.
        search.scopeChoice = scope == SearchScopeChoice.last.rawValue ? .all : SearchScopeChoice(rawValue: scope) ?? .all
        search.boundsModule = boundsModule; search.bounds = bounds
    }
}
private struct SessionHostEnvelope: Codable {
    var version = 1
    var hosts: [String: String] = [:]
}
struct SessionHostState: Codable, Sendable {
    var version = 1
    var showingBook: Bool, showChapter: Bool, verseNumbers: Bool, highlightSelected: Bool
    var referenceDraft: String, dictionaryDraft: String, bookDraft: String
    var dictionaryDisplayModule: String?, dictionaryDisplayKey: String?
    var dictionarySourceModule: String?, dictionarySourceReference: String?
    var search: SessionSearchDraft
    var frame: SessionWindowFrame?
    @MainActor init(_ model: ReaderModel, frame: NSRect? = nil) {
        showingBook = model.showingBook; showChapter = model.showChapter
        verseNumbers = model.verseNumbers; highlightSelected = model.highlightSelected
        referenceDraft = model.reference; dictionaryDraft = model.dictionaryKey; bookDraft = model.bookKey
        dictionaryDisplayModule = model.dictionaryEntry?.module; dictionaryDisplayKey = model.dictionaryEntry?.key
        dictionarySourceModule = model.dictionarySource?.module; dictionarySourceReference = model.dictionarySource?.reference
        search = SessionSearchDraft(model.search); self.frame = frame.map(SessionWindowFrame.init)
    }
    func encoded(preserving original: String = "") throws -> String {
        var envelope = original.isEmpty ? SessionHostEnvelope() : try JSONDecoder().decode(SessionHostEnvelope.self, from: Data(original.utf8))
        guard envelope.version == 1 else { throw SessionHostError(message: "This saved host metadata is unsupported.") }
        let encoder = JSONEncoder(); encoder.outputFormatting = [.sortedKeys]
        envelope.hosts["macos"] = String(decoding: try encoder.encode(self), as: UTF8.self)
        let data = try encoder.encode(envelope)
        guard data.count <= 32768, let text = String(data: data, encoding: .utf8) else {
            throw SessionHostError(message: "The window’s saved session is too large.")
        }
        return text
    }
    static func decode(_ text: String) throws -> SessionHostState? {
        if text.isEmpty { return nil }
        let envelope = try JSONDecoder().decode(SessionHostEnvelope.self, from: Data(text.utf8))
        guard envelope.version == 1 else { throw SessionHostError(message: "This saved host metadata is unsupported.") }
        guard let payload = envelope.hosts["macos"] else { return nil }
        let state = try JSONDecoder().decode(SessionHostState.self, from: Data(payload.utf8))
        guard state.version == 1, state.search.modules.count <= 32,
              CoreSearchMode(rawValue: state.search.mode) != nil,
              SearchScopeChoice(rawValue: state.search.scope) != nil,
              state.frame?.valid != false else {
            throw SessionHostError(message: "This saved window layout is unsupported.")
        }
        return state
    }
}
