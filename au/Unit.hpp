#pragma once
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#include "Engine.hpp"
#include "../app/program_library.hpp"
@interface CosmosAUUnit : AUAudioUnit
@property(atomic,copy) NSString* cosmosHostName;
- (CXLibrary*)library;
- (cosmos_au::Engine*)engine;
- (void)storeSlot:(unsigned)slot;
- (cosmos_presets::State)soundState;
- (void)restoreSound:(const cosmos_presets::State&)state;
@end
