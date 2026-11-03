#pragma once
#import <Foundation/Foundation.h>
#include <atomic>

namespace cosmos_language {
inline NSBundle* resources(){
#ifdef COSMOS_LIVE_GUIDE
    Class cls=NSClassFromString(@"COSMOSLive_CosmosPanelResources");
#else
    Class cls=NSClassFromString(@"CosmosPanelResources");
#endif
    return cls?[NSBundle bundleForClass:cls]:NSBundle.mainBundle;
}
inline NSArray* languages(){
    static NSArray* langs;static dispatch_once_t once;
    dispatch_once(&once,^{NSURL* url=[resources() URLForResource:@"interface_languages" withExtension:@"json"];
        NSData* data=url?[NSData dataWithContentsOfURL:url]:nil;
        langs=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nil]:nil;
        if(!langs.count)langs=@[@{@"code":@"zh-Hans",@"name":@"简体中文"}];
    });return langs;
}
inline NSArray<NSString*>* codes(){static NSArray* values=[languages() valueForKey:@"code"];return values;}
inline NSArray<NSString*>* names(){static NSArray* values=[languages() valueForKey:@"name"];return values;}
inline NSUserDefaults* settings(){static NSUserDefaults* defaults=[[NSUserDefaults alloc]initWithSuiteName:@"local.cosmos.interface"];return defaults;}
inline std::atomic<unsigned>& selection(){
    static std::atomic<unsigned> index{[] {NSString* value=[settings() stringForKey:@"language"];NSUInteger i=value?[codes() indexOfObject:value]:0;return unsigned(i<codes().count?i:0);}()};
    return index;
}
// A running AU and standalone app may share the selection. A new process
// reads the saved choice; open editors also pick up changes on the UI thread.
inline void syncPreference(){
    static CFAbsoluteTime next=0;
    const auto now=CFAbsoluteTimeGetCurrent();if(now<next)return;next=now+1;
    NSString* value=[settings() stringForKey:@"language"];NSUInteger i=value?[codes() indexOfObject:value]:0;
    if(i>=codes().count)i=0;
    if(selection().exchange(unsigned(i))!=i)
        [[NSNotificationCenter defaultCenter] postNotificationName:@"local.cosmos.interface.languageChanged" object:nil];
}
inline NSDictionary<NSString*,NSArray*>* table(){
    static NSDictionary* translations;static dispatch_once_t once;
    dispatch_once(&once, ^{NSURL* url=[resources() URLForResource:@"languages" withExtension:@"json"];
        NSData* data=url?[NSData dataWithContentsOfURL:url]:nil;translations=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nil]:@{};
    });return translations;
}
inline NSString* text(NSString* key,unsigned lang){
    if(!key||!lang)return key;
    NSArray* row=table()[key];return lang<=row.count?row[lang-1]:key;
}
inline bool rightToLeft(){return [codes()[selection().load()] isEqual:@"ar"];}
inline NSString* changeNotice(){return @"local.cosmos.interface.languageChanged";}
inline void select(NSInteger i){
    if(i<0||i>=NSInteger(codes().count)||selection().exchange(unsigned(i))==unsigned(i))return;
    [settings() setObject:codes()[i] forKey:@"language"];
    [[NSNotificationCenter defaultCenter] postNotificationName:changeNotice() object:nil];
}
}
static inline NSString* CL(NSString* key){return cosmos_language::text(key,cosmos_language::selection().load());}

static inline NSString* LocalizedAudioError(NSString* message){
    if(!message.length||!cosmos_language::selection().load())return message;
    NSArray* row=cosmos_language::table()[message];if(row)return CL(message);
    for(NSString* key in @[@"停止音频流失败：",@"无法创建声卡音频流，错误 ",@"无法启动音频输入输出，错误 ",@"无法读取声卡输入／输出格式，请检查连接后重试（错误 "]){
        if([message hasPrefix:key])return [CL(key) stringByAppendingString:[message substringFromIndex:key.length]];
    }
    return message;
}
