#include "ProcessorChainPortMagnitudesHelper.h"

ProcessorChainPortMagnitudesHelper::ProcessorChainPortMagnitudesHelper (ProcessorChain& procChain) : chain (procChain)
{
    pluginSettings->addProperties ({ { cableVizOnOffID, true } }, this);
    portMagsOn.store (pluginSettings->getProperty<bool> (cableVizOnOffID));
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

void ProcessorChainPortMagnitudesHelper::globalSettingChanged (SettingID settingID)
{
    if (settingID != cableVizOnOffID)
        return;

    const auto isNowOn = pluginSettings->getProperty<bool> (settingID);
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
