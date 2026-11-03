// SPDX-License-Identifier: AGPL-3.0-only
#include "../Source/HybridDSP.h"
#include "../Source/SynthParameters.h"
#include <iostream>
#include <memory>
#include <stdexcept>

void require(bool ok,const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    try {
        aetherloom::SineTable sine;
        auto bank=std::make_unique<aetherloom::WavetableBank>(sine);
        for (int wave=0;wave<5;++wave) for (float width:{0.05f,0.5f,0.95f}) for (double delta:{0.0001,0.01,0.24,0.45})
            for (int n=0;n<2048;++n) {
                const auto value=bank->oscillator(wave,n/2048.0,delta,0.72f,width,sine);
                require(std::isfinite(value) && std::abs(value)<2.1f,"Waveforms must remain finite/bounded");
            }
        const auto before=bank->oscillator(2,0.999999,0.02,0,0.5f,sine);
        const auto after=bank->oscillator(2,0.000001,0.02,0,0.5f,sine);
        require(std::abs(before-after)<0.001f,"PolyBLEP must remove the saw wrap discontinuity");
        for (int n=0;n<1024;++n)
            require(std::abs(bank->read(n/1024.0,0.24,2.0f/3)-sine(n/1024.0))<0.00001f,"High spectral notes must select fundamental-only mip");
        for (double rate:{8000.0,44100.0,48000.0,96000.0,192000.0}) for (int type=0;type<4;++type) {
            aetherloom::StateFilter filter;
            filter.configure(rate,static_cast<float>(rate),0.95f);
            double energy=0;
            for (int n=0;n<20000;++n) {
                const auto value=filter.process(n==0 ? 1.0f:0.0f,type);
                require(std::isfinite(value) && std::abs(value)<10,"Resonant filter must remain stable"); energy+=value*value;
            }
            require(energy>0.00001,"Every filter mode must respond to an impulse");
        }
        aetherloom::StateFilter low,high;
        low.configure(48000,500,0.2f); high.configure(48000,500,0.2f);
        float l=0,h=0;
        for (int n=0;n<2000;++n) { l=low.process(1,0);h=high.process(1,2); }
        require(std::abs(l-1)<0.001f && std::abs(h)<0.001f,"LP passes DC and HP rejects DC");
        for (size_t i=0;i<aetherloom::specs.size();++i) {
            const auto& spec=aetherloom::specs[i];
            require(spec.initial>=spec.low && spec.initial<=spec.high,"Parameter default must be in range");
            for (size_t j=i+1;j<aetherloom::specs.size();++j)
                require(std::string(spec.id)!=aetherloom::specs[j].id,"Parameter IDs must be unique");
        }
        std::cout << "Passed: all waveforms, PolyBLEP seam, spectral mip selection, filter stability/DC, parameter contracts\n";
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
