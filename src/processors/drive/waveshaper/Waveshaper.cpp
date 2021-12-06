#include "Waveshaper.h"
#include "../../ParameterHelpers.h"

namespace
{
const String shapeTag = "shape";
}

using namespace SurgeWaveshapers;

Waveshaper::Waveshaper (UndoManager* um) : BaseProcessor ("Waveshaper", createParameterLayout(), um)
{
    driveParam = vts.getRawParameterValue ("drive");
    shapeParam = vts.getRawParameterValue (shapeTag);

    // borrowed from: https://github.com/surge-synthesizer/surge/blob/main/src/surge-fx/SurgeLookAndFeel.h
    const Colour surgeOrange = Colour (255, 144, 0);
    const Colour surgeBlue = Colour (18, 52, 99);

    uiOptions.backgroundColour = surgeBlue;
    uiOptions.powerColour = surgeOrange;
    uiOptions.info.description = "Waveshaping effects borrowed from the venerable Surge Synthesizer.";
    uiOptions.info.authors = StringArray { "Surge Synthesizer Team" };
    uiOptions.info.infoLink = "https://surge-synthesizer.github.io";
}

AudioProcessorValueTreeState::ParameterLayout Waveshaper::createParameterLayout()
{
    using namespace ParameterHelpers;

    auto params = createBaseParams();
    createGainDBParameter (params, "drive", "Drive", -6.0f, 30.0f, 0.0f);

    params.push_back (std::make_unique<AudioParameterChoice> (shapeTag, "Shape", wst_names, wst_ojd));

    return { params.begin(), params.end() };
}

void Waveshaper::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    driveSmooth.reset (sampleRate, 0.05);
    driveSmooth.setCurrentAndTargetValue (Decibels::decibelsToGain (driveParam->load()));
}

void Waveshaper::processAudio (AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    auto driveGain = Decibels::decibelsToGain (driveParam->load());
    driveSmooth.setTargetValue (driveGain);

    if ((int) *shapeParam != lastShape)
    {
        lastShape = (int) *shapeParam;

        float R[n_waveshaper_registers];
        initializeWaveshaperRegister (lastShape, R);
        for (int i = 0; i < n_waveshaper_registers; ++i)
            wss.R[i] = Vec4 (R[i]);
        wss.init = dsp::SIMDRegister<uint32_t> (0);
    }

    auto wsptr = GetQFPtrWaveshaper (lastShape);

    if (wsptr)
    {
        float din alignas (16)[4] = { 0, 0, 0, 0 };
        if (numChannels == 1)
        {
            auto* data = buffer.getWritePointer (0);

            for (int i = 0; i < numSamples; ++i)
            {
                din[0] = data[i];
                auto dat = Vec4::fromRawArray (din);
                auto drv = Vec4 (driveSmooth.getNextValue());

                dat = wsptr (&wss, dat, drv);

                float res alignas (16)[4];
                dat.copyToRawArray (res);

                data[i] = res[0];
            }
        }
        else if (numChannels == 2)
        {
            auto* left = buffer.getWritePointer (0);
            auto* right = buffer.getWritePointer (1);

            for (int i = 0; i < numSamples; ++i)
            {
                din[0] = left[i];
                din[1] = right[i];
                auto dat = Vec4::fromRawArray (din);
                auto drv = Vec4 (driveSmooth.getNextValue());

                dat = wsptr (&wss, dat, drv);

                float res alignas (16)[4];
                dat.copyToRawArray (res);

                left[i] = res[0];
                right[i] = res[1];
            }
        }
    }
}

void Waveshaper::getCustomComponents (OwnedArray<Component>& customComps)
{
    struct CustomBoxAttach : private ComboBox::Listener
    {
    public:
        CustomBoxAttach (RangedAudioParameter& parameter, ComboBox& combo, UndoManager* undoManager = nullptr)
            : comboBox (combo),
              attachment (
                  parameter,
                  [this] (float f)
                  { setValue (f); },
                  undoManager)
        {
            sendInitialUpdate();
            comboBox.addListener (this);
        }

        ~CustomBoxAttach() override { comboBox.removeListener (this); }

        void sendInitialUpdate() { attachment.sendInitialUpdate(); }

    private:
        void setValue (float newValue)
        {
            const auto index = (int) newValue;

            if (index == comboBox.getSelectedItemIndex())
                return;

            const ScopedValueSetter<bool> svs (ignoreCallbacks, true);
            comboBox.setSelectedId (index + 1, sendNotificationSync);
        }

        void comboBoxChanged (ComboBox*) override
        {
            if (ignoreCallbacks)
                return;

            const auto selected = comboBox.getSelectedId() - 1;
            attachment.setValueAsCompleteGesture ((float) selected);
        }

        ComboBox& comboBox;
        ParameterAttachment attachment;
        bool ignoreCallbacks = false;
    };

    struct WaveshapeComboBox : public ComboBox
    {
        WaveshapeComboBox (AudioProcessorValueTreeState& vts, const String& paramID)
        {
            auto* param = vts.getParameter (paramID);
            attachment = std::make_unique<CustomBoxAttach> (*param, *this, vts.undoManager);
            shapeParam = vts.getRawParameterValue (shapeTag);
            refreshBox();

            setName (shapeTag + "__box");
        }

        void refreshBox()
        {
            clear();
            auto* menu = getRootMenu();

            std::map<String, std::vector<ws_type>> menuMap {
                { "Saturator", { wst_soft, wst_zamsat, wst_hard, wst_asym, wst_ojd } },
                { "Effect", { wst_sine, wst_digital } },
                { "Harmonic", { wst_cheby2, wst_cheby3, wst_cheby4, wst_cheby5, wst_add12, wst_add13, wst_add14, wst_add15, wst_add12345, wst_addsaw3, wst_addsqr3 } },
                { "Rectifiers", { wst_fwrectify, wst_poswav, wst_negwav, wst_softrect } },
                { "Wavefolder", { wst_softfold, wst_singlefold, wst_dualfold, wst_westfold } },
                { "Fuzz", { wst_fuzz, wst_fuzzheavy, wst_fuzzctr, wst_fuzzsoft, wst_fuzzsoftedge } },
                { "Trigonometric", { wst_sinpx, wst_sin2xpb, wst_sin3xpb, wst_sin7xpb, wst_sin10xpb, wst_2cyc, wst_7cyc, wst_10cyc, wst_2cycbound, wst_7cycbound, wst_10cycbound } },
            };

            auto currentShape = (int) *shapeParam;
            for (auto& map : menuMap)
            {
                PopupMenu typeMenu;
                for (auto type : map.second)
                    typeMenu.addItem ((int) type + 1, wst_names[type], true, currentShape == type);

                menu->addSubMenu (map.first, typeMenu);
            }

            setSelectedId (currentShape + 1);
        }

        std::atomic<float>* shapeParam = nullptr;
        std::unique_ptr<CustomBoxAttach> attachment;
    };

    customComps.add (std::make_unique<WaveshapeComboBox> (vts, shapeTag));
}
