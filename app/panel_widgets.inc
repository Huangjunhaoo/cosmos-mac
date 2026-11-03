#include "language.hpp"
// Shared native panel controls and typography for standalone and AU.
@interface CosmosPanelResources : NSObject @end
@implementation CosmosPanelResources @end
static NSColor *Ink(){return [NSColor colorWithSRGBRed:.12 green:.12 blue:.105 alpha:1];}
static NSColor *Muted(){return [NSColor colorWithSRGBRed:.43 green:.43 blue:.38 alpha:1];}
static NSColor *Paper(){return [NSColor colorWithSRGBRed:.935 green:.95 blue:.916 alpha:1];}
static NSColor *Teal(){return [NSColor colorWithSRGBRed:.13 green:.45 blue:.44 alpha:1];}
static NSColor *Coral(){return [NSColor colorWithSRGBRed:.84 green:.34 blue:.24 alpha:1];}
static NSColor *Track(){return [NSColor colorWithSRGBRed:.85 green:.87 blue:.84 alpha:1];}
static NSColor *Amber(){return [NSColor colorWithSRGBRed:1 green:.76 blue:.08 alpha:1];}
static NSString *Text(const std::string& s){return [[NSString alloc] initWithBytes:s.data() length:s.size() encoding:NSUTF8StringEncoding]?:@"";}
static NSFont *Font(CGFloat size,NSFontWeight weight=NSFontWeightRegular){return [NSFont systemFontOfSize:size weight:weight];}
#include "language_ui.inc"
static void DrawText(NSString *text,NSRect rect,NSFont *font,NSColor *color,NSTextAlignment alignment=NSTextAlignmentCenter){
    NSMutableParagraphStyle *style=[NSMutableParagraphStyle new];style.alignment=alignment;
    [text drawInRect:rect withAttributes:@{NSFontAttributeName:font,NSForegroundColorAttributeName:color,NSParagraphStyleAttributeName:style}];
}
static NSString *BankCode(NSUInteger bank){return bank==1?@"R":bank==2?CL(@"后景声 "):bank==3?CL(@"前后四声道 "):bank==4?@"PM":bank==5?@"PR":CL(@"前景声 ");}
static NSDictionary *ProgramInfo(NSUInteger bank,NSInteger selected){
    static NSArray *programs;static dispatch_once_t once;
    dispatch_once(&once,^{
        NSString *path=[[NSBundle bundleForClass:CosmosPanelResources.class]pathForResource:@"program_descriptions" ofType:@"json"];
        NSData *data=path?[NSData dataWithContentsOfFile:path]:nil;
        NSDictionary *catalog=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nil]:nil;
        programs=[catalog[@"programs"]isKindOfClass:[NSArray class]]?catalog[@"programs"]:@[];
    });
    for(NSDictionary *entry in programs)if([entry[@"bank"]isEqualToString:(bank==1||bank==5)?@"R":@"M"]&&[entry[@"program"]integerValue]==selected+1){
        NSString* name=entry[@"short_name"];NSNumber* original=entry[@"name_from_original_program"];
        if(original)for(NSDictionary* m in programs)if([m[@"bank"] isEqual:@"M"]&&[m[@"program"] isEqual:original]){name=m[@"short_name"];break;}
        NSMutableDictionary* info=[entry mutableCopy];info[@"short_name"]=CL(name);
        if(original){
            NSString* family=bank==5?@"PM1.61":@"M1.41";
            if(cosmos_language::rightToLeft())family=[NSString stringWithFormat:@"\u2066%@\u2069",family];
            info[@"reference"]=[NSString stringWithFormat:CL(@"对应原版 %@ · 第 %ld 档"),family,(long)original.integerValue];
        }
        info[@"description"]=CL(entry[@"description"]);
        if(bank==2)info[@"description"]=[info[@"description"] stringByAppendingString:CL(@"（后景声版时长）")];
        if(bank==3)info[@"description"]=CL(@"前景声与后景声各占一组立体声，两套面板独立控制、独立录音。");
        return info;
    }
    return @{@"short_name":CL(@"音色"),@"description":@""};
}
static NSString *ParameterHelp(NSUInteger bank,NSInteger selected,unsigned lane){
    if(bank>=4&&lane==2)return CL(@"控制录音和播放速度；向右升至正常速度，向左降至半速、低一个八度，带有数字颗粒感。");
    static NSDictionary *catalog;static dispatch_once_t once;
    dispatch_once(&once,^{
        NSString *path=[[NSBundle bundleForClass:CosmosPanelResources.class]pathForResource:@"parameter_help" ofType:@"json"];
        NSData *data=path?[NSData dataWithContentsOfFile:path]:nil;
        catalog=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nil]:nil;
    });
    NSArray *keys=@[@"blur",@"drift",@"drive",@"suppression_compression",@"feedback",@"mix"];
    if(lane>=keys.count)return @"";NSString *body=nil;
    for(NSDictionary *entry in catalog[@"programs"]){if([entry[@"bank"]isEqualToString:(bank==1||bank==5)?@"R":@"M"]&&[entry[@"program"]integerValue]==selected+1){body=entry[@"controls"][keys[lane]];break;}}
    if(![body isKindOfClass:NSString.class])for(NSDictionary *entry in catalog[@"controls"])if([entry[@"key"]isEqualToString:keys[lane]]){body=entry[@"common_text"];break;}
    if(![body isKindOfClass:NSString.class])body=CL(@"当前音色的参数说明暂不可用。");
    return CL(body);
}
static NSInteger WrapProgram(NSInteger value){const NSInteger position=value%12;return position<0?position+12:position;}
static NSTextField *Label(NSString *text,CGFloat size=13,NSColor *color=nil){
    NSTextField *v=[NSTextField labelWithString:text];v.font=Font(size);v.textColor=color?:Ink();return v;
}
static NSStackView *Stack(NSArray<NSView *> *views,NSUserInterfaceLayoutOrientation orientation,CGFloat spacing){
    NSStackView *s=[NSStackView stackViewWithViews:views];s.orientation=orientation;s.spacing=spacing;s.alignment=orientation==NSUserInterfaceLayoutOrientationVertical?NSLayoutAttributeCenterX:NSLayoutAttributeCenterY;
    s.translatesAutoresizingMaskIntoConstraints=NO;return s;
}
static void Fix(NSView *view,CGFloat width,CGFloat height){
    view.translatesAutoresizingMaskIntoConstraints=NO;
    if(width>0)[view.widthAnchor constraintEqualToConstant:width].active=YES;
    if(height>0)[view.heightAnchor constraintEqualToConstant:height].active=YES;
}
static void Place(NSView *view,NSView *parent,NSRect frame){
    [view removeConstraints:view.constraints];view.translatesAutoresizingMaskIntoConstraints=YES;view.frame=frame;[parent addSubview:view];
}
static void PlaceAt(NSView *view,NSView *parent,NSPoint point,NSPoint anchor,NSSize size,CGFloat scale=1){
    Place(view,parent,NSMakeRect(point.x-anchor.x*scale,point.y-anchor.y*scale,size.width*scale,size.height*scale));view.bounds=NSMakeRect(0,0,size.width,size.height);
}
static void Lamp(NSPoint center,BOOL on,CGFloat radius=3.6){
    NSBezierPath *rim=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(center.x-radius-1,center.y-radius-1,2*radius+2,2*radius+2)];
    [[NSColor colorWithWhite:.22 alpha:1]setFill];[rim fill];
    NSBezierPath *lens=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(center.x-radius,center.y-radius,2*radius,2*radius)];
    [NSGraphicsContext saveGraphicsState];
    if(on){NSShadow *glow=[NSShadow new];glow.shadowColor=[Amber()colorWithAlphaComponent:.65];glow.shadowBlurRadius=8;glow.shadowOffset=NSZeroSize;[glow set];}
    NSGradient *glass=[[NSGradient alloc]initWithStartingColor:on?[NSColor colorWithSRGBRed:1 green:.94 blue:.54 alpha:1]:[NSColor colorWithSRGBRed:.61 green:.53 blue:.28 alpha:1]
        endingColor:on?Amber():[NSColor colorWithSRGBRed:.35 green:.29 blue:.12 alpha:1]];
    [glass drawInBezierPath:lens angle:-90];[NSGraphicsContext restoreGraphicsState];
    NSBezierPath *shine=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(center.x-radius*.45,center.y+radius*.12,radius*.7,radius*.4)];
    [[NSColor colorWithWhite:1 alpha:on?.8:.25]setFill];[shine fill];
}

@interface CosmosWordmark : NSView @end
#include "wordmark.inc"
@implementation CosmosWordmark
- (instancetype)initWithFrame:(NSRect)frame{
    if((self=[super initWithFrame:frame])){Fix(self,205,34);self.accessibilityElement=YES;self.accessibilityRole=NSAccessibilityImageRole;self.accessibilityLabel=@"COSMOS 标识";}return self;
}
- (void)drawRect:(NSRect)rect{
    CGContextRef ctx=NSGraphicsContext.currentContext.CGContext;
    CGContextSaveGState(ctx);
    CGContextTranslateCTM(ctx,NSMinX(self.bounds),NSMinY(self.bounds));
    CGContextScaleCTM(ctx,NSWidth(self.bounds)/102.,NSHeight(self.bounds)/19.);
    CGContextSetRGBFillColor(ctx,.12,.12,.105,1);
    FillCosmosWordmark(ctx);
    CGContextRestoreGState(ctx);
}
@end

@interface PaperView : NSView @end
@implementation PaperView
- (void)drawRect:(NSRect)r{[Paper() setFill];NSRectFill(self.bounds);}
@end

static NSArray<NSString*> *ModeTitles(unsigned group){
    if(group==0)return @[CL(@"第一组"),CL(@"第二组"),CL(@"全部组")];
    if(group==3)return @[CL(@"高强度"),CL(@"中强度"),CL(@"低强度")];
    return @[CL(@"低强度"),CL(@"中强度"),CL(@"高强度")];
}
@interface ModeLabel : NSControl
@property(nonatomic,copy) NSString *title;
@property(nonatomic) NSInteger group;
@property(nonatomic) NSInteger mode;
@property(nonatomic,copy) void(^clicked)(ModeLabel*);
@end
@implementation ModeLabel
- (instancetype)initWithTitle:(NSString*)title group:(NSInteger)group{
    if((self=[super initWithFrame:NSZeroRect])){_title=title;_group=group;self.accessibilityElement=YES;self.accessibilityRole=NSAccessibilityPopUpButtonRole;self.accessibilityLabel=[title stringByAppendingString:CL(@"模式")];self.mode=0;}return self;
}
- (void)setMode:(NSInteger)value{_mode=std::clamp<NSInteger>(value,0,2);self.toolTip=[NSString stringWithFormat:CL(@"%@：%@，点击选择"),_title,ModeTitles(unsigned(_group))[_mode]];self.needsDisplay=YES;}
- (id)accessibilityValue{return ModeTitles(unsigned(_group))[_mode];}
- (void)drawRect:(NSRect)r{
    NSFont *font=Font(11,NSFontWeightSemibold);const CGFloat width=[_title sizeWithAttributes:@{NSFontAttributeName:font}].width;
    const CGFloat x=(self.bounds.size.width-width-12)/2;
    DrawText(_title,NSMakeRect(x,3,width,17),font,Ink());
    NSBezierPath *arrow=[NSBezierPath bezierPath];[arrow moveToPoint:NSMakePoint(x+width+5,13)];[arrow lineToPoint:NSMakePoint(x+width+11,13)];[arrow lineToPoint:NSMakePoint(x+width+8,9)];[arrow closePath];[Ink()setFill];[arrow fill];
}
- (BOOL)acceptsFirstResponder{return YES;}
- (void)mouseDown:(NSEvent*)event{[self.window makeFirstResponder:self];if(_clicked)_clicked(self);}
- (void)mouseUp:(NSEvent*)event{} // A menu click must never press or release the pedal.
- (void)keyDown:(NSEvent*)event{if(event.keyCode==49||event.keyCode==36)[self accessibilityPerformPress];else [super keyDown:event];}
- (BOOL)accessibilityPerformPress{if(_clicked)_clicked(self);return YES;}
@end

// A compact menu keeps six firmware families in the established header width.
@interface FirmwareMenu : NSPopUpButton
@property(nonatomic) NSInteger selectedSegment;
@end
@implementation FirmwareMenu
- (NSInteger)selectedSegment{return self.indexOfSelectedItem;}
- (void)setSelectedSegment:(NSInteger)value{[self selectItemAtIndex:value];}
@end

@interface ProgramDial : NSControl
@property(nonatomic) NSInteger selected;
@property(nonatomic) BOOL bankR;
@property(nonatomic) NSUInteger firmwareBank;
@property(nonatomic,copy) void(^changed)(NSInteger);
@end
@implementation ProgramDial {
    NSInteger _dragStart;
    NSPoint _dragOrigin;
    double _dragAngle,_dragSteps;
    BOOL _angularDrag,_dragging;
}
- (instancetype)initWithFrame:(NSRect)r{
    if((self=[super initWithFrame:r])){_selected=0;Fix(self,340,300);self.accessibilityElement=YES;self.accessibilityRole=NSAccessibilitySliderRole;self.accessibilityLabel=CL(@"音色，十二档循环选择");self.toolTip=CL(@"点击圆点或按住拖动选择；方向键逐档切换；第12档继续转回第1档");}return self;
}
- (BOOL)acceptsFirstResponder{return YES;}
- (NSView*)hitTest:(NSPoint)point{
    const NSPoint p=[self convertPoint:point fromView:self.superview];
    if(hypot(p.x-NSMidX(self.bounds),p.y-174)>98)return nil;
    return [super hitTest:point];
}
- (id)accessibilityValue{NSDictionary *info=ProgramInfo(_firmwareBank,_selected);NSMutableArray* parts=[NSMutableArray arrayWithObjects:[NSString stringWithFormat:@"%@%ld",BankCode(_firmwareBank),(long)_selected+1],info[@"short_name"],nil];if(info[@"reference"])[parts addObject:info[@"reference"]];[parts addObject:info[@"description"]];return [parts componentsJoinedByString:@" · "];}
- (void)setSelected:(NSInteger)v{_selected=WrapProgram(v);self.needsDisplay=YES;}
- (void)setBankR:(BOOL)value{_bankR=value;self.needsDisplay=YES;NSAccessibilityPostNotification(self,NSAccessibilityValueChangedNotification);}
- (void)setFirmwareBank:(NSUInteger)value{_firmwareBank=MIN(value,5u);self.bankR=value==1||value==5;}
- (void)choose:(NSInteger)v{v=WrapProgram(v);if(v!=_selected){self.selected=v;if(_changed)_changed(v);NSAccessibilityPostNotification(self,NSAccessibilityValueChangedNotification);}}
- (void)drawRect:(NSRect)rect{
    const NSPoint center=NSMakePoint(NSMidX(self.bounds),174);
    // The original scale is four outer arcs, one for each three-dot family.
    [Ink()setStroke];
    for(int group=0;group<4;++group){NSBezierPath *arc=[NSBezierPath bezierPath];[arc appendBezierPathWithArcWithCenter:center radius:98 startAngle:-119.-90.*group endAngle:-181.-90.*group clockwise:YES];arc.lineWidth=1.05;[arc stroke];}
    const CGFloat sizes[3]={2.4,3.8,5.3};
    for(int i=0;i<12;++i){
        const double angle=(-120.-30.*i)*M_PI/180.;const NSPoint p=NSMakePoint(center.x+77*cos(angle),center.y+77*sin(angle));
        const CGFloat radius=sizes[i%3];
        NSBezierPath *dot=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(p.x-radius,p.y-radius,radius*2,radius*2)];
        [(i==_selected?Amber():Ink())setFill];[dot fill];
        if(i==_selected){NSBezierPath *outline=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(p.x-radius-3,p.y-radius-3,radius*2+6,radius*2+6)];[Ink()setStroke];outline.lineWidth=.8;[outline stroke];}
    }
    // The four original panel glyphs: two lines, four lines, reverb rays,
    // then staggered grain fragments. Each spans one small/medium/large set.
    for(int group=0;group<4;++group){
        [NSGraphicsContext saveGraphicsState];NSAffineTransform *position=[NSAffineTransform transform];[position translateXBy:center.x yBy:center.y];[position rotateByDegrees:-150.-90.*group];[position translateXBy:98 yBy:0];[position rotateByDegrees:90];[position concat];
        [Ink()setFill];
        if(group<2){const int count=group==0?2:4;for(int k=0;k<count;++k)NSRectFill(NSMakeRect((k-(count-1)*.5)*6-1.6,-6,3.2,12));}
        else if(group==2){for(int k=0;k<9;++k){[NSGraphicsContext saveGraphicsState];NSAffineTransform *ray=[NSAffineTransform transform];[ray translateXBy:(k-4)*3.8 yBy:.25*(k-4)*(k-4)];[ray rotateByDegrees:(k-4)*-3];[ray concat];NSRectFill(NSMakeRect(-1.1,-6,2.2,12));[NSGraphicsContext restoreGraphicsState];}}
        else for(int k=0;k<7;++k)NSRectFill(NSMakeRect((k%3-1)*10-4,(k/3-1)*5,8,1.8));
        [NSGraphicsContext restoreGraphicsState];
    }
    // Black chicken-head selector traced in proportion from SOMA's two
    // official product photos: narrow nose, broad rounded tail, centered axis.
    [NSGraphicsContext saveGraphicsState];
    NSShadow *baseShadow=[NSShadow new];baseShadow.shadowColor=[NSColor colorWithWhite:0 alpha:.20];baseShadow.shadowBlurRadius=3;baseShadow.shadowOffset=NSMakeSize(1,-2);[baseShadow set];
    NSBezierPath *base=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(center.x-27,center.y-27,54,54)];
    NSGradient *baseFinish=[[NSGradient alloc]initWithStartingColor:[NSColor colorWithWhite:.16 alpha:1] endingColor:[NSColor colorWithWhite:.055 alpha:1]];[baseFinish drawInBezierPath:base angle:-70];
    [NSGraphicsContext restoreGraphicsState];
    NSBezierPath *rim=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(center.x-23,center.y-23,46,46)];rim.lineWidth=1.1;[[NSColor colorWithWhite:.30 alpha:1]setStroke];[rim stroke];
    NSBezierPath *recess=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(center.x-17,center.y-17,34,34)];[[NSColor colorWithWhite:.045 alpha:1]setFill];[recess fill];
    NSBezierPath *innerLip=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(center.x-14,center.y-14,28,28)];innerLip.lineWidth=1.2;[[NSColor colorWithWhite:.20 alpha:1]setStroke];[innerLip stroke];
    NSAffineTransform *rotation=[NSAffineTransform transform];[rotation translateXBy:center.x yBy:center.y];[rotation rotateByDegrees:-210.-30.*_selected];
    [NSGraphicsContext saveGraphicsState];[rotation concat];
    NSBezierPath *handle=[NSBezierPath bezierPath];
    [handle moveToPoint:NSMakePoint(0,50)];
    [handle curveToPoint:NSMakePoint(5.1,45) controlPoint1:NSMakePoint(3.4,50) controlPoint2:NSMakePoint(5.1,48.4)];
    [handle curveToPoint:NSMakePoint(6.2,15) controlPoint1:NSMakePoint(5.8,37) controlPoint2:NSMakePoint(5.1,22)];
    [handle curveToPoint:NSMakePoint(13.2,-44) controlPoint1:NSMakePoint(7.8,-4) controlPoint2:NSMakePoint(12,-31)];
    [handle curveToPoint:NSMakePoint(10.3,-50) controlPoint1:NSMakePoint(13.8,-48.5) controlPoint2:NSMakePoint(13,-50)];
    [handle curveToPoint:NSMakePoint(-10.3,-50) controlPoint1:NSMakePoint(5,-51.5) controlPoint2:NSMakePoint(-5,-51.5)];
    [handle curveToPoint:NSMakePoint(-13.2,-44) controlPoint1:NSMakePoint(-13,-50) controlPoint2:NSMakePoint(-13.8,-48.5)];
    [handle curveToPoint:NSMakePoint(-6.2,15) controlPoint1:NSMakePoint(-12,-31) controlPoint2:NSMakePoint(-7.8,-4)];
    [handle curveToPoint:NSMakePoint(-5.1,45) controlPoint1:NSMakePoint(-5.1,22) controlPoint2:NSMakePoint(-5.8,37)];
    [handle curveToPoint:NSMakePoint(0,50) controlPoint1:NSMakePoint(-5.1,48.4) controlPoint2:NSMakePoint(-3.4,50)];[handle closePath];
    [NSGraphicsContext saveGraphicsState];NSShadow *shadow=[NSShadow new];shadow.shadowColor=[NSColor colorWithWhite:0 alpha:.28];shadow.shadowBlurRadius=2.5;shadow.shadowOffset=NSMakeSize(1.5,-2);[shadow set];
    NSGradient *grip=[[NSGradient alloc]initWithStartingColor:[NSColor colorWithWhite:.20 alpha:1] endingColor:[NSColor colorWithWhite:.045 alpha:1]];[grip drawInBezierPath:handle angle:-35];[NSGraphicsContext restoreGraphicsState];
    // Raised side bevels follow the tapered body instead of a white stripe.
    NSBezierPath *leftBevel=[NSBezierPath bezierPath];[leftBevel moveToPoint:NSMakePoint(-3.9,42)];[leftBevel curveToPoint:NSMakePoint(-10.9,-44) controlPoint1:NSMakePoint(-3.3,15) controlPoint2:NSMakePoint(-8.8,-29)];leftBevel.lineWidth=1.5;leftBevel.lineCapStyle=NSLineCapStyleRound;[[NSColor colorWithWhite:.36 alpha:1]setStroke];[leftBevel stroke];
    NSBezierPath *rightBevel=[NSBezierPath bezierPath];[rightBevel moveToPoint:NSMakePoint(4.2,41)];[rightBevel curveToPoint:NSMakePoint(11,-44) controlPoint1:NSMakePoint(3.8,12) controlPoint2:NSMakePoint(9,-28)];rightBevel.lineWidth=.9;[[NSColor colorWithWhite:.24 alpha:1]setStroke];[rightBevel stroke];
    NSBezierPath *face=[NSBezierPath bezierPath];[face moveToPoint:NSMakePoint(-2.9,37)];[face lineToPoint:NSMakePoint(2.9,37)];[face curveToPoint:NSMakePoint(8.6,-42) controlPoint1:NSMakePoint(3,10) controlPoint2:NSMakePoint(7,-28)];[face curveToPoint:NSMakePoint(-8.6,-42) controlPoint1:NSMakePoint(5,-43) controlPoint2:NSMakePoint(-5,-43)];[face curveToPoint:NSMakePoint(-2.9,37) controlPoint1:NSMakePoint(-7,-28) controlPoint2:NSMakePoint(-3,10)];[face closePath];[[NSColor colorWithWhite:.065 alpha:1]setFill];[face fill];
    NSBezierPath *tailEdge=[NSBezierPath bezierPath];[tailEdge moveToPoint:NSMakePoint(-9.9,-47)];[tailEdge curveToPoint:NSMakePoint(9.9,-47) controlPoint1:NSMakePoint(-4,-48.2) controlPoint2:NSMakePoint(4,-48.2)];tailEdge.lineWidth=1.1;[[NSColor colorWithWhite:.27 alpha:1]setStroke];[tailEdge stroke];
    NSBezierPath *noseMark=[NSBezierPath bezierPath];[noseMark moveToPoint:NSMakePoint(-1.6,52)];[noseMark lineToPoint:NSMakePoint(0,49.8)];[noseMark lineToPoint:NSMakePoint(1.6,52)];noseMark.lineWidth=.9;[Paper()setStroke];[noseMark stroke];
    [NSGraphicsContext restoreGraphicsState];

}
- (void)mouseDown:(NSEvent*)event{
    NSPoint p=[self convertPoint:event.locationInWindow fromView:nil];const double dx=p.x-NSMidX(self.bounds),dy=p.y-174;
    _dragging=hypot(dx,dy)<=98;if(!_dragging)return;
    [self.window makeFirstResponder:self];
    if(hypot(dx,dy)>50&&p.y>70)[self choose:lround((-120.-atan2(dy,dx)*180./M_PI)/30.)];
    _dragStart=_selected;_dragOrigin=p;_dragAngle=atan2(dy,dx);_dragSteps=0;_angularDrag=hypot(dx,dy)>12&&p.y>70;
}
- (void)mouseDragged:(NSEvent*)event{
    if(!_dragging)return;
    NSPoint p=[self convertPoint:event.locationInWindow fromView:nil];
    if(_angularDrag){const double angle=atan2(p.y-174,p.x-NSMidX(self.bounds));double delta=angle-_dragAngle;while(delta>M_PI)delta-=2*M_PI;while(delta< -M_PI)delta+=2*M_PI;_dragAngle=angle;_dragSteps-=delta*6/M_PI;const NSInteger steps=lround(_dragSteps);if(steps){[self choose:_selected+steps];_dragSteps-=steps;}}
    else [self choose:_dragStart+lround((p.x-_dragOrigin.x+p.y-_dragOrigin.y)/12.)];
}
- (void)mouseUp:(NSEvent*)event{_dragging=NO;}
// Inherit scroll handling so trackpad and wheel gestures can scroll the panel
// without switching programs, including during momentum scrolling.
- (void)keyDown:(NSEvent*)event{if(event.keyCode==123||event.keyCode==125)[self choose:_selected-1];else if(event.keyCode==124||event.keyCode==126)[self choose:_selected+1];else [super keyDown:event];}
- (BOOL)accessibilityPerformIncrement{[self choose:_selected+1];return YES;}
- (BOOL)accessibilityPerformDecrement{[self choose:_selected-1];return YES;}
@end

@interface ParameterDial : NSControl<NSTextFieldDelegate>
@property(nonatomic,copy) NSString *title;
@property(nonatomic) double normalized;
@property(nonatomic) double defaultValue;
@property(nonatomic) BOOL showsUnityReference;
@property(nonatomic,copy) void(^changed)(double);
@property(nonatomic,copy) void(^dragging)(BOOL);
- (void)updateHelpHeading:(NSString*)heading body:(NSString*)body;
- (void)presentHelpInside:(NSView*)surface;
- (void)dismissHoverHelp;
@end
@implementation ParameterDial {
    double _dragValue;NSPoint _origin;
    NSTextField *_number;
    BOOL _numberEdited;
    CosmosParameterHover *_hoverHelp;
}
- (instancetype)initWithTitle:(NSString*)title initial:(double)value{
    if((self=[super initWithFrame:NSZeroRect])){
        _title=title;_normalized=value;_defaultValue=value;Fix(self,130,116);
        self.accessibilityElement=YES;self.accessibilityRole=NSAccessibilityGroupRole;self.accessibilityLabel=title;self.toolTip=CL(@"拖动调节；点击数字输入 0–100；双击旋钮复位");
        _number=[[NSTextField alloc]initWithFrame:NSMakeRect(34,92,62,23)];_number.alignment=NSTextAlignmentCenter;
        _number.font=[NSFont monospacedDigitSystemFontOfSize:11 weight:NSFontWeightMedium];_number.textColor=Ink();
        _number.bezelStyle=NSTextFieldRoundedBezel;_number.backgroundColor=[NSColor colorWithWhite:1 alpha:.45];
        _number.delegate=self;_number.target=self;_number.action=@selector(numberEntered:);
        _number.accessibilityLabel=[title stringByAppendingString:CL(@" 数值，0到100百分比")];
        [self addSubview:_number];[self updateNumber];_hoverHelp=[[CosmosParameterHover alloc]initWithView:self];
    }return self;
}
- (NSArray*)accessibilityChildren{return _number?@[_number]:@[];}
- (void)updateTrackingAreas{[super updateTrackingAreas];[_hoverHelp refreshTracking];}
- (void)viewWillMoveToWindow:(NSWindow*)window{if(!window)[_hoverHelp dismiss];[super viewWillMoveToWindow:window];}
- (void)updateHelpHeading:(NSString*)heading body:(NSString*)body{self.toolTip=nil;_number.toolTip=nil;self.accessibilityHelp=body;_number.accessibilityHelp=[body stringByAppendingString:CL(@" 输入0–100，按回车确认。")];[_hoverHelp updateHeading:heading body:body];}
- (void)presentHelpInside:(NSView*)surface{[_hoverHelp presentInsideView:surface];}
- (void)dismissHoverHelp{[_hoverHelp dismiss];}
- (void)updateNumber{_number.stringValue=[NSString stringWithFormat:@"%.1f",_normalized*100];_numberEdited=NO;}
- (void)setNormalized:(double)value{_normalized=std::clamp(value,0.,1.);[self updateNumber];self.needsDisplay=YES;}
- (BOOL)acceptsFirstResponder{return YES;}
- (void)change:(double)value{[_hoverHelp dismiss];const double before=_normalized;self.normalized=value;if(_changed&&fabs(before-_normalized)>1e-10)_changed(_normalized);}
- (void)numberEntered:(id)sender{
    // The rounded display is not a new parameter value. Return and focus loss
    // can both submit the same edit, so consume actual text changes only once.
    if(!_numberEdited)return;
    _numberEdited=NO;
    NSString *text=[_number.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    if([text hasSuffix:@"%"]||[text hasSuffix:@"％"])text=[text substringToIndex:text.length-1];
    NSScanner *scanner=[NSScanner scannerWithString:text];scanner.locale=@{@"NSDecimalSeparator":@"."};double value=0;
    if(![scanner scanDouble:&value]||!scanner.isAtEnd||!std::isfinite(value)||value<0||value>100){NSBeep();[self updateNumber];return;}
    [self change:value/100.];
}
- (void)controlTextDidEndEditing:(NSNotification*)notification{[self numberEntered:_number];}
- (void)controlTextDidBeginEditing:(NSNotification*)notification{_numberEdited=NO;[_hoverHelp dismiss];}
- (void)controlTextDidChange:(NSNotification*)notification{_numberEdited=YES;}
- (void)drawRect:(NSRect)r{
    DrawText(_title,NSMakeRect(0,2,130,18),Font(11,NSFontWeightSemibold),Ink());const NSPoint center=NSMakePoint(65,56);
    for(int i=0;i<9;++i){const double angle=(225.-270.*i/8.)*M_PI/180.;const NSPoint p=NSMakePoint(center.x+34*cos(angle),center.y+34*sin(angle));NSBezierPath *dot=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(p.x-1.7,p.y-1.7,3.4,3.4)];[Ink()setFill];[dot fill];}
    if(_showsUnityReference){const double a=(225.-270.*cosmos_app::FeedbackUnityUI)*M_PI/180.;const NSPoint p=NSMakePoint(center.x+49*cos(a),center.y+49*sin(a));DrawText(@"∞",NSMakeRect(p.x-9,p.y-9,18,18),Font(16,NSFontWeightMedium),Ink());}
    NSBezierPath *disc=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(40,31,50,50)];
    [NSGraphicsContext saveGraphicsState];NSShadow *shadow=[NSShadow new];shadow.shadowColor=[Ink()colorWithAlphaComponent:.14];shadow.shadowBlurRadius=2;shadow.shadowOffset=NSMakeSize(0,-1.5);[shadow set];[Ink()setFill];[disc fill];[NSGraphicsContext restoreGraphicsState];
    for(int i=0;i<28;++i){const double angle=i*2*M_PI/28;NSBezierPath *rib=[NSBezierPath bezierPath];[rib moveToPoint:NSMakePoint(center.x+22*cos(angle),center.y+22*sin(angle))];[rib lineToPoint:NSMakePoint(center.x+24*cos(angle),center.y+24*sin(angle))];[[NSColor colorWithWhite:.38 alpha:1]setStroke];rib.lineWidth=.7;[rib stroke];}
    NSBezierPath *face=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(46,37,38,38)];NSGradient *finish=[[NSGradient alloc]initWithStartingColor:[NSColor colorWithSRGBRed:.965 green:.958 blue:.925 alpha:1]endingColor:[NSColor colorWithSRGBRed:.84 green:.833 blue:.80 alpha:1]];[finish drawInBezierPath:face angle:-90];
    [[NSColor colorWithWhite:1 alpha:.55]setStroke];face.lineWidth=.6;[face stroke];
    const double a=(225-270*_normalized)*M_PI/180.;NSBezierPath *pointer=[NSBezierPath bezierPath];[pointer moveToPoint:NSMakePoint(center.x+12*cos(a),center.y+12*sin(a))];[pointer lineToPoint:NSMakePoint(center.x+19*cos(a),center.y+19*sin(a))];pointer.lineWidth=2.5;pointer.lineCapStyle=NSLineCapStyleRound;[Ink()setStroke];[pointer stroke];
    DrawText(@"%",NSMakeRect(99,96,16,15),Font(10),Muted());
}
- (void)mouseDown:(NSEvent*)e{[_hoverHelp dismiss];[self.window makeFirstResponder:self];if(_dragging)_dragging(YES);if(e.clickCount==2)[self change:_defaultValue];_dragValue=_normalized;_origin=[self convertPoint:e.locationInWindow fromView:nil];}
- (void)mouseDragged:(NSEvent*)e{NSPoint p=[self convertPoint:e.locationInWindow fromView:nil];[self change:_dragValue+(p.y-_origin.y+p.x-_origin.x)/160.];}
- (void)mouseUp:(NSEvent*)e{if(_dragging)_dragging(NO);[_hoverHelp resumeAfterInteraction];}
- (void)scrollWheel:(NSEvent*)e{[self change:_normalized+e.scrollingDeltaY*.005];[_hoverHelp resumeAfterInteraction];}
- (void)keyDown:(NSEvent*)e{if(e.keyCode==123||e.keyCode==125)[self change:_normalized-.01];else if(e.keyCode==124||e.keyCode==126)[self change:_normalized+.01];else [super keyDown:e];}
- (BOOL)accessibilityPerformIncrement{[self change:_normalized+.01];return YES;}
- (BOOL)accessibilityPerformDecrement{[self change:_normalized-.01];return YES;}
@end

static NSBezierPath *PedalMetalOutline(){
    NSBezierPath *outline=[NSBezierPath bezierPath];
    for(int i=0;i<6;++i){const double a=(30+i*60)*M_PI/180.;const NSPoint p=NSMakePoint(71+24*cos(a),55+24*sin(a));if(i==0)[outline moveToPoint:p];else[outline lineToPoint:p];}
    [outline closePath];outline.lineJoinStyle=NSLineJoinStyleRound;return outline;
}
@interface MomentaryButton : NSControl
@property(nonatomic,copy) NSString *title;
@property(nonatomic,copy) NSString *keyHint;
@property(nonatomic,copy) NSString *secondaryTitle;
@property(nonatomic) BOOL held;
@property(nonatomic) BOOL lampActive;
@property(nonatomic) BOOL illuminateAll;
@property(nonatomic) NSInteger mode;
@property(nonatomic) unsigned lampCount;
@property(nonatomic,strong) ModeLabel *modeMenu;
@property(nonatomic,copy) void(^holdChanged)(BOOL);
@end
@implementation MomentaryButton {
    BOOL _pressedByMouse;
}
- (BOOL)acceptsFirstResponder{return YES;}
- (instancetype)initWithTitle:(NSString*)title key:(NSString*)key{
    if((self=[super initWithFrame:NSZeroRect])){_title=title;_keyHint=key;Fix(self,142,146);self.accessibilityElement=YES;self.accessibilityRole=NSAccessibilityButtonRole;self.accessibilityLabel=[NSString stringWithFormat:CL(@"%@，按住生效，快捷键%@"),title,key];}return self;
}
- (void)setHeld:(BOOL)v{_held=v;self.needsDisplay=YES;}
- (id)accessibilityValue{return @(_lampActive || _illuminateAll);}
- (void)setLampActive:(BOOL)v{_lampActive=v;self.needsDisplay=YES;}
- (void)setModeMenu:(ModeLabel*)menu{[_modeMenu removeFromSuperview];_modeMenu=menu;if(menu)Place(menu,self,NSMakeRect(21,90,100,24));self.needsDisplay=YES;}
- (NSArray*)accessibilityChildren{return _modeMenu?@[_modeMenu]:@[];}
- (NSView*)hitTest:(NSPoint)point{
    NSView *hit=[super hitTest:point];
    if(hit!=self)return hit; // The label menu is a separate control.
    const NSPoint p=[self convertPoint:point fromView:self.superview];
    return [PedalMetalOutline() containsPoint:p]?self:nil;
}
- (void)drawRect:(NSRect)r{
    if(!_modeMenu)DrawText(_title,NSMakeRect(0,93,142,17),Font(11,NSFontWeightSemibold),Ink());
    if(_lampCount==2){Lamp(NSMakePoint(61,131),_illuminateAll||(_lampActive&&_mode!=1));Lamp(NSMakePoint(81,131),_illuminateAll||(_lampActive&&_mode!=0));}
    else if(_lampCount==1)Lamp(NSMakePoint(71,131),_illuminateAll||_lampActive);
    const NSPoint c=NSMakePoint(71,_held?54:56);
    NSBezierPath *base=PedalMetalOutline();
    [NSGraphicsContext saveGraphicsState];NSShadow *shadow=[NSShadow new];shadow.shadowColor=[Ink()colorWithAlphaComponent:.16];shadow.shadowBlurRadius=2;shadow.shadowOffset=NSMakeSize(0,-1);[shadow set];[[NSColor colorWithWhite:.52 alpha:1]setFill];[base fill];[NSGraphicsContext restoreGraphicsState];
    NSGradient *metal=[[NSGradient alloc]initWithStartingColor:[NSColor colorWithWhite:.94 alpha:1]endingColor:[NSColor colorWithWhite:.59 alpha:1]];[metal drawInBezierPath:base angle:-90];[[NSColor colorWithWhite:.42 alpha:1]setStroke];base.lineWidth=.6;[base stroke];
    NSBezierPath *ring=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(c.x-20.5,c.y-20.5,41,41)];[[NSColor colorWithWhite:.79 alpha:1]setFill];[ring fill];[[NSColor colorWithWhite:.40 alpha:1]setStroke];ring.lineWidth=.7;[ring stroke];
    NSBezierPath *rim=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(c.x-17,c.y-17,34,34)];[[NSColor colorWithWhite:.36 alpha:1]setFill];[rim fill];
    NSBezierPath *cap=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(c.x-15.5,c.y-15.5,31,31)];NSGradient *top=[[NSGradient alloc]initWithStartingColor:[NSColor colorWithWhite:_held?.77:.99 alpha:1]endingColor:[NSColor colorWithWhite:_held?.68:.82 alpha:1]];[top drawInBezierPath:cap angle:-90];[[NSColor colorWithWhite:1 alpha:.65]setStroke];cap.lineWidth=.5;[cap stroke];
    if(_secondaryTitle.length)DrawText(_secondaryTitle,NSMakeRect(0,1,142,13),Font(8,NSFontWeightRegular),Muted());
}
- (BOOL)acceptsFirstMouse:(NSEvent*)e{return YES;}
- (void)mouseDown:(NSEvent*)event{
    _pressedByMouse=[PedalMetalOutline() containsPoint:[self convertPoint:event.locationInWindow fromView:nil]];
    if(!_pressedByMouse)return;
    [self.window makeFirstResponder:self];
    if(_holdChanged)_holdChanged(YES);
}
- (void)mouseUp:(NSEvent*)event{if(!_pressedByMouse)return;_pressedByMouse=NO;if(_holdChanged)_holdChanged(NO);}
- (BOOL)accessibilityPerformPress{if(_holdChanged)_holdChanged(YES);__weak MomentaryButton *weak=self;dispatch_after(dispatch_time(DISPATCH_TIME_NOW,100*NSEC_PER_MSEC),dispatch_get_main_queue(),^{if(weak.holdChanged)weak.holdChanged(NO);});return YES;}
@end

