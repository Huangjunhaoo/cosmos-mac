#pragma once
#include "language.hpp"

#import <Foundation/Foundation.h>
#include <array>
#include <cmath>
#include <optional>
#include "midi_preset.hpp"

// Presets contain panel settings and optional portable MIDI mappings/options.
// Audio samples, device identities and transient MIDI gestures are excluded.
namespace cosmos_presets {
struct PanelState {
    int program=1;
    std::array<double,6> controls{{0,0,0,.5,0,0}};
    std::array<unsigned,4> modes{{0,0,0,0}};
    std::array<bool,4> enabled{{false,false,false,false}};
    double outputGainDb=0;
};
struct State {
    bool rhythm = false;
    unsigned alternateBank = 0; // 0: legacy M/R, 1: Q, 2: M+Q, 3: PM, 4: PR.
    unsigned firmwareBank()const { return alternateBank?alternateBank+1:unsigned(rhythm); }
    int program = 1;
    std::array<double, 6> controls{{0, 0, 0, .5, 0, 0}};
    double inputGainDb = 0, outputGainDb = 0;
    // Native group order: REC, REV, HPF, LPF. REV has only mode 0.
    std::array<unsigned, 4> modes{{0, 0, 0, 0}};
    std::array<bool, 4> enabled{{false, false, false, false}};
    PanelState q;
    double mOutputGainDb=0;
    bool mqMerged=true;
    std::optional<cosmos_midi::Preset> midi; // Absent in legacy files: preserve current MIDI settings.
};

inline bool fail(NSError **error, NSString *message, NSError *underlying = nil) {
    if (error) {
        NSMutableDictionary *info = [@{NSLocalizedDescriptionKey: message} mutableCopy];
        if (underlying) info[NSUnderlyingErrorKey] = underlying;
        *error = [NSError errorWithDomain:@"local.cosmos.preset" code:1 userInfo:info];
    }
    return false;
}

inline bool exactKeys(id value, NSArray<NSString *> *keys, NSString *field, NSError **error) {
    if (![value isKindOfClass:NSDictionary.class])
        return fail(error, [NSString stringWithFormat:CL(@"%@必须是一组具名设置。"), field]);
    NSSet *expected = [NSSet setWithArray:keys];
    NSSet *actual = [NSSet setWithArray:[value allKeys]];
    if (![actual isEqualToSet:expected])
        return fail(error, [NSString stringWithFormat:CL(@"%@存在缺失或无法识别的字段。"), field]);
    return true;
}

inline bool number(id value, double low, double high, bool integer,
                   NSString *field, double &result, NSError **error) {
    if (![value isKindOfClass:NSNumber.class] ||
        CFGetTypeID((__bridge CFTypeRef)value) == CFBooleanGetTypeID())
        return fail(error, [NSString stringWithFormat:CL(@"%@必须是数字。"), field]);
    const double candidate = [value doubleValue];
    if (!std::isfinite(candidate) || candidate < low || candidate > high ||
        (integer && (std::floor(candidate) != candidate ||
                     CFNumberIsFloatType((__bridge CFNumberRef)value))))
        return fail(error, [NSString stringWithFormat:CL(@"%@必须是 %@%g 到 %g 之间的有效数字。"), field,
                            integer ? CL(@"整数，范围为 ") : @"", low, high]);
    result = candidate;
    return true;
}

inline bool boolean(id value,bool& result,NSString* field,NSError** error){
    if(![value isKindOfClass:NSNumber.class]||CFGetTypeID((__bridge CFTypeRef)value)!=CFBooleanGetTypeID())return fail(error,[field stringByAppendingString:CL(@"必须为 true 或 false。")]);
    result=[value boolValue];return true;
}
inline bool decodeMIDI(id document,cosmos_midi::Preset& destination,NSError** error){
    if(!exactKeys(document,@[@"version",@"channel",@"program_change",@"clock_sync",@"record_bars",@"bindings"],CL(@"MIDI 设置"),error))return false;
    cosmos_midi::Preset candidate;double value=0;
    if(!number(document[@"version"],1,2,true,CL(@"MIDI 预设版本"),value,error))return false;
    const unsigned count=cosmos_midi::bindingCount(unsigned(value));
    if(!number(document[@"channel"],-1,15,true,CL(@"MIDI 接收通道"),value,error))return false;
    candidate.channel=int(value);
    if(!boolean(document[@"program_change"],candidate.programChange,CL(@"MIDI 预设切换"),error)||!boolean(document[@"clock_sync"],candidate.sync,CL(@"MIDI Clock 同步"),error)||!number(document[@"record_bars"],1,4,true,CL(@"录音小节数"),value,error))return false;
    candidate.bars=unsigned(value);if(candidate.bars==3)return fail(error,CL(@"MIDI 录音小节数须为 1、2 或 4。"));
    id bindings=document[@"bindings"];if(![bindings isKindOfClass:NSArray.class]||[bindings count]!=count)return fail(error,CL(@"MIDI 绑定列表不完整。"));
    for(unsigned i=0;i<count;++i){id entry=bindings[i];auto& b=candidate.bindings[i];
        if(!exactKeys(entry,@[@"kind",@"channel",@"number",@"pulse"],CL(@"MIDI 参数绑定"),error)||!number(entry[@"kind"],0,255,true,CL(@"MIDI 消息类型"),value,error))return false;b.kind=uint8_t(value);
        if(!number(entry[@"channel"],0,15,true,CL(@"MIDI 绑定通道"),value,error))return false;b.channel=uint8_t(value);
        if(!number(entry[@"number"],0,127,true,CL(@"MIDI 控制编号"),value,error))return false;b.number=uint8_t(value);
        if(!boolean(entry[@"pulse"],b.pulse,CL(@"MIDI 踏板点按"),error))return false;
        if(b.kind&&!(b.kind==0xb0&&b.number<120)&&!(b.kind==0x90&&cosmos_midi::acceptsPedalMessage(i)))return fail(error,CL(@"这个参数的 MIDI 消息类型或编号不受支持。"));
        for(unsigned j=0;j<i;++j){const auto& a=candidate.bindings[j];if(b.kind&&a.kind==b.kind&&a.channel==b.channel&&a.number==b.number)return fail(error,CL(@"同一条 MIDI 消息不能重复绑定多个参数。"));}
    }
    destination=candidate;return true;
}
inline NSDictionary* encodeMIDI(const cosmos_midi::Preset& settings){
    const unsigned version=cosmos_midi::bindingVersion(settings.bindings);
    NSMutableArray* bindings=[NSMutableArray new];
    for(unsigned i=0;i<cosmos_midi::bindingCount(version);++i){
        const auto& b=settings.bindings[i];
        [bindings addObject:@{@"kind":@(b.kind),@"channel":@(b.channel),@"number":@(b.number),@"pulse":@(b.pulse)}];
    }
    return @{@"version":@(version),@"channel":@(settings.channel),@"program_change":@(settings.programChange),@"clock_sync":@(settings.sync),@"record_bars":@(settings.bars),@"bindings":bindings};
}

inline bool decode(NSDictionary *document, State &destination, NSError **error = nullptr) {
    if([document isKindOfClass:NSDictionary.class]&&[document[@"version"]isKindOfClass:NSNumber.class]&&[document[@"version"]intValue]==5){
        double version=0;if(!number(document[@"version"],5,5,true,CL(@"预设版本"),version,error))return false;
        const bool quad=[document[@"bank"]isEqual:@"MQ"];
        NSArray* keys=quad?@[@"format",@"version",@"bank",@"program",@"controls",@"gains",@"modes",@"enabled",@"mq",@"midi"]:@[@"format",@"version",@"bank",@"program",@"controls",@"gains",@"modes",@"enabled",@"midi"];
        if(!exactKeys(document,keys,CL(@"包含 MIDI 的预设"),error))return false;
        cosmos_midi::Preset midi;if(!decodeMIDI(document[@"midi"],midi,error))return false;
        NSMutableDictionary* base=[document mutableCopy];[base removeObjectForKey:@"midi"];
        base[@"version"]=quad?@3:[document[@"bank"]isEqual:@"Q"]?@2:([document[@"bank"]isEqual:@"PM"]||[document[@"bank"]isEqual:@"PR"])?@4:@1;
        State candidate;if(!decode(base,candidate,error))return false;
        candidate.midi=midi;destination=candidate;return true;
    }

    // Version3 adds an independent Q panel and MQ mixer. Decode into temporary
    // states so a bad second panel never partially changes the first one.
    if([document isKindOfClass:NSDictionary.class]&&[document[@"version"]isKindOfClass:NSNumber.class]&&[document[@"version"]intValue]==3){
        if(!exactKeys(document,@[@"format",@"version",@"bank",@"program",@"controls",@"gains",@"modes",@"enabled",@"mq"],CL(@"双面板预设"),error))return false;
        double version=0;if(!number(document[@"version"],3,3,true,CL(@"预设版本"),version,error))return false;
        if(![document[@"bank"]isEqual:@"MQ"])return fail(error,CL(@"双面板预设必须使用前后四声道模式。"));
        NSMutableDictionary* base=[document mutableCopy];[base removeObjectForKey:@"mq"];base[@"version"]=@2;
        State candidate;if(!decode(base,candidate,error))return false;
        id mixer=document[@"mq"];
        if(!exactKeys(mixer,@[@"q",@"m_output_db",@"merged_output"],CL(@"前后四声道设置"),error))return false;
        id q=mixer[@"q"];
        if(!exactKeys(q,@[@"program",@"controls",@"modes",@"enabled",@"output_db"],CL(@"后景声面板"),error))return false;
        NSDictionary* qdoc=@{@"format":@"cosmos-preset",@"version":@2,@"bank":@"Q",@"program":q[@"program"],@"controls":q[@"controls"],@"modes":q[@"modes"],@"enabled":q[@"enabled"],@"gains":@{@"input_db":@0,@"output_db":q[@"output_db"]}};
        State qstate;if(!decode(qdoc,qstate,error))return false;
        candidate.q.program=qstate.program;candidate.q.controls=qstate.controls;candidate.q.modes=qstate.modes;candidate.q.enabled=qstate.enabled;candidate.q.outputGainDb=qstate.outputGainDb;
        if(!number(mixer[@"m_output_db"],-36,12,false,CL(@"前景声输出"),candidate.mOutputGainDb,error))return false;
        id flag=mixer[@"merged_output"];
        if(![flag isKindOfClass:NSNumber.class]||CFGetTypeID((__bridge CFTypeRef)flag)!=CFBooleanGetTypeID())return fail(error,CL(@"合并输出必须为 true 或 false。"));
        candidate.mqMerged=[flag boolValue];destination=candidate;return true;
    }
    if (!exactKeys(document, @[@"format", @"version", @"bank", @"program", @"controls", @"gains", @"modes", @"enabled"], CL(@"预设文件"), error)) return false;
    if (![document[@"format"] isKindOfClass:NSString.class] ||
        ![document[@"format"] isEqualToString:@"cosmos-preset"])
        return fail(error, CL(@"这个文件不是 COSMOS 预设。"));
    double value = 0;
    if (!number(document[@"version"], 1, 4, true, CL(@"预设版本"), value, error)) return false;
    const unsigned version=unsigned(value);
    id bank = document[@"bank"];
    if (![bank isKindOfClass:NSString.class] ||
        (![bank isEqualToString:@"M"] && ![bank isEqualToString:@"R"] && ![bank isEqualToString:@"Q"] && ![bank isEqualToString:@"MQ"] && ![bank isEqualToString:@"PM"] && ![bank isEqualToString:@"PR"]))
        return fail(error, CL(@"无法识别这个固件组。"));
    State candidate;
    candidate.rhythm = [bank isEqualToString:@"R"]||[bank isEqualToString:@"PR"];
    candidate.alternateBank=[bank isEqualToString:@"Q"]?1u:[bank isEqualToString:@"MQ"]?2u:[bank isEqualToString:@"PM"]?3u:[bank isEqualToString:@"PR"]?4u:0u;
    if(version!=(candidate.alternateBank>=3?4u:candidate.alternateBank?2u:1u))return fail(error,CL(@"固件组与预设版本不匹配。"));
    if (!number(document[@"program"], 1, 12, true, CL(@"音色编号"), value, error)) return false;
    candidate.program = int(value);
    NSArray *controlNames = @[@"blur", @"drift", @"drive", @"suppression_compression", @"feedback", @"mix"];
    id controls = document[@"controls"];
    if (!exactKeys(controls, controlNames, CL(@"旋钮参数"), error)) return false;
    for (unsigned i = 0; i < candidate.controls.size(); ++i)
        if (!number(controls[controlNames[i]], 0, 1, false, controlNames[i], candidate.controls[i], error)) return false;
    id gains = document[@"gains"];
    if (!exactKeys(gains, @[@"input_db", @"output_db"], CL(@"输入输出增益"), error) ||
        !number(gains[@"input_db"], -36, 12, false, CL(@"输入增益（dB）"), candidate.inputGainDb, error) ||
        !number(gains[@"output_db"], -36, 12, false, CL(@"输出增益（dB）"), candidate.outputGainDb, error)) return false;
    NSArray *groups = @[@"record", @"reverse", @"hpf", @"lpf"];
    id modes = document[@"modes"], enabled = document[@"enabled"];
    if (!exactKeys(modes, groups, CL(@"模式设置"), error) || !exactKeys(enabled, groups, CL(@"开关设置"), error)) return false;
    for (unsigned i = 0; i < candidate.modes.size(); ++i) {
        if (!number(modes[groups[i]], 0, i == 1 ? 0 : 2, true,
                    [groups[i] stringByAppendingString:CL(@" 模式")], value, error)) return false;
        candidate.modes[i] = unsigned(value);
        id flag = enabled[groups[i]];
        if (![flag isKindOfClass:NSNumber.class] ||
            CFGetTypeID((__bridge CFTypeRef)flag) != CFBooleanGetTypeID())
            return fail(error, [NSString stringWithFormat:CL(@"%@ 开关必须是 true 或 false。"), groups[i]]);
        candidate.enabled[i] = [flag boolValue];
    }
    if(candidate.alternateBank==2){
        // Old MQ presets controlled both engines together; preserve that sound.
        candidate.q.program=candidate.program;candidate.q.controls=candidate.controls;
        candidate.q.modes=candidate.modes;candidate.q.enabled=candidate.enabled;
        candidate.mqMerged=false;
    }
    destination = candidate; // Invalid files never partially replace live state.
    return true;
}

inline NSDictionary *encode(const State &state, NSError **error = nullptr) {
    if(state.alternateBank>4)return fail(error,CL(@"无法识别的固件组。")),nil;
    NSDictionary *document = @{
        @"format": @"cosmos-preset", @"version": @(state.alternateBank>=3?4:state.alternateBank?2:1),
        @"bank": state.alternateBank==1?@"Q":state.alternateBank==2?@"MQ":state.alternateBank==3?@"PM":state.alternateBank==4?@"PR":state.rhythm?@"R":@"M", @"program": @(state.program),
        @"controls": @{@"blur": @(state.controls[0]), @"drift": @(state.controls[1]),
            @"drive": @(state.controls[2]), @"suppression_compression": @(state.controls[3]),
            @"feedback": @(state.controls[4]), @"mix": @(state.controls[5])},
        @"gains": @{@"input_db": @(state.inputGainDb), @"output_db": @(state.outputGainDb)},
        @"modes": @{@"record": @(state.modes[0]), @"reverse": @(state.modes[1]),
            @"hpf": @(state.modes[2]), @"lpf": @(state.modes[3])},
        @"enabled": @{@"record": @(state.enabled[0]), @"reverse": @(state.enabled[1]),
            @"hpf": @(state.enabled[2]), @"lpf": @(state.enabled[3])}
    };
    if(state.alternateBank==2){
        State qstate;qstate.alternateBank=1;qstate.program=state.q.program;qstate.controls=state.q.controls;qstate.modes=state.q.modes;qstate.enabled=state.q.enabled;qstate.outputGainDb=state.q.outputGainDb;
        NSDictionary* qdoc=encode(qstate,error);if(!qdoc)return nil;
        NSMutableDictionary* extended=[document mutableCopy];extended[@"version"]=@3;
        extended[@"mq"]=@{@"m_output_db":@(state.mOutputGainDb),@"merged_output":@(state.mqMerged),@"q":@{@"program":qdoc[@"program"],@"controls":qdoc[@"controls"],@"modes":qdoc[@"modes"],@"enabled":qdoc[@"enabled"],@"output_db":@(state.q.outputGainDb)}};
        document=extended;
    }
    if(state.midi){NSMutableDictionary* extended=[document mutableCopy];extended[@"version"]=@5;extended[@"midi"]=encodeMIDI(*state.midi);document=extended;}
    State validated;
    return decode(document, validated, error) ? document : nil;
}

// NSJSONSerialization silently keeps one value when an object repeats a key.
// After normal JSON syntax validation, this small walker rejects such ambiguous
// files, including escaped spellings such as "record" and "rec\u006frd".
class UniqueKeys {
    NSString *text_;
    NSUInteger offset_ = 0;
    NSError *__autoreleasing *error_;
    void space() { while (offset_ < text_.length && [[NSCharacterSet whitespaceAndNewlineCharacterSet] characterIsMember:[text_ characterAtIndex:offset_]]) ++offset_; }
    NSString *string() {
        if (offset_ >= text_.length || [text_ characterAtIndex:offset_] != '"') return nil;
        const NSUInteger start = offset_++;
        bool closed = false;
        while (offset_ < text_.length) {
            const unichar ch = [text_ characterAtIndex:offset_++];
            if (ch == '\\') { if (offset_ < text_.length) ++offset_; }
            else if (ch == '"') { closed = true; break; }
        }
        if (!closed) return nil;
        NSString *token = [text_ substringWithRange:NSMakeRange(start, offset_ - start)];
        NSData *data = [[@"[" stringByAppendingString:[token stringByAppendingString:@"]"]] dataUsingEncoding:NSUTF8StringEncoding];
        NSArray *decoded = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
        return [decoded.firstObject isKindOfClass:NSString.class] ? decoded.firstObject : nil;
    }
    bool value(unsigned depth) {
        if (depth > 32) return fail(error_, CL(@"预设内容嵌套过深，无法加载。"));
        space();
        if (offset_ >= text_.length) return fail(error_, CL(@"预设内容不完整。"));
        const unichar ch = [text_ characterAtIndex:offset_];
        if (ch == '"') return string() != nil || fail(error_, CL(@"预设中的文本格式有误。"));
        if (ch == '{' || ch == '[') {
            const bool object = ch == '{'; const unichar end = object ? '}' : ']';
            ++offset_; space();
            NSMutableSet *keys = object ? [NSMutableSet set] : nil;
            if (offset_ < text_.length && [text_ characterAtIndex:offset_] == end) { ++offset_; return true; }
            while (offset_ < text_.length) {
                space();
                if (object) {
                    NSString *key = string();
                    if (!key) return fail(error_, CL(@"预设字段名称格式有误。"));
                    if ([keys containsObject:key]) return fail(error_, [NSString stringWithFormat:CL(@"预设重复设置了“%@”，请修正后再加载。"), key]);
                    [keys addObject:key]; space();
                    if (offset_ >= text_.length || [text_ characterAtIndex:offset_++] != ':') return fail(error_, CL(@"预设字段缺少冒号。"));
                }
                if (!value(depth + 1)) return false;
                space();
                if (offset_ >= text_.length) return fail(error_, CL(@"预设内容不完整。"));
                const unichar separator = [text_ characterAtIndex:offset_++];
                if (separator == end) return true;
                if (separator != ',') return fail(error_, CL(@"预设字段分隔格式有误。"));
            }
            return fail(error_, CL(@"预设内容不完整。"));
        }
        const NSUInteger start = offset_;
        while (offset_ < text_.length) {
            const unichar c = [text_ characterAtIndex:offset_];
            if (c == ',' || c == '}' || c == ']' || [[NSCharacterSet whitespaceAndNewlineCharacterSet] characterIsMember:c]) break;
            ++offset_;
        }
        return offset_ > start || fail(error_, CL(@"预设内容包含空值或多余的逗号。"));
    }
public:
    UniqueKeys(NSString *text, NSError *__autoreleasing *error): text_(text), error_(error) {}
    bool valid() {
        if (text_.length && [text_ characterAtIndex:0] == 0xFEFF) ++offset_;
        if (!value(0)) return false;
        space();
        return offset_ == text_.length || fail(error_, CL(@"预设文件末尾存在多余内容。"));
    }
};

inline NSData *encodeData(const State &state, NSError **error = nullptr) {
    NSDictionary *document = encode(state, error);
    if (!document) return nil;
    NSError *jsonError = nil;
    NSData *data = [NSJSONSerialization dataWithJSONObject:document options:NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys error:&jsonError];
    if (!data) fail(error, CL(@"无法生成预设文件。"), jsonError);
    return data;
}

inline bool decodeData(NSData *data, State &state, NSError **error = nullptr) {
    if (!data.length || data.length > 65536) return fail(error, CL(@"预设文件为空或超过 64 KB，无法加载。"));
    NSString *text = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    if (!text) return fail(error, CL(@"预设必须是 UTF-8 编码的 JSON 文件。"));
    NSError *jsonError = nil;
    id document = [NSJSONSerialization JSONObjectWithData:data options:0 error:&jsonError];
    if (!document) return fail(error, CL(@"预设格式有误，无法读取 JSON 内容。"), jsonError);
    if (!UniqueKeys(text, error).valid()) return false;
    return decode(document, state, error);
}

inline bool save(NSURL *url, const State &state, NSError **error = nullptr) {
    if (!url.isFileURL) return fail(error, CL(@"请选择电脑上的预设文件保存位置。"));
    NSData *data = encodeData(state, error);
    if (!data) return false;
    NSError *writeError = nil;
    if (![data writeToURL:url options:NSDataWritingAtomic error:&writeError])
        return fail(error, CL(@"预设保存失败，请检查文件夹权限和可用空间。"), writeError);
    return true;
}

inline bool load(NSURL *url, State &state, NSError **error = nullptr) {
    if (!url.isFileURL) return fail(error, CL(@"请选择电脑上的 COSMOS 预设文件。"));
    NSError *readError = nil;
    NSData *data = [NSData dataWithContentsOfURL:url options:NSDataReadingMappedIfSafe error:&readError];
    if (!data) return fail(error, CL(@"无法打开预设文件，请检查文件是否存在及读取权限。"), readError);
    return decodeData(data, state, error);
}
} // namespace cosmos_presets
