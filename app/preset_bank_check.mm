#include "preset_bank.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <limits>
#include <thread>

using cosmos_presets::State;
using cosmos_preset_bank::Bank;
namespace pb = cosmos_preset_bank;
static unsigned checks = 0;
static void require(bool value, const char *what) {
    ++checks;
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(1); }
}
static bool same(const State &a, const State &b) {
    return a.rhythm == b.rhythm && a.program == b.program && a.controls == b.controls &&
        a.inputGainDb == b.inputGainDb && a.outputGainDb == b.outputGainDb && a.modes == b.modes && a.enabled == b.enabled;
}
static bool same(const Bank &a, const Bank &b) {
    for (size_t i = 0; i < pb::slotCount; ++i)
        if (![a.slots[i].name isEqualToString:b.slots[i].name] ||
            a.slots[i].state.has_value() != b.slots[i].state.has_value() ||
            (a.slots[i].state && !same(*a.slots[i].state, *b.slots[i].state))) return false;
    return true;
}
static State stateFor(size_t index) {
    State state; state.rhythm = index & 1; state.program = int(index % 12 + 1);
    state.controls = {{0, double(index) / 128, .5, 1, .75, .125}};
    state.inputGainDb = -36 + double(index) * .375; state.outputGainDb = 12 - double(index) * .375;
    state.modes = {{unsigned(index % 3), 0, unsigned((index + 1) % 3), unsigned((index + 2) % 3)}};
    state.enabled = {{bool(index & 1), bool(index & 2), bool(index & 4), bool(index & 8)}};
    return state;
}
static NSMutableDictionary *documentFor(const Bank &bank) {
    return [NSJSONSerialization JSONObjectWithData:pb::encodeData(bank) options:NSJSONReadingMutableContainers error:nil];
}
static void invalidDocument(NSDictionary *document, const Bank &before, const char *what) {
    Bank actual = before; NSError *error = nil;
    require(!pb::decode(document, actual, &error) && error.localizedDescription.length && same(before, actual), what);
}
static void invalidData(NSData *data, const Bank &before, const char *what) {
    Bank actual = before; NSError *error = nil;
    require(!pb::decodeData(data, actual, &error) && error.localizedDescription.length && same(before, actual), what);
}
int main(int argc, const char *argv[]) {
    @autoreleasepool {
        if (argc == 2) {
            Bank restarted;
            NSURL *library = [NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[1]]];
            require(pb::loadBank(library, restarted), "child process reads existing bank");
            for (size_t i = 0; i < pb::slotCount; ++i)
                require(restarted.slots[i].state && same(*restarted.slots[i].state, stateFor(i)) &&
                        [restarted.slots[i].name isEqualToString:[NSString stringWithFormat:@"预设 %02zu", i + 1]], "child process restores all persisted slots");
            std::printf("{\"fresh_process_slots_restored\":128}\n"); return 0;
        }
        NSString *directory = [NSTemporaryDirectory() stringByAppendingPathComponent:[[NSUUID UUID] UUIDString]];
        NSURL *url = [NSURL fileURLWithPath:[directory stringByAppendingPathComponent:@"nested/preset-bank.json"]];
        require([pb::defaultURL().path hasSuffix:@"/Library/Application Support/COSMOS/preset-bank-v2.json"], "default URL computes expected path only");
        Bank bank; bank.slots[0].name = @"旧值"; bank.slots[0].state = stateFor(0);
        require(pb::loadBank(url, bank) && !bank.slots[0].state, "missing file loads empty library");
        require(![[NSFileManager defaultManager] fileExistsAtPath:directory], "missing-library load creates no directory");
        State destination = stateFor(9), before = destination; NSString *name = @"unchanged"; NSError *error = nil;
        require(!pb::loadSlot(bank, 0, destination, &name, &error) && same(destination, before) && [name isEqualToString:@"unchanged"], "empty slot leaves sound and name unchanged");
        require(!pb::saveSlot(url, 128, @"bad", State{}, &error), "reject index 128");
        require(!pb::saveSlot(url, size_t(-1), @"bad", State{}, &error), "reject negative wrapped index");
        require(!pb::saveSlot(url, 0, @" \n ", State{}, &error), "reject blank name");
        require(!pb::saveSlot(url, 0, [@"x" stringByPaddingToLength:41 withString:@"x" startingAtIndex:0], State{}, &error), "reject name over forty characters");
        State nonfinite; nonfinite.controls[0] = std::numeric_limits<double>::infinity();
        require(!pb::saveSlot(url, 0, @"bad", nonfinite, &error), "reject invalid state before creating files");
        require(![[NSFileManager defaultManager] fileExistsAtPath:directory], "invalid save creates no files");
        for (size_t i = 0; i < pb::slotCount; ++i) {
            NSString *slotName = [NSString stringWithFormat:@"预设 %02zu", i + 1];
            require(pb::saveSlot(url, i, slotName, stateFor(i), &error), "save each of 128 slots");
        }
        require(pb::loadBank(url, bank, &error), "reload all persisted slots");
        for (size_t i = 0; i < pb::slotCount; ++i) {
            State actual; NSString *actualName = nil;
            require(pb::loadSlot(bank, i, actual, &actualName, &error) && same(actual, stateFor(i)) &&
                    [actualName isEqualToString:[NSString stringWithFormat:@"预设 %02zu", i + 1]], "all slot names and states roundtrip");
        }
        Bank restarted;
        require(pb::loadBank(url, restarted, &error) && same(restarted, bank), "fresh Bank reload simulates process restart");
        NSTask *reader = [NSTask new];reader.executableURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[0]]];reader.arguments = @[url.path];
        NSPipe *childOutput = [NSPipe pipe]; reader.standardOutput = childOutput;
        require([reader launchAndReturnError:&error], "launch isolated second process to check persistence");
        [reader waitUntilExit];
        NSString *childResult = [[NSString alloc] initWithData:[childOutput.fileHandleForReading readDataToEndOfFile] encoding:NSUTF8StringEncoding];
        require(reader.terminationStatus == 0 && [childResult containsString:@"\"fresh_process_slots_restored\":128"], "fresh process restores all 128 slots");
        const Bank original = bank; State replacement = stateFor(3); replacement.program = 12;
        require(pb::saveSlot(url, 17, @"  替换音色  ", replacement, &error) && pb::loadBank(url, bank, &error), "replace selected slot with trimmed name");
        require([bank.slots[17].name isEqualToString:@"替换音色"] && same(*bank.slots[17].state, replacement), "saved name is canonical trimmed text");
        for (size_t i = 0; i < pb::slotCount; ++i) if (i != 17)
            require([bank.slots[i].name isEqualToString:original.slots[i].name] && same(*bank.slots[i].state, *original.slots[i].state), "non-target slots preserved");
        require([pb::normalizedName(@"👨‍👩‍👧‍👦 音色") isEqualToString:@"👨‍👩‍👧‍👦 音色"], "composed emoji name retained");
        NSString *forty = [@"乐" stringByPaddingToLength:40 withString:@"乐" startingAtIndex:0];
        require(pb::normalizedName(forty) && !pb::normalizedName([forty stringByAppendingString:@"乐"]), "forty Unicode characters accepted and forty-one rejected");
        require(!pb::normalizedName(@"音色\n第二行"), "embedded newline rejected");
        require(!pb::normalizedName((NSString *)(id)@3), "nonstring name rejected");
        NSMutableDictionary *d = documentFor(bank); d[@"version"] = @3; invalidDocument(d, bank, "unknown bank version rejected");
        d = documentFor(bank); d[@"version"] = @YES; invalidDocument(d, bank, "boolean bank version rejected");
        d = documentFor(bank); d[@"format"] = @"other"; invalidDocument(d, bank, "other bank format rejected");
        d = documentFor(bank); d[@"audio"] = @[]; invalidDocument(d, bank, "unknown bank field rejected");
        d = documentFor(bank); [d removeObjectForKey:@"slots"]; invalidDocument(d, bank, "missing bank field rejected");
        d = documentFor(bank); [d[@"slots"] removeLastObject]; invalidDocument(d, bank, "incomplete file rejected");
        d = documentFor(bank); d[@"slots"] = @{}; invalidDocument(d, bank, "nonarray slots rejected");
        d = documentFor(bank); d[@"slots"][0] = @"empty"; invalidDocument(d, bank, "nonobject occupied slot rejected");
        d = documentFor(bank); d[@"slots"][0][@"name"] = @""; invalidDocument(d, bank, "empty occupied slot name rejected");
        d = documentFor(bank); d[@"slots"][0][@"name"] = @" x "; invalidDocument(d, bank, "noncanonical disk name rejected");
        d = documentFor(bank); d[@"slots"][0][@"name"] = @3; invalidDocument(d, bank, "numeric occupied slot name rejected");
        d = documentFor(bank); d[@"slots"][0][@"unknown"] = @1; invalidDocument(d, bank, "unknown slot field rejected");
        d = documentFor(bank); [d[@"slots"][0] removeObjectForKey:@"preset"]; invalidDocument(d, bank, "missing slot preset rejected");
        d = documentFor(bank); d[@"slots"][0][@"preset"][@"program"] = @13; invalidDocument(d, bank, "invalid nested preset rejected");
        invalidData([NSData data], bank, "empty bank rejected");
        invalidData([NSMutableData dataWithLength:4194305], bank, "oversized bank rejected");
        invalidData([@"{broken" dataUsingEncoding:NSUTF8StringEncoding], bank, "broken bank JSON rejected");
        invalidData([@"[]" dataUsingEncoding:NSUTF8StringEncoding], bank, "array top-level bank rejected");
        NSString *json = [[NSString alloc] initWithData:pb::encodeData(bank) encoding:NSUTF8StringEncoding];
        invalidData([[json stringByReplacingOccurrencesOfString:@"\"format\" : \"cosmos-preset-bank\"" withString:@"\"format\" : \"cosmos-preset-bank\", \"format\" : \"cosmos-preset-bank\""] dataUsingEncoding:NSUTF8StringEncoding], bank, "duplicate root key rejected");
        invalidData([[json stringByReplacingOccurrencesOfString:@"\"record\" : false" withString:@"\"record\" : false, \"rec\\u006frd\" : true"] dataUsingEncoding:NSUTF8StringEncoding], bank, "escaped duplicate nested flag rejected");
        NSData *const backup = [NSData dataWithContentsOfURL:url];
        NSData *corrupt = [@"{broken bank file}" dataUsingEncoding:NSUTF8StringEncoding];
        require([corrupt writeToURL:url atomically:YES], "install isolated corrupt fixture");
        Bank unchanged = bank;
        require(!pb::loadBank(url, unchanged, &error) && same(unchanged, bank), "corrupt disk library leaves loaded bank unchanged");
        require(!pb::saveSlot(url, 0, @"勿覆盖", State{}, &error) && [[NSData dataWithContentsOfURL:url] isEqualToData:corrupt], "explicit slot save refuses to overwrite corrupt file");
        require([backup writeToURL:url atomically:YES], "restore valid test fixture");
        const int lock = open([[url.path stringByAppendingString:@".lock"] fileSystemRepresentation], O_RDWR);
        require(lock >= 0 && flock(lock, LOCK_EX | LOCK_NB) == 0, "hold independent library save lock");
        require(!pb::saveSlot(url, 0, @"忙碌", State{}, &error) && [[NSData dataWithContentsOfURL:url] isEqualToData:backup], "busy lock returns error and preserves library");
        flock(lock, LOCK_UN); close(lock);
        NSURL *link = [NSURL fileURLWithPath:[directory stringByAppendingPathComponent:@"dangling.json"]];
        require([[NSFileManager defaultManager] createSymbolicLinkAtPath:link.path withDestinationPath:@"missing-target.json" error:&error], "create isolated dangling fixture");
        require(!pb::loadBank(link, unchanged, &error) && !pb::saveSlot(link, 0, @"勿覆盖", State{}, &error) &&
                [[[NSFileManager defaultManager] attributesOfItemAtPath:link.path error:nil][NSFileType] isEqualToString:NSFileTypeSymbolicLink], "unreadable existing symlink never treated as empty library");
        NSURL *concurrentURL = [NSURL fileURLWithPath:[directory stringByAppendingPathComponent:@"concurrent/bank.json"]];
        std::atomic<unsigned> saved{0}; std::atomic<bool> threadFailure{false};
        auto writer = [&](size_t parity) {
            @autoreleasepool {
                for (size_t i = parity; i < pb::slotCount; i += 2) {
                    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
                    while (true) {
                        NSError *writeError = nil;
                        if (pb::saveSlot(concurrentURL, i, [NSString stringWithFormat:@"并发 %02zu", i + 1], stateFor(i), &writeError)) { ++saved; break; }
                        NSError *underlying = writeError.userInfo[NSUnderlyingErrorKey];
                        if (![underlying.domain isEqualToString:NSPOSIXErrorDomain] ||
                            (underlying.code != EWOULDBLOCK && underlying.code != EAGAIN) || std::chrono::steady_clock::now() >= deadline) {
                            threadFailure = true; return;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    }
                }
            }
        };
        std::thread first(writer, 0), second(writer, 1); first.join(); second.join();
        require(!threadFailure && saved == 128, "two concurrent writers save all 128 slots");
        Bank concurrent;
        require(pb::loadBank(concurrentURL, concurrent, &error), "reload concurrently saved library");
        for (size_t i = 0; i < pb::slotCount; ++i)
            require(concurrent.slots[i].state && same(*concurrent.slots[i].state, stateFor(i)), "concurrent non-target slot settings preserved");
        require([[NSFileManager defaultManager] removeItemAtPath:directory error:&error], "remove all isolated fixtures");
        std::printf("{\"suite\":\"preset_bank\",\"passed\":true,\"checks\":%u,\"slots_roundtripped\":128,\"fresh_process_slots_restored\":128,\"concurrent_writers\":2,\"concurrent_slots_preserved\":128,\"real_default_library_modified\":false,\"audio_started\":false}\n", checks);
    }
}
