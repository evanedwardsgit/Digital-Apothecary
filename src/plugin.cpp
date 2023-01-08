#include "plugin.hpp"

Plugin *pluginInstance;

void init(Plugin *p){
	pluginInstance = p;
	
	p->addModel(modelSage);
	p->addModel(modelLinden);
	p->addModel(modelRue);
}
