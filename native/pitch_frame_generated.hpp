// PM1.61/PR1.61 per-interrupt model; original comparisons in research/pitch-banks-2026-10-30.
// Included after FrameResult definitions. Saturation disabled only during DSP; pitch control RAM retained.
inline FrameResult pitch_process_words(State& mem,bool bank_r,uint32_t a,uint32_t b,const uint32_t* adc,
        uint32_t gpiob,uint32_t gpiof,uint32_t selector,uint32_t handler_sp,uint32_t& period) {
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
        const auto out=bank_r?erase_bypass(mem,true,routed[0],routed[1]):q_erase_bypass(mem,routed[0],routed[1]);
        result.phase=FramePhase::EraseBypass;result.limited=out.limited;result.packed=out.packed;
    } else {
        result.leds=panel_step(mem,bank_r,gpiob);
        erase_feedback(mem,bank_r,gpiob);result.requested=erase_request(mem,bank_r,gpiob,result.leds);
        if (result.requested && bank_r) {
            result.phase=FramePhase::ClearRequestedReturn;
            result.limited={mem.rf(layout.outputs),mem.rf(layout.outputs+4)};
            result.packed={mem.ru(layout.packed),mem.ru(layout.packed+4)};
        } else {
            const auto& cr=controls_roles(!bank_r);const float prior=mem.rf(cr.smooth[2]);
            if (bank_r) controls_r(mem);else controls_m(mem,routed[0]);
            float pitch=mla(prior,add(sub(prior,divf(f32(float(adc[2])),3900.f)),from_bits(0x3ccccccdu)),from_bits(0xba83126fu));
            if(pitch<0)pitch=0;if(pitch>=from_bits(0x3f800001u))pitch=1;
            mem.wf(cr.smooth[2],pitch);period=1920u+uint32_t(mul(sub(1.f,pitch),1920.f));
            const uint32_t entry_r6=switch_step(mem,bank_r,selector,routed[0],handler_sp);
            const uint32_t current=mem.ru(layout.current);const float gain=mem.rf(layout.gain);
            std::array<float,2> raw{};
            result.dispatched=is_dsp_supported(bank_r?1:0,current);
            if (result.dispatched) {
                const uint8_t adc_byte=mem.rb(bank_r?0x200102b0u:0x20000288u);
                mem.wf(cr.smooth[2],0.f);raw=pitch_dsp_step(mem,bank_r,current,mul(routed[0],gain),mul(routed[1],gain),adc_byte,entry_r6);
            mem.wf(cr.smooth[2],pitch);
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
