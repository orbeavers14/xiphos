// SPDX-License-Identifier: GPL-2.0-or-later
#import <Foundation/Foundation.h>
#import <Security/Security.h>

#include "remote_io.hpp"
#include <glib.h>
#include <exception>
#include <fstream>
#include <stdexcept>

using namespace xiphos::core;
using namespace xiphos::core::detail;

namespace {
bool valid_https(NSURL *url) {
    return url.scheme && [url.scheme caseInsensitiveCompare:@"https"] == NSOrderedSame &&
        url.host.length && !url.user && !url.password &&
        [url.absoluteString lengthOfBytesUsingEncoding:NSUTF8StringEncoding] <= 16384;
}

NSArray *trust_anchors(const std::string &path) {
    if (path.empty()) return nil;
    NSString *name = [NSString stringWithUTF8String:path.c_str()];
    NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath:name];
    constexpr NSUInteger limit = 4 * 1024 * 1024;
    NSData *data = [file readDataUpToLength:limit + 1 error:nil];
    [file closeFile];
    if (!data || data.length > limit) throw std::invalid_argument("Invalid CA bundle");
    NSString *pem = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    NSMutableArray *anchors = [NSMutableArray array];
    const auto records = [pem componentsSeparatedByString:@"-----BEGIN CERTIFICATE-----"];
    for (NSUInteger i = 1; i < records.count; ++i) {
        NSString *record = records[i];
        NSRange end = [record rangeOfString:@"-----END CERTIFICATE-----"];
        if (end.location == NSNotFound) throw std::invalid_argument("Invalid CA certificate");
        NSData *der = [[NSData alloc] initWithBase64EncodedString:[record substringToIndex:end.location]
                                                        options:NSDataBase64DecodingIgnoreUnknownCharacters];
        SecCertificateRef cert = der ? SecCertificateCreateWithData(nullptr, (__bridge CFDataRef)der) : nullptr;
        if (!cert) throw std::invalid_argument("Invalid CA certificate");
        [anchors addObject:CFBridgingRelease(cert)];
    }
    if (!anchors.count) throw std::invalid_argument("CA bundle contains no certificates");
    return anchors;
}

struct Sink {
    TransferControl &control;
    std::ofstream file;
    std::uint64_t limit, bytes = 0, total = 0;
    unsigned redirects = 0;
    std::exception_ptr error;
    std::unique_ptr<GChecksum, decltype(&g_checksum_free)> hash{g_checksum_new(G_CHECKSUM_SHA256), g_checksum_free};
    Sink(const std::filesystem::path &path, TransferControl &c, std::uint64_t maximum)
        : control(c), file(path, std::ios::binary), limit(maximum) {
        if (!file || !hash) throw std::runtime_error("Cannot create transfer file");
        file.exceptions(std::ios::badbit | std::ios::failbit);
        std::filesystem::permissions(path, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
    }
};
}

@interface XiphosDownloadDelegate : NSObject <NSURLSessionDataDelegate>
@property(nonatomic) Sink *sink;
@property(nonatomic, strong) NSArray *anchors;
@property(nonatomic, strong) dispatch_semaphore_t finished;
@end

@implementation XiphosDownloadDelegate
- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task
    willPerformHTTPRedirection:(NSHTTPURLResponse *)response newRequest:(NSURLRequest *)request
    completionHandler:(void (^)(NSURLRequest *))completion {
    completion(++self.sink->redirects <= 5 && valid_https(request.URL) ? request : nil);
}

- (void)URLSession:(NSURLSession *)session didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
    completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition, NSURLCredential *))completion {
    if (![challenge.protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust]) {
        completion(NSURLSessionAuthChallengeCancelAuthenticationChallenge, nil);
        return;
    }
    if (!self.anchors) {
        completion(NSURLSessionAuthChallengePerformDefaultHandling, nil);
        return;
    }
    SecTrustRef trust = challenge.protectionSpace.serverTrust;
    if (trust && SecTrustSetAnchorCertificates(trust, (__bridge CFArrayRef)self.anchors) == errSecSuccess &&
        SecTrustSetAnchorCertificatesOnly(trust, true) == errSecSuccess && SecTrustEvaluateWithError(trust, nullptr)) {
        completion(NSURLSessionAuthChallengeUseCredential, [NSURLCredential credentialForTrust:trust]);
    } else {
        completion(NSURLSessionAuthChallengeCancelAuthenticationChallenge, nil);
    }
}

- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)task
    didReceiveResponse:(NSURLResponse *)response completionHandler:(void (^)(NSURLSessionResponseDisposition))completion {
    auto &sink = *self.sink;
    try {
        TransferAccess::check(sink.control);
        if (![response isKindOfClass:NSHTTPURLResponse.class] ||
            ((NSHTTPURLResponse *)response).statusCode != 200 || !valid_https(response.URL))
            throw std::runtime_error("HTTPS download did not return a 200 response");
        const auto size = response.expectedContentLength;
        if (size > static_cast<int64_t>(sink.limit)) throw std::invalid_argument("Declared download exceeds byte limit");
        sink.total = size > 0 ? size : 0;
        TransferAccess::download(sink.control, 0, sink.total);
        completion(NSURLSessionResponseAllow);
    } catch (...) {
        sink.error = std::current_exception();
        completion(NSURLSessionResponseCancel);
    }
}

- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)task didReceiveData:(NSData *)data {
    auto &sink = *self.sink;
    if (sink.error) return;
    try {
        TransferAccess::check(sink.control);
        if (data.length > sink.limit - sink.bytes) throw std::invalid_argument("Download exceeds byte limit");
        sink.file.write(static_cast<const char *>(data.bytes), data.length);
        g_checksum_update(sink.hash.get(), static_cast<const guchar *>(data.bytes), data.length);
        sink.bytes += data.length;
        TransferAccess::download(sink.control, sink.bytes, sink.total);
    } catch (...) {
        sink.error = std::current_exception();
        [task cancel];
    }
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error {
    auto &sink = *self.sink;
    if (error && !sink.error)
        sink.error = std::make_exception_ptr(std::runtime_error("HTTPS download failed"));
    dispatch_semaphore_signal(self.finished);
}
@end

namespace xiphos::core::detail {
DownloadedArchive download_https(const std::string &url, const std::filesystem::path &file,
                                 const TransferOptions &options, TransferControl &control) {
    if (!transfer_options_valid(options) || url.rfind("https://", 0) != 0 || url.size() > 16384)
        throw std::invalid_argument("Invalid HTTPS transfer request");
    @autoreleasepool {
        NSURL *address = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
        if (!valid_https(address)) throw std::invalid_argument("Invalid HTTPS transfer URL");
        TransferAccess::phase(control, TransferPhase::downloading);
        Sink sink(file, control, options.maximum_download_bytes);
        auto delegate = [XiphosDownloadDelegate new];
        delegate.sink = &sink;
        delegate.anchors = trust_anchors(options.ca_bundle);
        delegate.finished = dispatch_semaphore_create(0);
        auto configuration = NSURLSessionConfiguration.ephemeralSessionConfiguration;
        configuration.timeoutIntervalForRequest = 15;
        configuration.timeoutIntervalForResource = options.timeout_seconds;
        configuration.TLSMinimumSupportedProtocolVersion = tls_protocol_version_TLSv12;
        configuration.HTTPShouldSetCookies = NO;
        configuration.URLCredentialStorage = nil;
        configuration.HTTPAdditionalHeaders = @{@"User-Agent": @"XiphosCore/1"};
        auto session = [NSURLSession sessionWithConfiguration:configuration delegate:delegate delegateQueue:nil];
        auto task = [session dataTaskWithURL:address];
        [task resume];
        while (dispatch_semaphore_wait(delegate.finished, dispatch_time(DISPATCH_TIME_NOW, 50 * NSEC_PER_MSEC))) {
            try { TransferAccess::check(control); }
            catch (const TransferCancelled &) { [task cancel]; }
        }
        [session finishTasksAndInvalidate];
        TransferAccess::check(control);
        if (sink.error) std::rethrow_exception(sink.error);
        sink.file.close();
        return {g_checksum_get_string(sink.hash.get()), sink.bytes};
    }
}
}
