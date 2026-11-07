#pragma once
#include "language.hpp"

#include "preset_store.hpp"
#include <optional>
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

// A local library of panel settings, not a startup preset. Reading a Bank has
// no audio/DSP effects; a caller must explicitly loadSlot and apply its State.
namespace cosmos_preset_bank {
constexpr size_t slotCount = 128;
struct Slot {
    NSString *__strong name = @"";
    std::optional<cosmos_presets::State> state;
};
struct Bank { std::array<Slot, slotCount> slots; };

inline bool fail(NSError **error, NSString *message, NSError *underlying = nil) {
    if (error) {
        NSMutableDictionary *info = [@{NSLocalizedDescriptionKey:message} mutableCopy];
        if (underlying) info[NSUnderlyingErrorKey] = underlying;
        *error = [NSError errorWithDomain:@"local.cosmos.preset-bank" code:1 userInfo:info];
    }
    return false;
}

inline NSURL *defaultURL() {
    // This function computes a path only. It does not create Application
    // Support or read/write a user's real preset library.
    return [NSURL fileURLWithPath:[NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support/COSMOS/preset-bank-v2.json"]];
}

inline NSString *normalizedName(NSString *name, NSError **error = nullptr) {
    if (![name isKindOfClass:NSString.class]) {
        fail(error, CL(@"预设名称必须是文字。")); return nil;
    }
    NSString *trimmed = [name stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    __block NSUInteger characters = 0;
    [trimmed enumerateSubstringsInRange:NSMakeRange(0, trimmed.length)
                              options:NSStringEnumerationByComposedCharacterSequences
                           usingBlock:^(NSString *substring, NSRange range, NSRange enclosing, BOOL *stop) {
        (void)substring; (void)range; (void)enclosing;
        if (++characters > 40) *stop = YES;
    }];
    bool controls = false;
    for (NSUInteger index = 0; index < trimmed.length; ++index) {
        const unichar value = [trimmed characterAtIndex:index];
        if (value < 0x20 || (value >= 0x7f && value <= 0x9f) || value == 0x2028 || value == 0x2029) controls = true;
    }
    if (characters < 1 || characters > 40 || controls) {
        fail(error, CL(@"预设名称请填写 1–40 个字符，不要包含换行或控制字符。")); return nil;
    }
    return [trimmed copy];
}

inline bool decode(NSDictionary *document, Bank &destination, NSError **error = nullptr) {
    NSError *detail = nil;
    if (!cosmos_presets::exactKeys(document, @[@"format", @"version", @"slots"], CL(@"预设库"), &detail))
        return fail(error, CL(@"预设库格式不完整或存在未知字段。"), detail);
    if (![document[@"format"] isKindOfClass:NSString.class] ||
        ![document[@"format"] isEqualToString:@"cosmos-preset-bank"])
        return fail(error, CL(@"这个文件不是 COSMOS 预设库。"));
    double version = 0;
    if (!cosmos_presets::number(document[@"version"], 1, 2, true, CL(@"预设库版本（目前支持 1–2）"), version, &detail))
        return fail(error, CL(@"这个预设库的版本无法读取，目前支持版本 1–2。"), detail);
    id slots = document[@"slots"];
    if (![slots isKindOfClass:NSArray.class] || [slots count] != (version == 1 ? 32 : slotCount))
        return fail(error, CL(@"预设库必须包含该版本规定的槽位。"));
    Bank candidate;
    for (size_t index = 0; index < [slots count]; ++index) {
        id entry = slots[index];
        if (entry == NSNull.null) continue;
        if (!cosmos_presets::exactKeys(entry, @[@"name", @"preset"], CL(@"槽位"), &detail))
            return fail(error, [NSString stringWithFormat:CL(@"预设库第 %zu 槽的格式有误。"), index + 1], detail);
        NSString *name = normalizedName(entry[@"name"], &detail);
        if (!name || ![name isEqualToString:entry[@"name"]])
            return fail(error, [NSString stringWithFormat:CL(@"预设库第 %zu 槽的名称无效。"), index + 1], detail);
        cosmos_presets::State state;
        if (!cosmos_presets::decode(entry[@"preset"], state, &detail))
            return fail(error, [NSString stringWithFormat:CL(@"预设库第 %zu 槽的参数无效。"), index + 1], detail);
        candidate.slots[index].name = name;
        candidate.slots[index].state = state;
    }
    destination = candidate; // A corrupt library never partially replaces a Bank.
    return true;
}

inline NSData *encodeData(const Bank &bank, NSError **error = nullptr) {
    NSMutableArray *slots = [NSMutableArray arrayWithCapacity:slotCount];
    for (size_t index = 0; index < slotCount; ++index) {
        const auto &slot = bank.slots[index];
        if (!slot.state) {
            if (slot.name.length) { fail(error, CL(@"空槽位不能包含预设名称。")); return nil; }
            [slots addObject:NSNull.null]; continue;
        }
        NSString *name = normalizedName(slot.name, error);
        if (!name) return nil;
        NSDictionary *preset = cosmos_presets::encode(*slot.state, error);
        if (!preset) return nil;
        [slots addObject:@{@"name":name, @"preset":preset}];
    }
    NSDictionary *document = @{@"format":@"cosmos-preset-bank", @"version":@2, @"slots":slots};
    NSError *detail = nil;
    NSData *data = [NSJSONSerialization dataWithJSONObject:document options:NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys error:&detail];
    if (!data) fail(error, CL(@"无法生成预设库内容。"), detail);
    return data;
}

inline bool decodeData(NSData *data, Bank &bank, NSError **error = nullptr) {
    if (!data.length || data.length > 4194304)
        return fail(error, CL(@"预设库为空或超过 4 MB，无法加载。现有文件会保留。"));
    NSString *text = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    if (!text) return fail(error, CL(@"预设库必须是 UTF-8 编码的 JSON 文件。"));
    NSError *detail = nil;
    id document = [NSJSONSerialization JSONObjectWithData:data options:0 error:&detail];
    if (!document) return fail(error, CL(@"预设库内容损坏，无法读取 JSON。现有文件会保留。"), detail);
    if (!cosmos_presets::UniqueKeys(text, &detail).valid())
        return fail(error, CL(@"预设库包含重复字段或格式异常。现有文件会保留。"), detail);
    return decode(document, bank, error);
}

inline bool loadBank(NSURL *url, Bank &bank, NSError **error = nullptr) {
    if (!url.isFileURL) return fail(error, CL(@"预设库必须位于电脑上的本地文件夹。"));
    NSError *detail = nil;
    NSData *data = [NSData dataWithContentsOfURL:url options:NSDataReadingMappedIfSafe error:&detail];
    if (!data) {
        if ([detail.domain isEqualToString:NSCocoaErrorDomain] && detail.code == NSFileReadNoSuchFileError &&
            ![[NSFileManager defaultManager] attributesOfItemAtPath:url.path error:nil]) {
            if ([url isEqual:defaultURL()]) {
                NSURL *legacy = [url.URLByDeletingLastPathComponent URLByAppendingPathComponent:@"preset-bank.json"];
                return loadBank(legacy, bank, error);
            }
            bank = Bank{}; return true; // First launch is empty; reading creates nothing.
        }
        return fail(error, CL(@"无法读取预设库，请检查文件的读取权限。现有文件会保留。"), detail);
    }
    return decodeData(data, bank, error);
}

inline bool loadSlot(const Bank &bank, size_t index, cosmos_presets::State &destination,
                     NSString *__autoreleasing *name = nullptr, NSError **error = nullptr) {
    if (index >= slotCount) return fail(error, CL(@"请选择 1–128 之间的预设槽位。"));
    const Slot &slot = bank.slots[index];
    if (!slot.state) return fail(error, CL(@"这个槽位还没有保存预设。"));
    NSString *validatedName = normalizedName(slot.name, error);
    if (!validatedName || !cosmos_presets::encode(*slot.state, error)) return false;
    destination = *slot.state;
    if (name) *name = validatedName;
    return true;
}

// A stable sidecar lock serializes saves from cooperating COSMOS app instances.
// Each save rereads disk under the lock before replacing only its chosen slot.
struct FileLock {
    int descriptor = -1;
    ~FileLock() { if (descriptor >= 0) { flock(descriptor, LOCK_UN); close(descriptor); } }
    FileLock() = default;
    FileLock(const FileLock &) = delete;
    FileLock &operator=(const FileLock &) = delete;
};

inline bool saveSlot(NSURL *url, size_t index, NSString *name, const cosmos_presets::State &state,
                     NSError **error = nullptr, bool emptyOnly = false) {
    if (!url.isFileURL) return fail(error, CL(@"预设库必须保存在电脑上的本地文件夹。"));
    if (index >= slotCount) return fail(error, CL(@"请选择 1–128 之间的预设槽位。"));
    NSString *trimmed = normalizedName(name, error);
    if (!trimmed || !cosmos_presets::encode(state, error)) return false;
    NSError *detail = nil;
    if (![[NSFileManager defaultManager] createDirectoryAtURL:url.URLByDeletingLastPathComponent
                                withIntermediateDirectories:YES attributes:nil error:&detail])
        return fail(error, CL(@"无法创建预设库文件夹，请检查保存权限。"), detail);
    NSString *lockPath = [url.path stringByAppendingString:@".lock"];
    FileLock lock;
    lock.descriptor = open(lockPath.fileSystemRepresentation, O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (lock.descriptor < 0)
        return fail(error, CL(@"无法锁定预设库，请检查文件夹权限。"), [NSError errorWithDomain:NSPOSIXErrorDomain code:errno userInfo:nil]);
    if (flock(lock.descriptor, LOCK_EX | LOCK_NB) != 0)
        return fail(error, CL(@"另一个 COSMOS 窗口正在保存预设，请稍后重试。"), [NSError errorWithDomain:NSPOSIXErrorDomain code:errno userInfo:nil]);
    Bank latest;
    if (!loadBank(url, latest, &detail))
        return fail(error, CL(@"现有预设库无法安全读取，已停止保存并保留原文件。"), detail);
    if (emptyOnly && latest.slots[index].state) return fail(error, CL(@"槽位已被使用。"));
    latest.slots[index].name = trimmed;
    latest.slots[index].state = state;
    NSData *data = encodeData(latest, error);
    if (!data) return false;
    if (![data writeToURL:url options:NSDataWritingAtomic error:&detail])
        return fail(error, CL(@"预设保存失败，请检查文件夹权限和可用空间。原预设库会保留。"), detail);
    return true;
}
} // namespace cosmos_preset_bank
