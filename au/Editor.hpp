#pragma once
#import <CoreAudioKit/CoreAudioKit.h>
#import "Unit.hpp"

@interface CosmosAUView : AUViewController<AUAudioUnitFactory>
@property(nonatomic,copy) void (^hostGesture)(unsigned,BOOL);
- (instancetype)initWithUnit:(CosmosAUUnit*)unit change:(void (^)(unsigned,float))change;
@end
