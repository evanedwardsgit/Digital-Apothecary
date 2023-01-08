/*
* DAcompnentlibrary - The library of custom components for the Digital Apothecary Plugin
* Copyright 2022, Evan JJ Edwards - GPLv3 or later
* Email eedwards6@wisc.edu with any comments, concerns, or suggestions
* 
* I would like to reiterate the intellectual properties utilized in the graphics that are referenced here
* Rogan1PSRed, Rogan1PSRed_fg adapted into R-Style1PPink, R-Style1PPink_fg respectively
* All of the above is under Component Library graphics - © 2016-2021 VCV - CC BY-NC 4.0
*
* Under the terms of this license, I have adapted these graphics, referenced under "R-Style"
* Appropriate use of licensing and giving credit is imporant to me, and if you have any sugestions on how to better do this, please contact me
*/

#pragma once

#include <rack.hpp>
#include <asset.hpp>

//d35854ff
static const NVGcolor SCHEME_PINK = nvgRGB(0xd3, 0x58, 0x54);

namespace rack::componentlibrary{
    using namespace window;
    
    struct RStyle1PPink : Rogan{
        RStyle1PPink(){
            setSvg(Svg::load(asset::plugin(pluginInstance, "res/DAComponents/R-Style1PPink.svg")));
            bg->setSvg(Svg::load(asset::system("res/ComponentLibrary/Rogan1P_bg.svg")));
            fg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/DAComponents/R-Style1PPink_fg.svg")));
            
        }
    };

    template <typename TBase = GrayModuleLightWidget>
    struct TPinkLight : TBase {
        TPinkLight() {
            this->addBaseColor(SCHEME_PINK);
        }
    };
    using PinkLight = TPinkLight<>;

}