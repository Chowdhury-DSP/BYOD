#include "ProcessorChainPortMagnitudesHelper.h"

ProcessorChainPortMagnitudesHelper::ProcessorChainPortMagnitudesHelper (ProcessorChain& procChain) : chain (procChain)
{
    pluginSettings->addProperties<&ProcessorChainPortMagnitudesHelper::globalSettingChanged> ({ { cableVizOnOffID, true } }, *this);
    portMagsOn.store (pluginSettings->getProperty<bool> (cableVizOnOffID));
    prevPortMagsOn = portMagsOn.load();

    onProcessorAdded = chain.processorAddedBroadcaster.connect ([this] (BaseProcessor* proc)
                                                                { proc->resetPortMagnitudes (portMagsOn.load()); });

    chain.getInputProcessor().resetPortMagnitudes (prevPortMagsOn);
    chain.getOutputProcessor().resetPortMagnitudes (prevPortMagsOn);
}

ProcessorChainPortMagnitudesHelper::~ProcessorChainPortMagnitudesHelper()
{
    pluginSettings->removePropertyListener (*this);
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
