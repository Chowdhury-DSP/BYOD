#include "ProcessorChainPortMagnitudesHelper.h"

ProcessorChainPortMagnitudesHelper::ProcessorChainPortMagnitudesHelper (ProcessorChain& procChain) : chain (procChain)
{
    pluginSettings->addProperties ({ { cableVizOnOffID, true } }, this);
    portMagsOn.store ((bool) pluginSettings->getProperty (cableVizOnOffID));
    prevPortMagsOn = portMagsOn.load();

    chain.addListener (this);
    chain.getInputProcessor().resetPortMagnitudes (prevPortMagsOn);
    chain.getOutputProcessor().resetPortMagnitudes (prevPortMagsOn);
}

ProcessorChainPortMagnitudesHelper::~ProcessorChainPortMagnitudesHelper()
{
    chain.removeListener (this);
    pluginSettings->removePropertyListener (this);
}

void ProcessorChainPortMagnitudesHelper::processorAdded (BaseProcessor* proc)
{
    proc->resetPortMagnitudes (portMagsOn.load());
}

void ProcessorChainPortMagnitudesHelper::propertyChanged (const Identifier& settingID, const var& property)
{
    if (settingID != cableVizOnOffID)
        return;

    if (! property.isBool())
        return;

    const auto isNowOn = (bool) property;
    Logger::writeToLog ("Turning cable visualization: " + String (isNowOn ? "ON" : "OFF"));
    portMagsOn.store (isNowOn);
}

void ProcessorChainPortMagnitudesHelper::preparePortMagnitudes()
{
    if (portMagsOn.load() == prevPortMagsOn)
        return;

    prevPortMagsOn = portMagsOn.load();

    chain.getInputProcessor().resetPortMagnitudes (prevPortMagsOn);
    chain.getOutputProcessor().resetPortMagnitudes (prevPortMagsOn);
    for (auto* proc : chain.getProcessors())
        proc->resetPortMagnitudes (prevPortMagsOn);
}

const Identifier ProcessorChainPortMagnitudesHelper::cableVizOnOffID { "cable_viz_onoff" };
