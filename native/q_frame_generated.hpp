// Included inside cosmos_native after FrameResult definitions.
inline OutputTail q_erase_bypass(State& mem,float a,float b) {
    const float dry=sub(1.f,mem.rf(0x2000047cu));
    const auto out=output_tail(mul(dry,a),mul(dry,b),mem.ru(0x200001a0u)!=0,mem.rf(0x2000027cu),mem.rf(0x20000278u));
    mem.wf(0x20000454u,a);
    for(unsigned i=0;i<2;++i){mem.wf(0x2000045cu+4*i,out.limited[i]);mem.wu(0x2000043cu+4*i,out.packed[i]);}
    return out;
}
inline FrameResult q_process_words(State& mem,uint32_t a,uint32_t b,const uint32_t* adc,
        uint32_t gpiob=0xffffu,uint32_t gpiof=0xc0u,uint32_t selector=0x800u,uint32_t handler_sp=0x2001f000u) {
    constexpr bool bank_r=false;
    const auto& layout=frame_layout(bank_r);
    if (mem.ru(layout.special)) throw std::domain_error("factory diagnostics are not a software audio mode");
    if (!adc) throw std::invalid_argument("six ADC values required");
    controls_apply_inputs(mem,!bank_r,adc);
    mem.wu(layout.slots,a);mem.wu(layout.slots+4,b);
    FrameResult result;result.tx={mem.ru(layout.packed),mem.ru(layout.packed+4)};
    mem.wu(layout.timing,0);mem.wu(layout.tx,result.tx[0]);mem.wu(layout.tx+4,result.tx[1]);
    if (!bank_r && mem.ru(erase_request_address(false)) && !std::isfinite(mem.rf(0x2000047c)))
        throw std::domain_error("finite bypass wet mix required");
    const auto routed=input_stage(mem,bank_r,decode_slot(a),decode_slot(b),gpiof);
    if (mem.ru(erase_request_address(bank_r))) {
        const auto out=q_erase_bypass(mem,routed[0],routed[1]);
        result.phase=FramePhase::EraseBypass;result.limited=out.limited;result.packed=out.packed;
    } else {
        result.leds=panel_step(mem,bank_r,gpiob);
        erase_feedback(mem,bank_r,gpiob);result.requested=erase_request(mem,bank_r,gpiob,result.leds);
        if (result.requested && bank_r) {
            result.phase=FramePhase::ClearRequestedReturn;
            result.limited={mem.rf(layout.outputs),mem.rf(layout.outputs+4)};
            result.packed={mem.ru(layout.packed),mem.ru(layout.packed+4)};
        } else {
            if (bank_r) controls_r(mem);else controls_m(mem,routed[0]);
            const uint32_t entry_r6=q_switch_step(mem,selector,routed[0],handler_sp);
            const uint32_t current=mem.ru(layout.current);const float gain=mem.rf(layout.gain);
            std::array<float,2> raw{};
            result.dispatched=is_q_supported(current);
            if (result.dispatched) {
                const uint8_t adc_byte=mem.rb(bank_r?0x200102b0u:0x20000288u);
                raw=q_dsp_step(mem,current,mul(routed[0],gain),mul(routed[1],gain),adc_byte,entry_r6);
            } else {
                mem.wf(layout.inputs,mul(routed[0],gain));mem.wf(layout.inputs+4,mul(routed[1],gain));
                raw={mem.rf(layout.outputs),mem.rf(layout.outputs+4)};
            }
            if (!bank_r) mem.wf(layout.inputs,mul(routed[0],gain));
            const auto out=output_tail(raw[0],raw[1],mem.ru(layout.mono)!=0,gain,mem.rf(layout.env));
            result.limited=out.limited;result.packed=out.packed;
            for (unsigned i=0;i<2;++i) { mem.wf(layout.outputs+4*i,out.limited[i]);mem.wu(layout.packed+4*i,out.packed[i]); }
        }
    }
    // A mode-change commit may explicitly clear the TX buffer after the
    // prologue copy. Publish its actual final contents, as the original does.
    result.tx={mem.ru(layout.tx),mem.ru(layout.tx+4)};
    result.current=mem.ru(layout.current);return result;
}
