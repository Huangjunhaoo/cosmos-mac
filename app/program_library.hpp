#pragma once
#import <Cocoa/Cocoa.h>
#include "preset_bank.hpp"

@interface CXLibrary : NSObject
@property(copy) cosmos_presets::State (^capture)(void);
@property(copy) BOOL (^applySound)(const cosmos_presets::State&, NSString*);
@property(copy) BOOL (^legacyRecall)(NSInteger);
@property(copy) NSString* (^legacyName)(NSInteger);
@property(copy) NSDictionary* (^legacyEntry)(NSInteger);
@property(copy) BOOL (^editLegacy)(NSInteger, NSDictionary*, NSString*);
@property(copy) NSInteger (^channel)(void);
@property(copy) void (^setChannelValue)(NSInteger);
@property(readonly) NSInteger active;
@property(copy,readonly) NSString *name;
@property(copy,readonly) NSString *issue;
@property(readonly) NSUInteger revision;
@property(atomic,copy) NSDictionary *baseline;

- (instancetype)initWithURL:(NSURL*)url;
- (BOOL)reload;
- (NSString*)nameAt:(NSInteger)index;
- (BOOL)load:(NSInteger)index;
- (BOOL)save:(NSInteger)index name:(NSString*)name;
- (NSDictionary*)entryAt:(NSInteger)index;
- (BOOL)edit:(NSInteger)index expected:(NSDictionary*)entry newName:(NSString*)name;
- (void)track:(NSInteger)slot name:(NSString*)name;
@end

@interface CXDrawer : NSView <NSTextFieldDelegate>
- (instancetype)initWithParent:(NSView*)parent library:(CXLibrary*)model;
@property(copy) void (^visibilityChanged)(BOOL open);
@property(copy) void (^fileMenu)(NSButton*);
- (void)toggle;
- (void)tick;
@end
