#pragma once
#include "language.hpp"
#import <Foundation/Foundation.h>
#include "../native/midi_control.hpp"
#include "midi_preset.hpp"
#include <cmath>
namespace cosmos_midi {
struct Settings {bool enabled=false,sync=false,programChange=true;int32_t source=0;unsigned bars=1;Router router;};
inline Preset capturePreset(const Settings& settings){Preset p;p.bindings=settings.router.bindings;p.channel=settings.router.channel;p.programChange=settings.programChange;p.sync=settings.sync;p.bars=settings.bars;return p;}
inline void restorePreset(const Preset& p,Settings& settings){settings.router.release();settings.router.learning=-1;settings.router.bindings=p.bindings;settings.router.channel=p.channel;settings.programChange=p.programChange;settings.sync=p.sync;settings.bars=p.bars;}
// Visible menu order is separate from stable saved MIDI target IDs.
inline NSString* modeName(unsigned bank){return @[CL(@"M1.41 前景声原版"),CL(@"R1.51 节奏版"),CL(@"M1.41 后景声版"),CL(@"前后四声道模式"),CL(@"PM1.61 变速原版"),CL(@"PR1.61 变速节奏版")][bank<6?bank:0];}
inline NSArray<NSString*>* targetNames(unsigned bank=0){
    NSString* current=bank==3?CL(@"前后四声道模式 · 前景声"):modeName(bank);
    NSString* rear=CL(@"前后四声道模式 · 后景声");
    auto name=[](NSString* panel,NSString* parameter){return [panel stringByAppendingFormat:@" · %@",parameter];};
    return @[name(current,@"BLUR"),name(current,@"DRIFT"),name(current,bank>=4?@"SPEED":@"DRIVE"),name(current,@"SUP / COM"),name(current,@"FB"),name(current,@"MIX"),
             name(rear,@"BLUR"),name(rear,@"DRIFT"),name(rear,@"DRIVE"),name(rear,@"SUP / COM"),name(rear,@"FB"),name(rear,@"MIX"),
             name(bank==3?CL(@"前后四声道模式 · 共用"):modeName(bank),CL(@"输入音量")),name(bank==3?CL(@"前后四声道模式 · 共用"):modeName(bank),bank==3?CL(@"总输出音量"):CL(@"输出音量")),
             CL(@"前后四声道模式 · 前景声 · 输出音量"),CL(@"前后四声道模式 · 后景声 · 输出音量"),
             name(current,CL(@"HPF 踏板")),name(current,CL(@"LPF 踏板")),name(current,CL(@"ERASE 踏板")),name(current,CL(@"REVERSE 踏板")),name(current,CL(@"RECORD 踏板")),
             name(rear,CL(@"HPF 踏板")),name(rear,CL(@"LPF 踏板")),name(rear,CL(@"ERASE 踏板")),name(rear,CL(@"REVERSE 踏板")),name(rear,CL(@"RECORD 踏板")),
             name(current,CL(@"音色 1–12")),name(rear,CL(@"音色 1–12")),
             name(bank==3?CL(@"前后四声道模式 · 共用"):current,CL(@"ERASE ALL（清除全部录音）"))];
}
inline NSArray<NSArray<NSNumber*>*>* targetGroups(unsigned bank){
    if(bank==3)return @[@[@0,@1,@2,@3,@4,@5,@26,@16,@17,@18,@19,@20,@14],
                        @[@6,@7,@8,@9,@10,@11,@27,@21,@22,@23,@24,@25,@15],@[@12,@13,@(EraseAllTarget)]];
    return @[@[@0,@1,@2,@3,@4,@5,@26,@16,@17,@18,@19,@20,@(EraseAllTarget)],@[@12,@13]];
}
inline NSArray<NSString*>* targetGroupNames(unsigned bank){
    if(bank==3)return @[CL(@"前后四声道模式 · 前景声"),CL(@"前后四声道模式 · 后景声"),CL(@"前后四声道模式 · 共用控制")];
    return @[[modeName(bank) stringByAppendingString:CL(@" · 面板参数")],[modeName(bank) stringByAppendingString:CL(@" · 输入输出")]];
}
inline NSURL* settingsURL(){return [[[NSFileManager defaultManager]URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask].firstObject URLByAppendingPathComponent:@"COSMOS/midi-settings.json"];}
inline bool integer(id v,int64_t lo,int64_t hi){if(![v isKindOfClass:NSNumber.class])return false;const double d=[v doubleValue];return std::isfinite(d)&&d==std::floor(d)&&d>=lo&&d<=hi;}
inline BOOL settingsError(NSError** error){if(error)*error=[NSError errorWithDomain:@"COSMOS.MIDI" code:1 userInfo:@{NSLocalizedDescriptionKey:CL(@"MIDI 设置文件格式不正确，原文件已保留。")}];return NO;}
inline BOOL loadSettings(NSURL* url,Settings& out,NSError** error){
    if(![[NSFileManager defaultManager]fileExistsAtPath:url.path])return YES;
    NSData* data=[NSData dataWithContentsOfURL:url options:0 error:error];if(!data)return NO;
    if(data.length>65536)return settingsError(error);
    id obj=[NSJSONSerialization JSONObjectWithData:data options:0 error:error];if(!obj)return NO;
    if(![obj isKindOfClass:NSDictionary.class]||![obj[@"format"]isEqual:@"cosmos-midi"]||!integer(obj[@"version"],1,2)||!integer(obj[@"source"],INT32_MIN,INT32_MAX)||!integer(obj[@"channel"],-1,15)||!integer(obj[@"bars"],1,4))return settingsError(error);
    const unsigned count=bindingCount([obj[@"version"]unsignedIntValue]);
    if(![obj[@"bindings"]isKindOfClass:NSArray.class]||[obj[@"bindings"]count]!=count)return settingsError(error);
    for(NSString* key in @[@"enabled",@"sync",@"programChange"])if(!integer(obj[key],0,1))return settingsError(error);
    Settings s;s.enabled=[obj[@"enabled"]boolValue];s.sync=[obj[@"sync"]boolValue];s.programChange=[obj[@"programChange"]boolValue];s.source=[obj[@"source"]intValue];s.router.channel=[obj[@"channel"]intValue];s.bars=[obj[@"bars"]unsignedIntValue];if(s.bars==3)return settingsError(error);
    for(unsigned i=0;i<count;++i){id b=obj[@"bindings"][i];if(![b isKindOfClass:NSDictionary.class]||!integer(b[@"kind"],0,255)||!integer(b[@"channel"],0,15)||!integer(b[@"number"],0,127)||!integer(b[@"pulse"],0,1))return settingsError(error);
        const unsigned kind=[b[@"kind"]unsignedIntValue],number=[b[@"number"]unsignedIntValue];if(kind&&!(kind==0xb0&&number<120)&&!(acceptsPedalMessage(i)&&kind==0x90))return settingsError(error);
        s.router.bindings[i]={uint8_t(kind),uint8_t([b[@"channel"]unsignedIntValue]),uint8_t(number),bool([b[@"pulse"]boolValue])};
        for(unsigned j=0;j<i;++j){const auto& a=s.router.bindings[j];if(kind&&a.kind==kind&&a.channel==s.router.bindings[i].channel&&a.number==number)return settingsError(error);}
    }out=s;return YES;
}
inline BOOL saveSettings(NSURL* url,const Settings& s,NSError** error){
    const unsigned version=bindingVersion(s.router.bindings);
    NSMutableArray* list=[NSMutableArray new];
    for(unsigned i=0;i<bindingCount(version);++i){
        const auto& b=s.router.bindings[i];
        [list addObject:@{@"kind":@(b.kind),@"channel":@(b.channel),@"number":@(b.number),@"pulse":@(b.pulse)}];
    }
    NSDictionary* object=@{@"format":@"cosmos-midi",@"version":@(version),@"enabled":@(s.enabled),@"source":@(s.source),@"channel":@(s.router.channel),@"sync":@(s.sync),@"bars":@(s.bars),@"programChange":@(s.programChange),@"bindings":list};
    NSData* data=[NSJSONSerialization dataWithJSONObject:object options:NSJSONWritingPrettyPrinted error:error];if(!data)return NO;
    if(![[NSFileManager defaultManager]createDirectoryAtURL:url.URLByDeletingLastPathComponent withIntermediateDirectories:YES attributes:nil error:error])return NO;
    return [data writeToURL:url options:NSDataWritingAtomic error:error];
}
}
