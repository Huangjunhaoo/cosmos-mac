// Included inside CosmosDelegate. MIDI state is owned by the main thread;
// only timestamped real-time messages bypass it for the audio scheduler.
- (NSURL*)midiSettingsURL{return cosmos_midi::settingsURL();}
- (void)initializeMIDI{
    NSError* error=nil;_midiSettingsReadable=cosmos_midi::loadSettings([self midiSettingsURL],_midiSettings,&error);
    if(!_midiSettingsReadable)_midiNotice=error.localizedDescription;
    _midiInput=std::make_unique<cosmos_midi::Input>();NSString* failure=nil;
    DesktopAudio* audio=_audio.get();
    if(!_midiInput->open([audio](cosmos_midi::Message m){audio->feedMidiClock(m);},&failure))_midiNotice=failure;
    else if(!_midiInput->select(_midiSettings.enabled,_midiSettings.source,&failure)){_midiNotice=failure;_midiSettings.enabled=false;}
    _audio->setMidiSync(_midiSettings.enabled&&_midiSettings.sync,_midiSettings.bars);
    [self installMIDIMenu];
}
- (void)openMIDIMapGuide:(id)sender{OpenMIDIMapGuide();}
- (void)installMIDIMenu{
    NSMenuItem* root=[[NSMenuItem alloc]initWithTitle:@"MIDI" action:nil keyEquivalent:@""];NSMenu* menu=[[NSMenu alloc]initWithTitle:@"MIDI"];root.submenu=menu;
    NSMenuItem* settings=[menu addItemWithTitle:@"MIDI 设置…" action:@selector(showMIDI:) keyEquivalent:@""];settings.target=self;
    [menu addItemWithTitle:@"MIDI 映射指南（PDF）" action:@selector(openMIDIMapGuide:) keyEquivalent:@""].target=self;
    LocalizedMenu(menu,YES);[[NSApp mainMenu]addItem:root];
    _midiTimer=[NSTimer timerWithTimeInterval:.01 target:self selector:@selector(pollMIDI:) userInfo:nil repeats:YES];[[NSRunLoop mainRunLoop]addTimer:_midiTimer forMode:NSRunLoopCommonModes];
}
- (void)saveMIDISettings{
    if(!_midiSettingsReadable){_midiNotice=CL(@"MIDI 设置文件损坏，已保留原文件。本次改动只在当前运行中生效。");return;}
    NSError* error=nil;if(!cosmos_midi::saveSettings([self midiSettingsURL],_midiSettings,&error))_midiNotice=error.localizedDescription;
}
- (void)applyMIDIPreset:(const cosmos_midi::Preset&)preset{
    // Caller releases old gestures first. Keep the active input connection so
    // loading a sound via Program Change does not reconnect to another device.
    cosmos_midi::restorePreset(preset,_midiSettings);
    if(_midiInput)_midiInput->discardPending();
    _audio->setMidiSync(_midiSettings.enabled&&_midiSettings.sync,_midiSettings.bars);
    [_midiChannel selectItemAtIndex:_midiSettings.router.channel+1];_midiPC.state=_midiSettings.programChange;_midiSync.state=_midiSettings.sync;
    [_midiBars selectItemAtIndex:_midiSettings.bars==4?2:_midiSettings.bars==2?1:0];
    const unsigned target=[self selectedMIDITarget];if(target<cosmos_midi::TargetCount)[_midiPedalMode selectItemAtIndex:_midiSettings.router.bindings[target].pulse?0:1];
    _midiNotice=nil;[self saveMIDISettings];[self updateMIDIDisplay];
}
- (void)reloadMIDISources{
    if(!_midiSource)return;[_midiSource removeAllItems];
    [_midiSource addItemWithTitle:CL(@"关闭")];_midiSource.lastItem.representedObject=NSNull.null;
    [_midiSource addItemWithTitle:CL(@"软件输入 · COSMOS MIDI In")];_midiSource.lastItem.representedObject=@0;
    if(_midiInput)for(const auto& source:_midiInput->sources()){[_midiSource addItemWithTitle:source.name];_midiSource.lastItem.representedObject=@(source.uid);}
    [_midiSource selectItemAtIndex:0];
    if(_midiSettings.enabled)for(NSMenuItem* item in _midiSource.itemArray)if([item.representedObject isEqual:@(_midiSettings.source)]){[_midiSource selectItem:item];break;}
}
- (void)showMIDI:(id)sender{
    if(_closing||!_audio)return;
    if(!_midiWindow){
        _midiWindow=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,640,530) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable backing:NSBackingStoreBuffered defer:NO];
        _midiWindow.title=@"MIDI";_midiWindow.releasedWhenClosed=NO;_midiWindow.backgroundColor=Paper();_midiWindow.contentView=[[PaperView alloc]initWithFrame:_midiWindow.contentView.bounds];NSView* root=_midiWindow.contentView;
        auto label=[&](NSString* text,CGFloat y){Place(Label(text,11,Muted()),root,NSMakeRect(24,y,105,22));};
        auto popup=[&](CGFloat x,CGFloat y,CGFloat w,SEL action){NSPopUpButton* p=[[NSPopUpButton alloc]initWithFrame:NSZeroRect pullsDown:NO];p.font=Font(12);p.target=self;p.action=action;Place(p,root,NSMakeRect(x,y,w,26));return p;};
        auto button=[&](NSString* title,CGFloat x,CGFloat y,CGFloat w,SEL action){NSButton* b=[NSButton buttonWithTitle:title target:self action:action];b.font=Font(11);Place(b,root,NSMakeRect(x,y,w,28));return b;};
        label(@"输入",482);_midiSource=popup(124,478,492,@selector(midiSourceChanged:));_midiSource.accessibilityLabel=@"MIDI 输入";
        label(@"通道",442);_midiChannel=popup(124,438,150,@selector(midiChannelChanged:));[_midiChannel addItemWithTitle:@"全部通道"];for(unsigned i=1;i<=16;++i)[_midiChannel addItemWithTitle:[NSString stringWithFormat:CL(@"通道 %u"),i]];[_midiChannel selectItemAtIndex:_midiSettings.router.channel+1];
        _midiPC=[NSButton checkboxWithTitle:@"Program Change 切换 32 组预设" target:self action:@selector(midiPCChanged:)];_midiPC.font=Font(11);_midiPC.state=_midiSettings.programChange;Place(_midiPC,root,NSMakeRect(294,438,322,26));
        label(@"学习控制",395);_midiTarget=popup(124,391,290,@selector(midiTargetChanged:));[self rebuildMIDITargets];
        _midiLearn=button(@"学习",426,390,86,@selector(midiLearn:));button(@"移除",522,390,94,@selector(midiClear:));
        label(@"踏板动作",358);_midiPedalMode=popup(124,354,190,@selector(midiPedalModeChanged:));[_midiPedalMode addItemsWithTitles:@[@"点按（短按一次）",@"按住（跟随按下／松开）"]];
        Place(Label(@"点按可每次只发 127。按住用 127 按下、0 松开。",10,Muted()),root,NSMakeRect(326,356,290,22));
        NSScrollView* scroll=[[NSScrollView alloc]initWithFrame:NSZeroRect];scroll.hasVerticalScroller=YES;scroll.borderType=NSBezelBorder;
        _midiAssignments=[[NSTextView alloc]initWithFrame:NSMakeRect(0,0,588,140)];_midiAssignments.editable=NO;_midiAssignments.font=Font(11);_midiAssignments.backgroundColor=Paper();_midiAssignments.textColor=Ink();_midiAssignments.autoresizingMask=NSViewWidthSizable;scroll.documentView=_midiAssignments;Place(scroll,root,NSMakeRect(24,202,592,142));
        _midiSync=[NSButton checkboxWithTitle:@"跟随 MIDI Clock（R / PR 第 1–9 档）" target:self action:@selector(midiSyncChanged:)];_midiSync.font=Font(11);_midiSync.state=_midiSettings.sync;Place(_midiSync,root,NSMakeRect(24,157,356,28));
        _midiBars=popup(429,157,187,@selector(midiSyncChanged:));[_midiBars addItemsWithTitles:@[@"录 1 小节",@"录 2 小节",@"录 4 小节"]];[_midiBars selectItemAtIndex:_midiSettings.bars==4?2:_midiSettings.bars==2?1:0];
        NSTextField* hint=Label(@"4/4 拍。按 RECORD 等下一小节开始，再按可取消等待或提前收尾。\n新循环请先清除旧录音；旧循环不随 BPM 拉伸。PR 录音时请保持 SPEED 不动。\nPC 0–31 对应预设 01–32；空白组保留当前声音。",10,Muted());hint.maximumNumberOfLines=3;Place(hint,root,NSMakeRect(24,88,592,60));
        _midiStatusLabel=Label(@"",11,Teal());_midiStatusLabel.maximumNumberOfLines=2;Place(_midiStatusLabel,root,NSMakeRect(24,24,592,55));
        LocalizedView(root,YES);[self reloadMIDISources];[self midiTargetChanged:nil];[_midiWindow center];
    }
    [self updateMIDIDisplay];[_midiWindow makeKeyAndOrderFront:nil];
}
- (void)midiSourceChanged:(NSPopUpButton*)sender{
    if(_closing||!_audio)return;
    [self releaseMIDI];_midiSettings.router.learning=-1;
    const id uid=sender.selectedItem.representedObject;_midiSettings.enabled=[uid isKindOfClass:NSNumber.class];_midiSettings.source=_midiSettings.enabled?[uid intValue]:0;
    NSString* error=nil;if(!_midiInput||!_midiInput->select(_midiSettings.enabled,_midiSettings.source,&error)){_midiSettings.enabled=false;_midiNotice=error?:CL(@"MIDI 不可用。");}else _midiNotice=nil;
    _audio->resetMidiClock();_audio->setMidiSync(_midiSettings.enabled&&_midiSettings.sync,_midiSettings.bars);[self saveMIDISettings];[self reloadMIDISources];[self updateMIDIDisplay];
}
- (void)midiChannelChanged:(NSPopUpButton*)sender{
    if(_closing||!_audio)return;[self releaseMIDI];_midiSettings.router.channel=int(sender.indexOfSelectedItem)-1;_midiSettings.router.learning=-1;[self saveMIDISettings];[self updateMIDIDisplay];}
- (void)midiPCChanged:(NSButton*)sender{
    if(_closing||!_audio)return;_midiSettings.programChange=sender.state==NSControlStateValueOn;[self saveMIDISettings];}
- (unsigned)selectedMIDITarget{
    NSMenuItem* selected=_midiTarget.selectedItem;
    return selected&&selected.tag>=0&&selected.representedObject?unsigned(selected.tag):cosmos_midi::TargetCount;
}
- (void)rebuildMIDITargets{
    if(!_midiTarget)return;
    NSString* menuKey=[NSString stringWithFormat:CL(@"MIDI 目标 · %lu"),(unsigned long)_selectedBank];
    if([_midiTarget.menu.title isEqual:menuKey]&&_midiTarget.numberOfItems)return;
    const unsigned previous=[self selectedMIDITarget];_midiSettings.router.learning=-1;
    [_midiTarget removeAllItems];_midiTarget.menu.title=menuKey;_midiTarget.menu.autoenablesItems=NO;
    NSArray* names=cosmos_midi::targetNames(unsigned(_selectedBank));NSArray* headings=cosmos_midi::targetGroupNames(unsigned(_selectedBank));
    NSArray<NSArray<NSNumber*>*>* groups=cosmos_midi::targetGroups(unsigned(_selectedBank));
    for(NSUInteger group=0;group<groups.count;++group){
        if(group)[_midiTarget.menu addItem:NSMenuItem.separatorItem];
        NSMenuItem* heading=[[NSMenuItem alloc]initWithTitle:headings[group] action:nil keyEquivalent:@""];heading.enabled=NO;heading.tag=-1;[_midiTarget.menu addItem:heading];
        for(NSNumber* number in groups[group]){const unsigned target=number.unsignedIntValue;NSMenuItem* item=[[NSMenuItem alloc]initWithTitle:names[target] action:nil keyEquivalent:@""];item.tag=target;item.representedObject=number;item.indentationLevel=1;[_midiTarget.menu addItem:item];}
    }
    if(![_midiTarget selectItemWithTag:previous])[_midiTarget selectItemWithTag:0];
    const unsigned target=[self selectedMIDITarget];
    if(target<cosmos_midi::TargetCount){_midiPedalMode.enabled=cosmos_midi::pedal(target);[_midiPedalMode selectItemAtIndex:_midiSettings.router.bindings[target].pulse?0:1];}
}
- (void)midiTargetChanged:(id)sender{
    if(_closing||!_audio)return;
    _midiSettings.router.learning=-1;const unsigned i=[self selectedMIDITarget];if(i>=cosmos_midi::TargetCount)return;
    _midiPedalMode.enabled=cosmos_midi::pedal(i);[_midiPedalMode selectItemAtIndex:_midiSettings.router.bindings[i].pulse?0:1];[self updateMIDIDisplay];
}
- (void)midiPedalModeChanged:(NSPopUpButton*)sender{
    if(_closing||!_audio)return;
    [self releaseMIDI];const unsigned i=[self selectedMIDITarget];if(i<cosmos_midi::TargetCount)_midiSettings.router.bindings[i].pulse=sender.indexOfSelectedItem==0;
    _midiSettings.router.learnPulse=sender.indexOfSelectedItem==0;[self saveMIDISettings];[self updateMIDIDisplay];
}
- (void)midiLearn:(id)sender{
    if(_closing||!_audio)return;
    if(!_midiSettings.enabled){_midiNotice=CL(@"请先选择 MIDI 输入。");[self updateMIDIDisplay];return;}
    [self releaseMIDI];if(_midiSettings.router.learning>=0)_midiSettings.router.learning=-1;
    else _midiSettings.router.learn([self selectedMIDITarget],_midiPedalMode.indexOfSelectedItem==0);
    _midiNotice=nil;[self updateMIDIDisplay];
}
- (void)midiClear:(id)sender{
    if(_closing||!_audio)return;[self releaseMIDI];const unsigned i=[self selectedMIDITarget];if(i<cosmos_midi::TargetCount)_midiSettings.router.bindings[i]={};_midiSettings.router.learning=-1;[self saveMIDISettings];[self updateMIDIDisplay];}
- (void)midiSyncChanged:(id)sender{
    if(_closing||!_audio)return;
    [self releaseMIDI];_midiSettings.sync=_midiSync.state==NSControlStateValueOn;_midiSettings.bars=1u<<unsigned(_midiBars.indexOfSelectedItem);
    _audio->setMidiSync(_midiSettings.enabled&&_midiSettings.sync,_midiSettings.bars);_midiNotice=nil;[self saveMIDISettings];[self updateMIDIDisplay];
}
- (void)updateMIDIDisplay{
    if(_closing||!_audio)return;
    if(!_midiWindow)return;NSMutableString* text=[NSMutableString new];NSArray* names=cosmos_midi::targetNames(unsigned(_selectedBank));
    [self rebuildMIDITargets];
    NSArray<NSArray<NSNumber*>*>* groups=cosmos_midi::targetGroups(unsigned(_selectedBank));NSArray* headings=cosmos_midi::targetGroupNames(unsigned(_selectedBank));
    for(NSUInteger group=0;group<groups.count;++group){BOOL hasHeading=NO;
        for(NSNumber* number in groups[group]){const unsigned i=number.unsignedIntValue;const auto& b=_midiSettings.router.bindings[i];if(!b.kind)continue;
            if(!hasHeading){if(text.length)[text appendString:@"\n"];[text appendFormat:@"%@\n",headings[group]];hasHeading=YES;}
            NSString* parameter=[[names[i]componentsSeparatedByString:@" · "]lastObject];
            [text appendFormat:@"  %@   ←   CH %u · %@ %u%@\n",parameter,unsigned(b.channel)+1,b.kind==0xb0?@"CC":@"Note",unsigned(b.number),cosmos_midi::pedal(i)?(b.pulse?CL(@" · 点按"):CL(@" · 按住")):@""];
        }
    }
    if(!text.length)[text appendFormat:CL(@"%@\n选择参数 → 点“学习” → 转动控制器或踩下踏板。"),cosmos_midi::modeName(unsigned(_selectedBank))];
    if(![_midiAssignments.string isEqual:text])_midiAssignments.string=text;
    _midiLearn.title=_midiSettings.router.learning>=0?CL(@"取消学习"):CL(@"学习");
    NSString* status=CL(@"MIDI 已关闭");
    if(_midiSettings.enabled){status=CL(@"MIDI 已连接 · 自由运行");
        if(_midiSettings.sync){const auto s=_audio->midiStats();NSArray* titles=@[CL(@"自由运行"),CL(@"等待 MIDI Clock"),CL(@"已对拍 · 按 RECORD 等下一小节"),CL(@"等待下一小节录音…"),CL(@"正在录音 · 到小节末自动收尾"),CL(@"发送端已停止 · 循环继续播放"),CL(@"时钟中断 · 已结束本次录音"),CL(@"正在清除、切档或已在录音，请稍后重试"),CL(@"超过录音长度，请减少小节数或提高 BPM"),CL(@"此档保持自由运行"),CL(@"MIDI 信息过多 · 已取消录音")];
            status=s.bpm>0?[NSString stringWithFormat:@"%.1f BPM · %@",s.bpm,titles[unsigned(s.state)]]:titles[unsigned(s.state)];
            if(!_running)status=CL(@"音频尚未启动 · 启动后跟随 MIDI Clock");
        }
    }
    if(_midiSettings.router.learning>=0)status=[NSString stringWithFormat:CL(@"等待控制器：%@"),names[_midiSettings.router.learning]];
    _midiStatusLabel.stringValue=_midiNotice.length?_midiNotice:status;
}
- (BOOL)quantizedRecord{
    return _midiSettings.enabled&&_midiSettings.sync&&(_selectedBank==1||_selectedBank==5)&&_program.selected<9;
}
- (void)updatePedal:(unsigned)i panel:(unsigned)panel{
    if(i>=5||panel>1||_closing||!_audio)return;
    const bool held=panel?(_qMouseHeld[i]||_qKeyHeld[i]||_midiHeld[i+5]):(_mouseHeld[i]||_keyHeld[i]||_midiHeld[i]);
    const unsigned offset=panel*5+i;const bool edge=held&&!_aggregateHeld[offset];_aggregateHeld[offset]=held;
    (panel?_rearPanel.buttons:_buttons)[i].held=held;const unsigned bits[5]={9,8,7,6,5};
    if(!panel&&i==4&&[self quantizedRecord]){_audio->setButton(5,false);if(edge){_audio->requestMidiRecord();_midiNotice=nil;}return;}
    if(panel)_audio->setPanelButton(1,bits[i],held);else _audio->setButton(bits[i],held);
}
- (void)releaseMIDI{
    _midiSettings.router.release();_midiHeld.fill(false);for(auto& generation:_midiPulse)++generation;
    for(unsigned i=0;i<5;++i){[self updatePedal:i panel:0];[self updatePedal:i panel:1];}
    if(_audio)_audio->cancelMidiRecord();
}
- (void)applyMIDI:(cosmos_midi::Action)a{
    if(_closing||_busy||!_audio)return;const unsigned i=a.target;
    if(i==cosmos_midi::EraseAllTarget){
        if(a.down)_audio->requestEraseAll();
        return;
    }
    if(i<12){const unsigned lane=i%6;if(i>=6&&_selectedBank!=3)return;ParameterDial* dial=(i<6?_parameters:_rearPanel.parameters)[lane];dial.normalized=a.value;if(dial.changed)dial.changed(a.value);}
    else if(i<16){
        if(i==12){const float db=float(_inputGain.slider.minValue+a.value*(_inputGain.slider.maxValue-_inputGain.slider.minValue));[_inputGain setGain:db];_inputGain.changed(db);}
        else if(i==13&&_selectedBank==3){_masterSlider.doubleValue=_masterSlider.minValue+a.value*(_masterSlider.maxValue-_masterSlider.minValue);[self masterChanged:_masterSlider];}
        else{if(i>=14&&_selectedBank!=3)return;GainCard* gain=i==15?_rearPanel.gain:_outputGain;const float db=float(gain.slider.minValue+a.value*(gain.slider.maxValue-gain.slider.minValue));[gain setGain:db];gain.changed(db);}
    }else if(i<26){
        const unsigned k=i-16,panel=k/5,button=k%5;if(panel&&_selectedBank!=3)return;
        if(a.pulse && a.down && button==4 && _running){
            const auto first=_aggregateHeld.begin()+panel*5;
            if(std::none_of(first,first+5,[](bool down){return down;})){
                // A discrete REC command also handles presses closer together
                // than the panel debounce. Modifier chords still use the panel.
                if(!panel && [self quantizedRecord])_audio->requestMidiRecord();
                else _audio->setPanelEnabled(panel,0,!_audio->panelStats(panel,true).record);
                return;
            }
        }
        if(a.pulse){if(!a.down)return;const unsigned generation=++_midiPulse[k];_midiHeld[k]=true;[self updatePedal:button panel:panel];__weak CosmosDelegate* weak=self;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW,150*NSEC_PER_MSEC),dispatch_get_main_queue(),^{CosmosDelegate* s=weak;if(s&&!s->_closing&&s->_midiPulse[k]==generation){s->_midiHeld[k]=false;[s updatePedal:button panel:panel];}});
        }else{_midiHeld[k]=a.down;[self updatePedal:button panel:panel];}
    }else if(i<28){if(i==27&&_selectedBank!=3)return;ProgramDial* dial=i==27?_rearPanel.program:_program;const NSInteger position=MIN(11,NSInteger(std::round(a.value*127)*12/128));if(position!=dial.selected){dial.selected=position;if(dial.changed)dial.changed(position);}}
}
- (void)midiProgram:(unsigned)slot{
    if(!_midiSettings.programChange||_busy||_closing||_window.attachedSheet||slot>=32)return;
    NSMenuItem* item=[[NSMenuItem alloc]initWithTitle:@"" action:nil keyEquivalent:@""];item.tag=slot;[self slotChanged:item];
}
- (void)receiveMIDI:(cosmos_midi::Message)message{
    if(!_midiSettings.enabled||_busy||_closing)return;
    const bool learned=_midiSettings.router.receive(message,[&](cosmos_midi::Action a){[self applyMIDI:a];},[&](unsigned slot){[self midiProgram:slot];});
    if(learned){_midiNotice=nil;[self saveMIDISettings];[self updateMIDIDisplay];}
}
- (void)pollMIDI:(NSTimer*)timer{
    if(_closing||!_midiInput)return;
    if(_midiInput->changed()){
        if(_midiSettings.enabled&&_midiSettings.source){BOOL found=NO;for(const auto& source:_midiInput->sources())if(source.uid==_midiSettings.source)found=YES;
            if(!found){[self releaseMIDI];_midiInput->select(false,0,nullptr);_midiSettings.enabled=false;_audio->resetMidiClock();_audio->setMidiSync(false,_midiSettings.bars);_midiNotice=CL(@"MIDI 设备已断开，请重新选择输入。");}}
        [self reloadMIDISources];
    }
    const uint64_t lost=_midiInput->lost();if(lost!=_midiLost){_midiLost=lost;[self releaseMIDI];_midiInput->discardPending();_midiNotice=CL(@"MIDI 信息过多，踏板已松开，请重试。");}
    cosmos_midi::Message message;unsigned count=0;while(count++<512&&_midiInput->poll(message))[self receiveMIDI:message];
    if(++_midiDisplayTick%10==0)[self updateMIDIDisplay];
}
