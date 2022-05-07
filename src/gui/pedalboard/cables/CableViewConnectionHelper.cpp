#include "CableViewConnectionHelper.h"
#include "../BoardComponent.h"
#include "CableViewPortLocationHelper.h"
#include "processors/chain/ProcessorChainActionHelper.h"

namespace
{
std::unique_ptr<Cable> connectionToCable (const ConnectionInfo& connection)
{
    return std::make_unique<Cable> (connection.startProc, connection.startPort, connection.endProc, connection.endPort);
}

ConnectionInfo cableToConnection (const Cable& cable)
{
    return { cable.startProc, cable.startIdx, cable.endProc, cable.endIdx };
}

void updateConnectionStatuses (const BoardComponent* board, const ConnectionInfo& connection, bool isConnected)
{
    if (auto* editor = board->findEditorForProcessor (connection.startProc))
    {
        bool shouldBeConnected = isConnected || connection.startProc->getNumOutputConnections (connection.startPort) > 0;
        editor->setConnectionStatus (shouldBeConnected, connection.startPort, false);
    }

    if (auto* editor = board->findEditorForProcessor (connection.endProc))
        editor->setConnectionStatus (isConnected, connection.endPort, true);
}

void addConnectionsForProcessor (OwnedArray<Cable>& cables, BaseProcessor* proc, const BoardComponent* board)
{
    for (int portIdx = 0; portIdx < proc->getNumOutputs(); ++portIdx)
    {
        auto numConnections = proc->getNumOutputConnections (portIdx);
        for (int cIdx = 0; cIdx < numConnections; ++cIdx)
        {
            const auto& connection = proc->getOutputConnection (portIdx, cIdx);
            cables.add (connectionToCable (connection));

            updateConnectionStatuses (board, connection, true);
        }
    }
}
} // namespace

CableViewConnectionHelper::CableViewConnectionHelper (CableView& cv) : cableView (cv),
                                                                       board (cableView.board),
                                                                       cables (cableView.cables)
{
}

void CableViewConnectionHelper::processorBeingAdded (BaseProcessor* newProc)
{
    addConnectionsForProcessor (cables, newProc, board);
}

void CableViewConnectionHelper::processorBeingRemoved (const BaseProcessor* proc)
{
    for (int i = cables.size() - 1; i >= 0; --i)
    {
        if (cables[i]->startProc == proc || cables[i]->endProc == proc)
        {
            updateConnectionStatuses (board, cableToConnection (*cables[i]), false);
            cables.remove (i);
        }
    }
}

void CableViewConnectionHelper::refreshConnections()
{
    cables.clear();

    for (auto* proc : board->procChain.getProcessors())
        addConnectionsForProcessor (cables, proc, board);

    addConnectionsForProcessor (cables, &board->procChain.getInputProcessor(), board);

    cableView.repaint();
}

void CableViewConnectionHelper::connectionAdded (const ConnectionInfo& info)
{
    updateConnectionStatuses (board, info, true);

    if (ignoreConnectionCallbacks)
        return;

    cables.add (connectionToCable (info));

    cableView.repaint();
}

void CableViewConnectionHelper::connectionRemoved (const ConnectionInfo& info)
{
    updateConnectionStatuses (board, info, false);

    if (ignoreConnectionCallbacks)
        return;

    {
        for (auto* cable : cables)
            if (cable->startProc == info.startProc
                && cable->startIdx == info.startPort
                && cable->endProc == info.endProc
                && cable->endIdx == info.endPort)
            {
                cables.removeObject (cable);
                break;
            }
    }

    cableView.repaint();
}

void CableViewConnectionHelper::createCable (ProcessorEditor* origin, int portIndex, const MouseEvent& e)
{
    cables.add (std::make_unique<Cable> (origin->getProcPtr(), portIndex));
    cableMouse = std::make_unique<MouseEvent> (std::move (e.getEventRelativeTo (&cableView)));
    cableView.repaint();
}

void CableViewConnectionHelper::refreshCable (const MouseEvent& e)
{
    cableMouse = std::make_unique<MouseEvent> (std::move (e.getEventRelativeTo (&cableView)));
    cableView.repaint();
}

void CableViewConnectionHelper::releaseCable (const MouseEvent& e)
{
    cableMouse.reset();

    // check if we're releasing near an output port
    auto relMouse = e.getEventRelativeTo (&cableView);
    auto mousePos = relMouse.getPosition();

    auto* cable = cables.getLast();
    auto [editor, portIdx, _] = cableView.getPortLocationHelper()->getNearestInputPort (mousePos, cable->startProc);
    if (editor != nullptr)
    {
        cable->endProc = editor->getProcPtr();
        cable->endIdx = portIdx;

        const ScopedValueSetter<bool> svs (ignoreConnectionCallbacks, true);
        auto connection = cableToConnection (*cable);
        board->procChain.getActionHelper().addConnection (std::move (connection));

        cableView.repaint();
        return;
    }

    // not being connected... trash the latest cable
    cables.removeObject (cables.getLast());

    cableView.repaint();
}

void CableViewConnectionHelper::destroyCable (ProcessorEditor* origin, int portIndex)
{
    const auto* proc = origin->getProcPtr();
    for (auto* cable : cables)
    {
        if (cable->endProc == proc && cable->endIdx == portIndex)
        {
            const ScopedValueSetter<bool> svs (ignoreConnectionCallbacks, true);
            board->procChain.getActionHelper().removeConnection (cableToConnection (*cable));
            cables.removeObject (cable);

            break;
        }
    }

    cableView.repaint();
}
