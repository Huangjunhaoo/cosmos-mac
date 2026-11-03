#pragma once
#import <Cocoa/Cocoa.h>
#include <algorithm>
#include <cmath>

// Self-contained, read-only parameter help. Owning controls retain this helper;
// the helper never retains its control and never opens or starts an audio host.
@interface CosmosParameterHover : NSObject
- (instancetype)initWithView:(NSView *)view;
- (void)presentInsideView:(NSView *)surface;
- (void)refreshTracking;
- (void)updateHeading:(NSString *)heading body:(NSString *)body;
- (void)dismiss;
- (void)resumeAfterInteraction;
@end

@interface CosmosParameterHoverPanel : NSPanel @end
@implementation CosmosParameterHoverPanel
- (BOOL)canBecomeKeyWindow { return NO; }
- (BOOL)canBecomeMainWindow { return NO; }
@end

@interface CosmosParameterHoverBubble : NSView
- (NSSize)configureHeading:(NSString *)heading body:(NSString *)body maximumHeight:(CGFloat)maximumHeight;
@end
@implementation CosmosParameterHoverBubble {
    NSTextField *_body;
}
- (NSView *)hitTest:(NSPoint)point { return nil; } // A help bubble never owns the pointer.
- (NSTextField *)labelWithSize:(CGFloat)size weight:(NSFontWeight)weight color:(NSColor *)color {
    NSTextField *field = [NSTextField labelWithString:@""];
    field.font = [NSFont systemFontOfSize:size weight:weight];
    field.textColor = color; field.editable = NO; field.selectable = NO;
    field.bordered = NO; field.bezeled = NO; field.drawsBackground = NO;
    field.maximumNumberOfLines = 0; field.lineBreakMode = NSLineBreakByWordWrapping;
    field.cell.wraps = YES; field.cell.scrollable = NO; field.cell.usesSingleLineMode = NO;
    [self addSubview:field]; return field;
}
- (instancetype)initWithFrame:(NSRect)frame {
    if ((self = [super initWithFrame:frame])) {
        _body = [self labelWithSize:11 weight:NSFontWeightRegular color:[NSColor colorWithSRGBRed:.2 green:.25 blue:.2 alpha:1]];
        self.accessibilityElement = YES; self.accessibilityRole = NSAccessibilityHelpTagRole;
    }
    return self;
}
- (CGFloat)setText:(NSString *)text field:(NSTextField *)field width:(CGFloat)width {
    NSMutableParagraphStyle *style = [NSMutableParagraphStyle new];
    style.lineBreakMode = NSLineBreakByWordWrapping; style.lineSpacing = 1;
    NSAttributedString *content = [[NSAttributedString alloc] initWithString:text attributes:@{
        NSFontAttributeName:field.font, NSForegroundColorAttributeName:field.textColor,
        NSParagraphStyleAttributeName:style}];
    field.attributedStringValue = content;
    // The cell includes fallback-font metrics and text insets that the plain
    // attributed-string bounds can miss (especially for multiline Chinese).
    const CGFloat textHeight = [content boundingRectWithSize:NSMakeSize(width - 4, CGFLOAT_MAX)
        options:NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading].size.height;
    const CGFloat cellHeight = [field.cell cellSizeForBounds:NSMakeRect(0, 0, width, CGFLOAT_MAX)].height;
    return std::ceil(std::max(textHeight, cellHeight)) + 2;
}
- (NSSize)configureHeading:(NSString *)heading body:(NSString *)body maximumHeight:(CGFloat)maximumHeight {
    const CGFloat padding = 10;
    const CGFloat naturalWidth = std::ceil([body sizeWithAttributes:@{NSFontAttributeName:_body.font}].width) + 2 * padding + 4;
    const CGFloat width = std::clamp<CGFloat>(naturalWidth, 76, 208), textWidth = width - 2 * padding;
    CGFloat bodyHeight = [self setText:body field:_body width:textWidth];
    bodyHeight = std::max<CGFloat>(15, std::min(bodyHeight, std::max<CGFloat>(15, maximumHeight - 2 * padding)));
    const CGFloat height = bodyHeight + 2 * padding;
    self.frame = NSMakeRect(0, 0, width, height);
    _body.frame = NSMakeRect(padding, padding, textWidth, bodyHeight);
    self.accessibilityLabel = heading; self.accessibilityHelp = body;
    return NSMakeSize(width, height);
}
- (void)drawRect:(NSRect)dirty {
    (void)dirty;
    NSBezierPath *path = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(self.bounds, .5, .5) xRadius:6 yRadius:6];
    [[NSColor colorWithSRGBRed:.965 green:.976 blue:.955 alpha:.98] setFill]; [path fill];
    [[NSColor colorWithSRGBRed:.77 green:.82 blue:.76 alpha:.65] setStroke]; path.lineWidth = .7; [path stroke];
}
@end

@implementation CosmosParameterHover {
    __weak NSView *_owner, *_surface;
    BOOL _embedded;
    NSTrackingArea *_tracking;
    NSTimer *_timer;
    CosmosParameterHoverPanel *_panel;
    CosmosParameterHoverBubble *_bubble;
    NSMutableArray<id> *_observers;
    NSString *_heading, *_body;
    BOOL _inside, _suppressed;
}
- (instancetype)initWithView:(NSView *)view {
    if ((self = [super init])) {
        _owner = view; _heading = @""; _body = @""; _observers = [NSMutableArray new];
        __weak CosmosParameterHover *weak = self;
        NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
        for (NSNotificationName name in @[NSApplicationDidResignActiveNotification,
                                         NSWindowDidResignKeyNotification, NSWindowWillCloseNotification,
                                         NSWindowWillMiniaturizeNotification]) {
            id token = [center addObserverForName:name object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *notification) {
                CosmosParameterHover *strong = weak;
                if (!strong) return;
                if ([notification.name isEqualToString:NSApplicationDidResignActiveNotification] ||
                    notification.object == strong->_owner.window) [strong dismiss];
            }];
            [_observers addObject:token];
        }
        for (NSNotificationName name in @[NSWindowDidMoveNotification, NSWindowDidResizeNotification]) {
            id token = [center addObserverForName:name object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *notification) {
                CosmosParameterHover *strong = weak;
                if (strong && notification.object == strong->_owner.window && [strong bubbleVisible])
                    [strong positionBubble];
            }];
            [_observers addObject:token];
        }
        [self refreshTracking];
    }
    return self;
}
- (void)presentInsideView:(NSView *)surface {
    [self dismiss];
    _embedded = YES;
    _surface = surface;
}
- (BOOL)bubbleVisible { return _embedded ? _bubble.superview != nil : _panel.isVisible; }
- (BOOL)pointerInside {
    NSView *view = _owner; NSWindow *window = view.window;
    if (!view || !window || view.hiddenOrHasHiddenAncestor || !window.isVisible || window.isMiniaturized) return NO;
    NSPoint point = [view convertPoint:[window convertPointFromScreen:NSEvent.mouseLocation] fromView:nil];
    return NSMouseInRect(point, NSIntersectionRect(view.bounds, view.visibleRect), view.isFlipped);
}
- (BOOL)canDisplay {
    NSView *view = _owner; NSWindow *window = view.window;
    return view && window && (_embedded ? _surface.window == window : NSApp.isActive && window.isKeyWindow) && !window.attachedSheet &&
        !view.hiddenOrHasHiddenAncestor && window.isVisible && !window.isMiniaturized;
}
- (void)refreshTracking {
    NSView *view = _owner;
    if (!view) { [self dismiss]; return; }
    // InVisibleRect follows resizing automatically. Frequent control refreshes
    // must not restart a dwell timer that is already counting down.
    if (!_tracking || ![view.trackingAreas containsObject:_tracking]) {
        if (_tracking) [view removeTrackingArea:_tracking];
        const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited | NSTrackingInVisibleRect |
            NSTrackingActiveInKeyWindow | NSTrackingEnabledDuringMouseDrag;
        _tracking = [[NSTrackingArea alloc] initWithRect:NSZeroRect options:options owner:self userInfo:nil];
        [view addTrackingArea:_tracking];
    }
    _inside = [self pointerInside];
    if (_inside && !_suppressed && [self canDisplay]) [self armAfter:1.2];
    else if (!_inside) [self dismiss];
}
- (void)armAfter:(NSTimeInterval)delay {
    if (_timer || [self bubbleVisible] || _suppressed || !_inside || !_body.length) return;
    __weak CosmosParameterHover *weak = self;
    _timer = [NSTimer timerWithTimeInterval:delay repeats:NO block:^(NSTimer *timer) {
        [weak dwellTimerFired:timer];
    }];
    [[NSRunLoop mainRunLoop] addTimer:_timer forMode:NSRunLoopCommonModes];
}
- (void)dwellTimerFired:(NSTimer *)timer {
    if (timer != _timer) return;
    _timer = nil;
    if (_suppressed || !_inside || ![self pointerInside] || ![self canDisplay]) { [self dismiss]; return; }
    // Entering while dragging may start the dwell, but help never appears
    // until every mouse button is released. Retry without resetting the dwell.
    if (NSEvent.pressedMouseButtons) { [self armAfter:.1]; return; }
    [self showBubble];
}
- (void)mouseEntered:(NSEvent *)event {
    (void)event; _inside = [self pointerInside]; _suppressed = NO;
    if (_inside && [self canDisplay]) [self armAfter:1.2];
}
- (void)mouseExited:(NSEvent *)event {
    (void)event; _inside = NO; [self dismiss];
}
- (void)updateHeading:(NSString *)heading body:(NSString *)body {
    NSString *newHeading = heading ?: @"", *newBody = body ?: @"";
    if ([_heading isEqualToString:newHeading] && [_body isEqualToString:newBody]) return;
    _heading = [newHeading copy]; _body = [newBody copy];
    if (!_body.length) { [self dismiss]; return; }
    if ([self bubbleVisible]) {
        if (!_suppressed && [self canDisplay] && [self pointerInside] && !NSEvent.pressedMouseButtons) [self positionBubble];
        else [self dismiss];
    } else if (_inside && !_suppressed && [self canDisplay]) [self armAfter:1.2];
}
- (NSScreen *)ownerScreen {
    NSView *view = _owner; NSWindow *window = view.window;
    if (!window) return NSScreen.mainScreen;
    const NSRect screenRect = [window convertRectToScreen:[view convertRect:view.bounds toView:nil]];
    const NSPoint center = NSMakePoint(NSMidX(screenRect), NSMidY(screenRect));
    for (NSScreen *screen in NSScreen.screens) if (NSPointInRect(center, screen.frame)) return screen;
    return window.screen ?: NSScreen.mainScreen;
}
- (void)positionBubble {
    NSView *view = _owner;
    if (_embedded) {
        NSView *surface = _surface;
        if (!surface || !view || surface.window != view.window) { [self dismiss]; return; }
        NSRect area = NSInsetRect(surface.bounds, 6, 6);
        NSRect anchor = [view convertRect:view.bounds toView:surface];
        NSSize size = [_bubble configureHeading:_heading body:_body maximumHeight:NSHeight(area)];
        CGFloat x = std::clamp(NSMidX(anchor) - size.width / 2, NSMinX(area), std::max(NSMinX(area), NSMaxX(area) - size.width));
        CGFloat y = surface.isFlipped ? NSMinY(anchor) - size.height - 5 : NSMaxY(anchor) + 5;
        if (y < NSMinY(area) || y + size.height > NSMaxY(area))
            y = surface.isFlipped ? NSMaxY(anchor) + 5 : NSMinY(anchor) - size.height - 5;
        [_bubble setFrameOrigin:NSMakePoint(x, std::clamp(y, NSMinY(area), std::max(NSMinY(area), NSMaxY(area) - size.height)))];
        return;
    }
    NSWindow *window = view.window; NSScreen *screen = [self ownerScreen];
    if (!view || !window || !screen || !_panel) return;
    const NSRect visible = NSInsetRect(screen.visibleFrame, 10, 10);
    const NSSize size = [_bubble configureHeading:_heading body:_body maximumHeight:visible.size.height];
    const NSRect ownerRect = [window convertRectToScreen:[view convertRect:view.bounds toView:nil]];
    CGFloat x = NSMidX(ownerRect) - size.width / 2, y = NSMaxY(ownerRect) + 8;
    if (y + size.height > NSMaxY(visible)) y = NSMinY(ownerRect) - 8 - size.height;
    x = std::max(NSMinX(visible), std::min(x, NSMaxX(visible) - size.width));
    y = std::max(NSMinY(visible), std::min(y, NSMaxY(visible) - size.height));
    [_panel setFrame:NSMakeRect(x, y, size.width, size.height) display:YES];
    _bubble.needsDisplay = YES;
}
- (void)showBubble {
    if (![self canDisplay] || ![self pointerInside] || NSEvent.pressedMouseButtons) return;
    if (_embedded) {
        NSView *surface = _surface;
        if (!surface || surface.window != _owner.window) return;
        if (!_bubble) _bubble = [[CosmosParameterHoverBubble alloc] initWithFrame:NSZeroRect];
        [surface addSubview:_bubble positioned:NSWindowAbove relativeTo:nil];
        [self positionBubble];
        return;
    }
    if (!_panel) {
        _panel = [[CosmosParameterHoverPanel alloc] initWithContentRect:NSMakeRect(0, 0, 208, 60)
                                                            styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
                                                              backing:NSBackingStoreBuffered defer:YES];
        _panel.releasedWhenClosed = NO; _panel.opaque = NO; _panel.backgroundColor = NSColor.clearColor;
        _panel.hasShadow = YES; _panel.hidesOnDeactivate = YES; _panel.ignoresMouseEvents = YES;
        _panel.becomesKeyOnlyIfNeeded = YES; _panel.level = NSFloatingWindowLevel;
        _panel.collectionBehavior = NSWindowCollectionBehaviorTransient | NSWindowCollectionBehaviorIgnoresCycle | NSWindowCollectionBehaviorFullScreenAuxiliary;
        _bubble = [[CosmosParameterHoverBubble alloc] initWithFrame:NSMakeRect(0, 0, 208, 60)];
        _panel.contentView = _bubble;
    }
    NSWindow *ownerWindow = _owner.window;
    if (_panel.parentWindow != ownerWindow) {
        [_panel.parentWindow removeChildWindow:_panel];
        [ownerWindow addChildWindow:_panel ordered:NSWindowAbove];
    }
    [self positionBubble]; [_panel orderFront:nil];
}
- (void)dismiss {
    _suppressed = YES; [_timer invalidate]; _timer = nil;
    if (_embedded) { [_bubble removeFromSuperview]; return; }
    [_panel orderOut:nil];
    [_panel.parentWindow removeChildWindow:_panel];
}
- (void)resumeAfterInteraction {
    _inside = [self pointerInside]; _suppressed = NO;
    if (_inside && [self canDisplay]) [self armAfter:1.2];
    else [self dismiss];
}
- (void)dealloc {
    [_timer invalidate];
    for (id token in _observers) [NSNotificationCenter.defaultCenter removeObserver:token];
    NSView *view = _owner;
    if (_tracking && view) [view removeTrackingArea:_tracking];
    if (_embedded) [_bubble removeFromSuperview];
    else { [_panel orderOut:nil]; [_panel.parentWindow removeChildWindow:_panel]; [_panel close]; }
}
@end
